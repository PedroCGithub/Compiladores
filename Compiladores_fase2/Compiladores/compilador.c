//  COMPILADORES - PROJETO Fase 2 - MiniVisualg
//  Fase 1: analisador lexico + analisador sintatico
//  Fase 2: analisador semantico + geracao de codigo MEPA
//  Integrantes:
//    - Henrique Ferreira Marciano - RA 10439797
//    - Pedro Casas Pequeno Junior - RA 10437031
//    - Pedro Gabriel Guimarães Fernandes - RA 10437465
//  Professora, as explicacoes da gramatica e das decisoes de projeto
//  estao no arquivo Readme.txt (secao 6 = Fase 2).
//  Uso de IA: ver Readme.txt, secao 6.9.
//  Compilar: gcc -g -Og -Wall compilador.c -o compilador
//  Executar: ./compilador programa.txt [arquivo_mepa.txt] [-completo]
//              arquivo_mepa.txt: nome do arquivo MEPA (padrao saida_mepa.txt)
//              -completo: rotinas com CHPR/ENPR/RTPR (permite recursao),
//                         ver Readme.txt 6.8; sem ele, so instrucoes da aula
//  Saidas:   saida.txt            -> tokens e mensagens de erro
//            saida_mepa.txt       -> codigo MEPA no estilo da aula (nomes)
//            saida_mepa_exec.txt  -> mesmo codigo no formato do InterprMEPA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>

#define TAM_LEXEMA 256
#define MAX_SIMBOLOS 500
#define MAX_PARAMS 8

// Geracao de codigo (Fase 2)
#define ARQ_MEPA_PADRAO "saida_mepa.txt"       // estilo da aula
#define TAM_NOME_ARQ    512
#define TAM_LINHA_MEPA  (2 * TAM_LEXEMA + 64)  // cabe instrucao + nomes
#define QTD_TEMPORARIAS 2   // posicoes extras de memoria usadas pelo compilador
#define TMP_INDICE      0   // guarda o indice de um vetor (ou o "a" do MOD)
#define TMP_VALOR       1   // guarda um valor (gravar em vetor, "b" do MOD)

// Definicao de tipos
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,
    TOKEN_NUM_INT,
    TOKEN_NUM_FLOAT,
    TOKEN_OP_REL,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_SIMBOLO
} TokenNome;

const char *nomesTokens[] = {
    "EOF", "ID", "NUM_INT", "NUM_FLOAT", "OP_REL",
    "KEYWORD", "STRING", "SIMBOLO"
};

// OP_NE = operador "<>" (ver README, secao 4.5, sobre o bug corrigido)
typedef enum {
    OP_LT, // <
    OP_LE, // <=
    OP_EQ, // =
    OP_GT, // >
    OP_GE, // >=
    OP_NE  // <>
} OpRelAtributo;

const char *nomesOpRel[] = { "OP_LT", "OP_LE", "OP_EQ", "OP_GT", "OP_GE", "OP_NE" };

typedef struct {
    TokenNome type;
    int line;
    char lexema[TAM_LEXEMA];

    union {
        int table_index;
        int int_value;
        double float_value;
        OpRelAtributo op_code;
    } attribute;
} Token;

// Categoria de cada identificador da tabela de simbolos (Fase 2)
typedef enum {
    CAT_VAR,
    CAT_VETOR,
    CAT_PARAM,
    CAT_PROC,
    CAT_FUNC
} Categoria;

// Tipos da linguagem; T_NENHUM = "sem tipo" (ex.: procedimento)
typedef enum {
    T_INT,
    T_REAL,
    T_CHAR,
    T_LOG,
    T_NENHUM
} TipoBase;

const char *nomesTipos[] = { "inteiro", "real", "caractere", "logico", "sem tipo" };

// Texto que cresce conforme o codigo MEPA e gerado (ver README 6.5)
typedef struct {
    char *texto;
    size_t tamanho;     // quantos caracteres ja foram usados
    size_t capacidade;  // quantos cabem antes de precisar de realloc
} Buffer;

// Um trecho de codigo MEPA nas duas versoes que o compilador grava
typedef struct {
    Buffer aula;   // estilo da aula: "CRVL soma"
    Buffer exec;   // formato do InterprMEPA: "CRVL 0, 2"
} Codigo;

const char *palavrasReservadas[] = {
    "algoritmo", "var", "inicio", "fimalgoritmo",
    "caractere", "inteiro", "real", "logico",
    "verdadeiro", "falso",
    "leia", "escreva", "escreval",
    "se", "entao", "senao", "fimse",
    "para", "de", "ate", "passo", "faca", "fimpara",
    "enquanto", "fimenquanto",
    "vetor",
    "procedimento", "fimprocedimento",
    "funcao", "fimfuncao", "retorne",
    "MOD", "E", "OU"
};
#define QTD_PALAVRAS (sizeof(palavrasReservadas) / sizeof(palavrasReservadas[0]))

const char *simbolos[] = {
    "<-", "..",
    "+", "-", "*", "/", "\\",
    "(", ")", "[", "]", ":", ","
};
#define QTD_SIMBOLOS (sizeof(simbolos) / sizeof(simbolos[0]))

// Funcoes do analisador lexico (Etapa 2)
void iniciarAnalisador(FILE *arquivo);
int fimDoArquivo();
Token obterToken();
TokenNome classificarLexema(char *lexema);
void imprimirToken(Token token);
void fecharAnalisador();

int ehDelimitador(int c);
int ehOperador(int c);
int ehLetra(int c);
int ehDigito(int c);
int peek();
void voltar();
void pularEspacosEComentarios();
void guardar(Token *token, int *i, int c);
int inserirSimbolo(char *nome);
noreturn void erroLexico(char *sequencia, char *motivo);

// Variaveis globais do analisador lexico
FILE *fonte;
int linhaAtual = 1;
FILE *saida = NULL;
char tabelaSimbolos[MAX_SIMBOLOS][TAM_LEXEMA];
int  qtdSimbolos = 0;

// Tabela de simbolos estendida (Fase 2) - ver README 6.1
// a posicao i de cada vetor descreve o identificador tabelaSimbolos[i]
int       declarada[MAX_SIMBOLOS];   // 1 = foi declarado e esta no escopo
Categoria categoria[MAX_SIMBOLOS];   // variavel, vetor, parametro, rotina
TipoBase  tipoBase[MAX_SIMBOLOS];    // tipo (vetor: dos elementos; funcao: do retorno)
int       endereco[MAX_SIMBOLOS];    // ordem de declaracao (0, 1, 2...)
int       linhaDecl[MAX_SIMBOLOS];   // linha onde foi declarado
int       atribuida[MAX_SIMBOLOS];   // recebeu valor (<-, leia ou para)
int       usada[MAX_SIMBOLOS];       // teve o valor lido em alguma expressao
int       limInf[MAX_SIMBOLOS];      // limites do vetor
int       limSup[MAX_SIMBOLOS];
int       qtdParams[MAX_SIMBOLOS];   // assinatura de procedimento/funcao
TipoBase  tipoParam[MAX_SIMBOLOS][MAX_PARAMS];

int proxEndereco = 0;           // proximo endereco livre
int rotinaAtual = -1;           // indice da rotina em analise (-1 = programa principal)
int rotuloFimRotina = 0;        // rotulo do fim da rotina atual (destino do retorne)
int funcaoTemRetorne = 0;       // a funcao em analise ja teve algum retorne?

// Geracao de codigo (Fase 2) - ver README 6.5
Codigo codigoPrincipal;               // comandos do programa principal
Codigo codigoRotina[MAX_SIMBOLOS];    // "molde" com o corpo de cada rotina
Codigo *destino = &codigoPrincipal;   // onde as funcoes emitir*() escrevem agora
int modoCompleto = 0;                 // 1 = opcao -completo (README 6.8)
int usouTemporaria = 0;               // 1 = algum vetor ou MOD usou _tmp0/_tmp1
char arqMepa[TAM_NOME_ARQ];           // arquivo no estilo da aula
char arqMepaExec[TAM_NOME_ARQ];       // mesmo codigo para o InterprMEPA
int rotuloPrincipal = 0;              // so no modo completo: inicio do programa
int rotuloRotina[MAX_SIMBOLOS];       // so no modo completo: inicio de cada rotina
int contadorRotulos = 0;              // ultimo rotulo entregue por proximo_rotulo()
int proxOculta = QTD_TEMPORARIAS;     // proxima posicao escondida livre
int primeiroRotulo[MAX_SIMBOLOS];     // faixa de rotulos usada no molde da rotina
int ultimoRotulo[MAX_SIMBOLOS];
int posicaoResultado[MAX_SIMBOLOS];   // posicao escondida do resultado da funcao
int paramDaRotina[MAX_SIMBOLOS][MAX_PARAMS];  // indice (tabela) de cada parametro
int posicaoParam[MAX_SIMBOLOS][MAX_PARAMS];   // posicao escondida de cada parametro

// Funcoes do analisador semantico (Fase 2)
noreturn void erroSemantico(char *identificador, char *motivo);
noreturn void erroSemanticoNaLinha(int linha, char *identificador, char *motivo);
void declarar(int idx, Categoria cat, TipoBase tipo);
int busca_tabela_simbolos(char *id);
int proximo_rotulo(void);
void usarComoVariavel(int idx, int temIndice, int linha);
void usarComoRotina(int idx, int linha);
void iniciarRotina(int idx, Categoria cat);
void encerrarRotina(void);
int ehNumerico(TipoBase tipo);
int podeAtribuir(TipoBase destino, TipoBase origem);
TipoBase tipoAritmetico(TipoBase esq, TipoBase dir, int linha, const char *operador);
void exigirTipo(TipoBase obtido, TipoBase esperado, int linha, const char *contexto);
void verificarVariaveisNaoUsadas(void);

// Funcoes da geracao de codigo MEPA (Fase 2)
void iniciarGeracao(void);
void anexarBuffer(Buffer *buffer, const char *texto);
void emitirLinhas(const char *linhaAula, const char *linhaExec);
void emitir(const char *instrucao);
void emitirInt(const char *instrucao, int valor);
void emitirDivisaoReal(void);
void emitirRotulo(int rotulo);
void emitirRotuloInstrucao(int rotulo, const char *instrucao);
void emitirDesvio(const char *instrucao, int rotulo);
void emitirOculta(const char *instrucao, int posicao, const char *nomeAula);
void emitirTemporaria(const char *instrucao, int temporaria);
void emitirVariavel(const char *instrucao, int idx);
void emitirElementoVetor(const char *instrucao, int idxVetor, int posicao);
void emitirLeituraVetor(int idxVetor);
void emitirEscritaVetor(int idxVetor);
void emitirRetornoFuncao(int idxFuncao);
void copiarMolde(Buffer *saidaBuffer, Buffer *molde, int primeiro, int base);
void expandirRotina(int idxRotina, int linha);
noreturn void erroGeracao(int linha, char *identificador, char *motivo);
void gravarBuffer(FILE *arquivo, Buffer *buffer, int trocarOcultas);
void gravarCodigoMepa(void);
void lerArgumentos(int argc, char *argv[]);
void aviso(int linha, const char *mensagem);

// Funcoes do analisador sintatico (Etapa 3)
// Uma funcao por nao-terminal da gramatica (ver README, secao 3).

Token tokenAtual; // token atual (lookahead), usado por todo o parser

void avancar(void);
int tokenEh(const char *lexema);
void casar(const char *lexemaEsperado);
void casarTipo(TokenNome tipoEsperado, const char *descricao);
int inicioDeComando(void);
noreturn void erroSintatico(char *tokenEncontrado, char *motivo);

void parsePrograma(void);
void parseListaSubprogramas(void);
void parseSubprograma(void);
void parseProcedimento(void);
void parseFuncao(void);
void parseParametrosOpc(void);
void parseListaParametros(void);
void parseParametro(void);
void parseSecaoVar(void);
void parseListaDeclaracoes(void);
void parseDeclaracao(void);
void parseListaIds(int ids[], int *qtdIds);
TipoBase parseTipo(int *ehVetor, int *limiteInf, int *limiteSup);
TipoBase parseTipoBase(void);
void parseListaComandos(void);
void parseComando(void);
void parseAtribuicaoOuChamada(void);
void parseArgumentos(int idxRotina, int linha);
void parseComandoSe(void);
void parseComandoPara(void);
int parsePasso(void);
void parseComandoEnquanto(void);
void parseComandoLeia(void);
void parseVariavelLeia(void);
void parseComandoEscreva(void);
void parseComandoRetorne(void);
TipoBase parseExpressao(void);
TipoBase parseExprE(void);
TipoBase parseExprRel(void);
TipoBase parseExprArit(void);
TipoBase parseTermo(void);
TipoBase parseFator(void);
TipoBase parseIdentificadorEmExpressao(void);
void parseIndiceVetor(int idxVetor);

// main
int main(int argc, char *argv[])
{

    if (argc < 2) {
        printf("Uso: compilador <arquivo_fonte> [arquivo_mepa.txt] [-completo]\n");
        return 0;
    }
    lerArgumentos(argc, argv);
    fonte = fopen(argv[1], "rb");
    if (fonte == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    iniciarAnalisador(fonte);
    iniciarGeracao();

    avancar(); // pega o primeiro token (nextToken)
    parsePrograma();

    if (tokenAtual.type != TOKEN_EOF) {
        erroSintatico(tokenAtual.lexema, "codigo inesperado depois de 'fimalgoritmo'");
    }

    // so chega aqui se nao houve erro lexico, sintatico nem semantico
    gravarCodigoMepa();

    printf("\n>>> Programa aceito: a cadeia de tokens pertence a linguagem MiniVisualg.\n");
    printf(">>> Codigo MEPA gerado em %s (e %s)%s.\n", arqMepa, arqMepaExec,
           modoCompleto ? ", modo completo" : "");

    fecharAnalisador();
    return 0;
}

// Analisador lexico (Etapa 2)
void iniciarAnalisador(FILE *arquivo)
{
    fonte = arquivo;
    linhaAtual = 1;

    saida = fopen("saida.txt", "w");
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida.\n");
        exit(0);
    }
}

int fimDoArquivo()
{
    pularEspacosEComentarios();
    return peek() == EOF;
}

Token obterToken()
{
    Token token;
    int c;
    int i = 0;

    pularEspacosEComentarios();

    token.line = linhaAtual;
    token.attribute.int_value = 0;

    c = fgetc(fonte);

    if (c == EOF) {
        token.type = TOKEN_EOF;
        strcpy(token.lexema, "EOF");
        return token;
    }

    if (ehLetra(c)) {
        guardar(&token, &i, c);
        while (ehLetra(peek()) || ehDigito(peek()) || peek() == '_') {
            guardar(&token, &i, fgetc(fonte));
        }
    }
    else if (ehDigito(c)) {
        guardar(&token, &i, c);
        while (ehDigito(peek())) {
            guardar(&token, &i, fgetc(fonte));
        }

        if (peek() == '.') {
            fgetc(fonte);
            if (ehDigito(peek())) {
                guardar(&token, &i, '.');
                while (ehDigito(peek())) {
                    guardar(&token, &i, fgetc(fonte));
                }
            } else {
                voltar();
            }
        }

        if (ehLetra(peek()) || peek() == '_') {
            while (ehLetra(peek()) || ehDigito(peek()) || peek() == '_') {
                guardar(&token, &i, fgetc(fonte));
            }
            token.lexema[i] = '\0';
            erroLexico(token.lexema, "numero malformado");
        }
    }
    else if (c == '"') {
        guardar(&token, &i, c);
        while (peek() != '"') {
            if (peek() == '\n' || peek() == EOF) {
                token.lexema[i] = '\0';
                erroLexico(token.lexema, "cadeia nao foi fechada");
            }
            guardar(&token, &i, fgetc(fonte));
        }
        guardar(&token, &i, fgetc(fonte));
    }
    else if (ehOperador(c)) {
        guardar(&token, &i, c);
        // '<=' '<-' '<>' e '>=' sao os unicos operadores de 2 caracteres
        if ((c == '<' && (peek() == '=' || peek() == '>' || peek() == '-'))
            || (c == '>' && peek() == '=')) {
            guardar(&token, &i, fgetc(fonte));
        }
    }
    else if (ehDelimitador(c)) {
        guardar(&token, &i, c);
        if (c == '.') {
            if (peek() != '.') {
                token.lexema[i] = '\0';
                erroLexico(token.lexema, "ponto sozinho (esperado ..)");
            }
            guardar(&token, &i, fgetc(fonte));
        }
    }
    else {
        guardar(&token, &i, c);
        token.lexema[i] = '\0';
        erroLexico(token.lexema, "caractere nao pertence a linguagem");
    }

    token.lexema[i] = '\0';

    token.type = classificarLexema(token.lexema);

    if (token.type == TOKEN_ID) {
        token.attribute.table_index = inserirSimbolo(token.lexema);
    }
    else if (token.type == TOKEN_NUM_INT) {
        token.attribute.int_value = atoi(token.lexema);
    }
    else if (token.type == TOKEN_NUM_FLOAT) {
        token.attribute.float_value = atof(token.lexema);
    }
    else if (token.type == TOKEN_OP_REL) {
        if (strcmp(token.lexema, "<") == 0)  token.attribute.op_code = OP_LT;
        if (strcmp(token.lexema, "<=") == 0) token.attribute.op_code = OP_LE;
        if (strcmp(token.lexema, "=") == 0)  token.attribute.op_code = OP_EQ;
        if (strcmp(token.lexema, ">") == 0)  token.attribute.op_code = OP_GT;
        if (strcmp(token.lexema, ">=") == 0) token.attribute.op_code = OP_GE;
        if (strcmp(token.lexema, "<>") == 0) token.attribute.op_code = OP_NE;
    }

    return token;
}

TokenNome classificarLexema(char *lexema)
{
    int i;

    for (i = 0; i < (int) QTD_PALAVRAS; i++) {
        if (strcmp(lexema, palavrasReservadas[i]) == 0)
            return TOKEN_KEYWORD;
    }

    // "<>" incluido aqui -- bug corrigido, ver README secao 4.5
    if (strcmp(lexema, "<") == 0 || strcmp(lexema, "<=") == 0 ||
        strcmp(lexema, "=") == 0 || strcmp(lexema, ">") == 0 ||
        strcmp(lexema, ">=") == 0 || strcmp(lexema, "<>") == 0)
        return TOKEN_OP_REL;

    for (i = 0; i < (int) QTD_SIMBOLOS; i++) {
        if (strcmp(lexema, simbolos[i]) == 0)
            return TOKEN_SIMBOLO;
    }

    if (lexema[0] == '"')
        return TOKEN_STRING;

    if (ehDigito(lexema[0])) {
        if (strchr(lexema, '.') != NULL)
            return TOKEN_NUM_FLOAT;
        return TOKEN_NUM_INT;
    }

    return TOKEN_ID;
}

void imprimirToken(Token token)
{
    char atributo[TAM_LEXEMA + 20];

    if (token.type == TOKEN_ID) {
        sprintf(atributo, "%d (%s)", token.attribute.table_index, token.lexema);
    }
    else if (token.type == TOKEN_NUM_INT) {
        sprintf(atributo, "%d", token.attribute.int_value);
    }
    else if (token.type == TOKEN_NUM_FLOAT) {
        sprintf(atributo, "%g", token.attribute.float_value);
    }
    else if (token.type == TOKEN_OP_REL) {
        strcpy(atributo, nomesOpRel[token.attribute.op_code]);
    }
    else {
        strcpy(atributo, token.lexema);
    }

    printf("%d# %s | %s\n", token.line, nomesTokens[token.type], atributo);
    fprintf(saida, "%d# %s | %s\n", token.line, nomesTokens[token.type], atributo);
}

int ehDelimitador(int c)
{
    return c == '(' || c == ')' || c == '[' || c == ']' ||
           c == ':' || c == ',' || c == '.';
}

int ehOperador(int c)
{
    return c == '<' || c == '>' || c == '=' ||
           c == '+' || c == '-' || c == '*' || c == '/' || c == '\\';
}

int ehLetra(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int ehDigito(int c)
{
    return c >= '0' && c <= '9';
}

int peek()
{
    int c = fgetc(fonte);
    if (c != EOF) {
        voltar();
    }
    return c;
}

void voltar()
{
    fseek(fonte, -1, SEEK_CUR);
}

void pularEspacosEComentarios()
{
    int c;

    while (1) {
        c = fgetc(fonte);

        if (c == '\n') {
            linhaAtual++;
        }
        else if (c == ' ' || c == '\t' || c == '\r') {
            // so ignora
        }
        else if (c == '/' && peek() == '/') {
            while (c != '\n' && c != EOF) {
                c = fgetc(fonte);
            }
            if (c == '\n') {
                linhaAtual++;
            }
        }
        else {
            if (c != EOF) {
                voltar();
            }
            break;
        }
    }
}

void guardar(Token *token, int *i, int c)
{
    if (*i >= TAM_LEXEMA - 1) {
        token->lexema[*i] = '\0';
        erroLexico(token->lexema, "lexema muito grande");
    }
    token->lexema[*i] = (char) c;
    (*i)++;
}

int inserirSimbolo(char *nome)
{
    int i;

    for (i = 0; i < qtdSimbolos; i++) {
        if (strcmp(tabelaSimbolos[i], nome) == 0) {
            return i;
        }
    }

    if (qtdSimbolos >= MAX_SIMBOLOS) {
        erroLexico(nome, "tabela de simbolos cheia");
    }

    strcpy(tabelaSimbolos[qtdSimbolos], nome);
    qtdSimbolos++;
    return qtdSimbolos - 1;
}

noreturn void erroLexico(char *sequencia, char *motivo)
{
    printf("ERRO LEXICO | linha %d | sequencia '%s' | %s\n", linhaAtual, sequencia, motivo);
    fprintf(saida, "ERRO LEXICO | linha %d | sequencia '%s' | %s\n", linhaAtual, sequencia, motivo);
    fecharAnalisador();
    exit(0);
}

void fecharAnalisador()
{
    fclose(fonte);
    fclose(saida);
}

// Analisador sintatico (Etapa 3)

// nextToken(): pede o proximo token ao lexico e ja o imprime (Etapa 2)
void avancar(void)
{
    tokenAtual = obterToken();
    imprimirToken(tokenAtual);
}

// compara o lexema atual com um texto fixo (palavras reservadas e simbolos)
int tokenEh(const char *lexema)
{
    return strcmp(tokenAtual.lexema, lexema) == 0;
}

// confere e consome um token pelo lexema; se nao bater, erro sintatico
void casar(const char *lexemaEsperado)
{
    if (tokenEh(lexemaEsperado)) {
        avancar();
    } else {
        erroSintatico(tokenAtual.lexema, tokenAtual.lexema);
    }
}

// igual a casar(), mas conferindo o TIPO do token (id, numero, cadeia...)
void casarTipo(TokenNome tipoEsperado, const char *descricao)
{
    if (tokenAtual.type == tipoEsperado) {
        avancar();
    } else {
        erroSintatico(tokenAtual.lexema, (char *) descricao);
    }
}

// FIRST(comando): tokens que podem iniciar um comando
int inicioDeComando(void)
{
    return tokenAtual.type == TOKEN_ID ||
           tokenEh("se") || tokenEh("para") || tokenEh("enquanto") ||
           tokenEh("leia") || tokenEh("escreva") || tokenEh("escreval") ||
           tokenEh("retorne");
}

noreturn void erroSintatico(char *tokenEncontrado, char *motivo)
{
    printf("ERRO SINTATICO | linha %d | token '%s' | motivo: %s\n", tokenAtual.line, tokenEncontrado, motivo);
    fprintf(saida, "ERRO SINTATICO | linha %d | token '%s' | motivo: %s\n", tokenAtual.line, tokenEncontrado, motivo);
    fecharAnalisador();
    exit(0);
}

//  programa / subprogramas / declaracoes

// Estrutura do codigo MEPA gerado (ver README 6.5), como na aula:
//      INPP
//      AMEM n          <- escritos em gravarCodigoMepa(), no fim
//      ... comandos do programa principal ...
//      PARA
// As rotinas nao aparecem separadas: o corpo delas e copiado em cada
// chamada (expandirRotina).
// No modo completo (README 6.8) as rotinas ficam no comeco do codigo:
//      DSVS Lp ; <rotinas> ; Lp: NADA ; <programa principal>
void parsePrograma(void)
{
    if (modoCompleto) {
        rotuloPrincipal = proximo_rotulo();
        emitirDesvio("DSVS", rotuloPrincipal);
    }

    casar("algoritmo");
    casarTipo(TOKEN_STRING, "esperado o nome do algoritmo entre aspas");
    parseListaSubprogramas();
    parseSecaoVar();
    casar("inicio");
    if (modoCompleto) {
        emitirRotulo(rotuloPrincipal);
    }
    parseListaComandos();
    casar("fimalgoritmo");

    verificarVariaveisNaoUsadas();
}

void parseListaSubprogramas(void)
{
    while (tokenEh("procedimento") || tokenEh("funcao")) {
        parseSubprograma();
    }
}

void parseSubprograma(void)
{
    if (tokenEh("procedimento")) {
        parseProcedimento();
    } else {
        parseFuncao();
    }
}

// Rotinas: o codigo do corpo NAO vai para o programa principal; ele e
// guardado num "molde" (codigoRotina[idx]) que sera copiado em cada
// chamada. O molde termina com o rotulo de fim, para onde o retorne
// desvia:
//      <corpo>
//  L#fim: NADA
// No modo completo (README 6.8) o corpo vai direto para o codigo:
//  Lk: ENPR 1 ; <corpo> ; Lfim: NADA ; RTPR 1, <qtd parametros>
void parseProcedimento(void)
{
    int idx;

    casar("procedimento");
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    casarTipo(TOKEN_ID, "esperado o nome do procedimento");
    iniciarRotina(idx, CAT_PROC);
    parseParametrosOpc();

    if (modoCompleto) {
        emitirRotuloInstrucao(rotuloRotina[idx], "ENPR 1");
    }
    casar("inicio");
    parseListaComandos();
    casar("fimprocedimento");
    emitirRotulo(rotuloFimRotina);
    if (modoCompleto) {
        emitirInt("RTPR 1,", qtdParams[idx]);
    }

    encerrarRotina();
}

void parseFuncao(void)
{
    int idx;

    casar("funcao");
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    casarTipo(TOKEN_ID, "esperado o nome da funcao");
    iniciarRotina(idx, CAT_FUNC);
    parseParametrosOpc();
    casar(":");
    tipoBase[idx] = parseTipoBase();         // tipo de retorno da funcao

    if (modoCompleto) {
        emitirRotuloInstrucao(rotuloRotina[idx], "ENPR 1");
    }
    casar("inicio");
    funcaoTemRetorne = 0;
    parseListaComandos();
    if (!funcaoTemRetorne) {
        erroSemantico(tabelaSimbolos[idx], "funcao sem nenhum comando retorne");
    }
    casar("fimfuncao");
    emitirRotulo(rotuloFimRotina);
    if (modoCompleto) {
        emitirInt("RTPR 1,", qtdParams[idx]);
    }

    encerrarRotina();
}

// parenteses so aparecem se houver parametros (ver README secao 3.4)
void parseParametrosOpc(void)
{
    if (tokenEh("(")) {
        casar("(");
        if (!tokenEh(")")) {
            parseListaParametros();
        }
        casar(")");
    }
}

void parseListaParametros(void)
{
    parseParametro();
    while (tokenEh(",")) {
        casar(",");
        parseParametro();
    }
}

// Declara o parametro (CAT_PARAM) e registra seu tipo na assinatura
// da rotina em analise (qtdParams / tipoParam), conferida em parseArgumentos().
void parseParametro(void)
{
    int idx;
    int n = qtdParams[rotinaAtual];

    if (tokenAtual.type == TOKEN_KEYWORD) {
        erroSemantico(tokenAtual.lexema, "palavra reservada nao pode ser usada como nome de parametro");
    }
    if (n >= MAX_PARAMS) {
        erroSemantico(tabelaSimbolos[rotinaAtual], "rotina com parametros demais");
    }
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    casarTipo(TOKEN_ID, "esperado nome de parametro");
    casar(":");
    declarar(idx, CAT_PARAM, parseTipoBase());

    // modo aula: o parametro mora numa posicao escondida so dele (6.5.5)
    // modo completo: endereco = posicao do parametro na chamada (6.8)
    if (modoCompleto) {
        endereco[idx] = n;
    } else {
        endereco[idx] = proxOculta;
        proxOculta++;
    }
    paramDaRotina[rotinaAtual][n] = idx;
    posicaoParam[rotinaAtual][n] = endereco[idx];
    tipoParam[rotinaAtual][n] = tipoBase[idx];
    qtdParams[rotinaAtual] = n + 1;
}

void parseSecaoVar(void)
{
    if (tokenEh("var")) {
        casar("var");
        parseListaDeclaracoes();
    }
}

// Declaracoes da secao var.
// O laco tambem entra quando o nome e uma palavra reservada (exceto
// "inicio", que fecha a secao), para que parseListaIds() possa
// acusar o erro semantico em vez de um erro sintatico (README 6.2.2).
void parseListaDeclaracoes(void)
{
    while (tokenAtual.type == TOKEN_ID ||
           (tokenAtual.type == TOKEN_KEYWORD && !tokenEh("inicio"))) {
        parseDeclaracao();
    }
}

// decl_lista -> id_lista ':' tipo
// parseListaIds() ja registra cada nome na tabela (para acusar nome
// repetido na linha certa). Como o tipo so aparece depois do ':',
// aqui completamos o tipo e os enderecos de cada nome da lista.
void parseDeclaracao(void)
{
    int ids[MAX_SIMBOLOS];             // indices (tabelaSimbolos) dos nomes
    int qtdIds = 0;
    int enderecoInicial = proxEndereco;
    int ehVetor = 0;
    int limiteInf = 0;
    int limiteSup = 0;
    int tamanho;                       // posicoes de memoria por nome
    TipoBase tipo;
    int i;

    parseListaIds(ids, &qtdIds);
    casar(":");
    tipo = parseTipo(&ehVetor, &limiteInf, &limiteSup);

    tamanho = ehVetor ? (limiteSup - limiteInf + 1) : 1;

    for (i = 0; i < qtdIds; i++) {
        tipoBase[ids[i]] = tipo;
        endereco[ids[i]] = enderecoInicial + i * tamanho;
        if (ehVetor) {
            categoria[ids[i]] = CAT_VETOR;
            limInf[ids[i]] = limiteInf;
            limSup[ids[i]] = limiteSup;
        }
    }
    proxEndereco = enderecoInicial + qtdIds * tamanho;
}

// id_lista -> id (',' id)*
// Cada nome e declarado assim que e lido (tipo ainda desconhecido,
// T_NENHUM), e seu indice e guardado em ids[] para parseDeclaracao()
// completar o tipo. O indice tem que ser lido ANTES de casarTipo(),
// porque avancar() troca o tokenAtual.
void parseListaIds(int ids[], int *qtdIds)
{
    int idx;

    *qtdIds = 0;
    do {
        if (*qtdIds > 0) {
            casar(",");
        }
        if (tokenAtual.type == TOKEN_KEYWORD) {
            erroSemantico(tokenAtual.lexema, "palavra reservada nao pode ser usada como nome de variavel");
        }
        idx = tokenAtual.attribute.table_index;
        if (tokenAtual.type == TOKEN_ID) {
            declarar(idx, CAT_VAR, T_NENHUM);  // erro se o nome ja existe
        }
        casarTipo(TOKEN_ID, "esperado identificador");
        ids[*qtdIds] = idx;
        (*qtdIds)++;
    } while (tokenEh(","));
}

// tipo -> tipo_base | vetor '[' num_int '..' num_int ']' de tipo_base
// Devolve o tipo base. Se for vetor, *ehVetor = 1 e os limites saem
// em *limiteInf e *limiteSup.
TipoBase parseTipo(int *ehVetor, int *limiteInf, int *limiteSup)
{
    *ehVetor = 0;

    if (tokenEh("vetor")) {
        casar("vetor");
        casar("[");
        *limiteInf = tokenAtual.attribute.int_value;  // ler antes de consumir
        casarTipo(TOKEN_NUM_INT, "esperado numero inteiro (limite inferior do vetor)");
        casar("..");
        *limiteSup = tokenAtual.attribute.int_value;
        casarTipo(TOKEN_NUM_INT, "esperado numero inteiro (limite superior do vetor)");
        casar("]");
        casar("de");

        if (*limiteInf > *limiteSup) {
            erroSemantico("vetor", "limite inferior maior que o limite superior");
        }
        *ehVetor = 1;
    }

    return parseTipoBase();
}

// tipo_base -> inteiro | real | caractere | logico
TipoBase parseTipoBase(void)
{
    TipoBase tipo = T_NENHUM;

    if (tokenEh("inteiro"))        tipo = T_INT;
    else if (tokenEh("real"))      tipo = T_REAL;
    else if (tokenEh("caractere")) tipo = T_CHAR;
    else if (tokenEh("logico"))    tipo = T_LOG;
    else {
        erroSintatico(tokenAtual.lexema, "esperado um tipo (inteiro, real, caractere ou logico)");
    }

    avancar();
    return tipo;
}

//comandos

void parseListaComandos(void)
{
    while (inicioDeComando()) {
        parseComando();
    }
}

void parseComando(void)
{
    if (tokenAtual.type == TOKEN_ID) {
        parseAtribuicaoOuChamada();
    }
    else if (tokenEh("se")) {
        parseComandoSe();
    }
    else if (tokenEh("para")) {
        parseComandoPara();
    }
    else if (tokenEh("enquanto")) {
        parseComandoEnquanto();
    }
    else if (tokenEh("leia")) {
        parseComandoLeia();
    }
    else if (tokenEh("escreva") || tokenEh("escreval")) {
        parseComandoEscreva();
    }
    else if (tokenEh("retorne")) {
        parseComandoRetorne();
    }
    else {
        erroSintatico(tokenAtual.lexema, "token nao inicia nenhum comando valido");
    }
}

// depois do id, o proximo token decide entre atribuicao (simples ou
// em vetor) e chamada (com ou sem argumentos) ver README secao 4.4
//
// Codigo gerado:
//   x <- expr      =>  <expr> ; ARMZ x
//   v[i] <- expr   =>  <i> ; <expr> ; (gravacao em vetor, emitirEscritaVetor)
//   proc(args)     =>  <args> ; (copia do corpo, expandirRotina)
void parseAtribuicaoOuChamada(void)
{
    int idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    int linha = tokenAtual.line;
    TipoBase tipoExpr;

    casarTipo(TOKEN_ID, "esperado identificador");

    if (tokenEh("[")) {
        // atribuicao em posicao de vetor
        usarComoVariavel(idx, 1, linha);
        parseIndiceVetor(idx);      // deixa o indice (ou endereco) na pilha
        if (!tokenEh("<-")) {
            erroSintatico(tokenAtual.lexema, "esperado '<-' depois do indice do vetor");
        }
        casar("<-");
        tipoExpr = parseExpressao();
        if (!podeAtribuir(tipoBase[idx], tipoExpr)) {
            erroSemanticoNaLinha(linha, tabelaSimbolos[idx],
                                 "tipo da expressao incompativel com o tipo do vetor");
        }
        atribuida[idx] = 1;
        emitirEscritaVetor(idx);
    }
    else if (tokenEh("<-")) {
        // atribuicao simples
        usarComoVariavel(idx, 0, linha);
        casar("<-");
        tipoExpr = parseExpressao();
        if (!podeAtribuir(tipoBase[idx], tipoExpr)) {
            erroSemanticoNaLinha(linha, tabelaSimbolos[idx],
                                 "tipo da expressao incompativel com o tipo da variavel");
        }
        atribuida[idx] = 1;
        emitirVariavel("ARMZ", idx);
    }
    else {
        // chamada de procedimento, com ou sem argumentos
        usarComoRotina(idx, linha);
        if (categoria[idx] == CAT_FUNC) {
            erroSemanticoNaLinha(linha, tabelaSimbolos[idx],
                                 "funcao nao pode ser usada como procedimento (use em expressao)");
        }
        parseArgumentos(idx, linha);
        expandirRotina(idx, linha);
    }
}

// Le os argumentos de uma chamada (se houver) e confere quantidade e
// tipo contra a assinatura da rotina. Cada argumento fica na pilha,
// na ordem, que e onde a rotina espera encontrar seus parametros.
void parseArgumentos(int idxRotina, int linha)
{
    int qtd = 0;
    TipoBase tipoArg;

    if (tokenEh("(")) {
        casar("(");
        if (!tokenEh(")")) {
            do {
                if (qtd > 0) {
                    casar(",");
                }
                tipoArg = parseExpressao();
                if (qtd >= qtdParams[idxRotina]) {
                    erroSemanticoNaLinha(linha, tabelaSimbolos[idxRotina], "chamada com argumentos demais");
                }
                if (!podeAtribuir(tipoParam[idxRotina][qtd], tipoArg)) {
                    erroSemanticoNaLinha(linha, tabelaSimbolos[idxRotina],
                                         "tipo de argumento incompativel com o parametro");
                }
                qtd++;
            } while (tokenEh(","));
        }
        casar(")");
    }

    if (qtd < qtdParams[idxRotina]) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idxRotina], "chamada com argumentos de menos");
    }
}

// se (cond) entao A senao B fimse
//      <cond> ; DSVF Lsenao ; A ; DSVS Lfim ;
//      Lsenao: NADA ; B ; Lfim: NADA       (mesmo padrao do "if" da aula)
// Os dois rotulos sao pegos ANTES de analisar os comandos internos,
// porque eles tambem chamam proximo_rotulo() (ver enunciado).
void parseComandoSe(void)
{
    int rotuloSenao = proximo_rotulo();
    int rotuloFim = proximo_rotulo();
    int linha;

    casar("se");
    casar("(");
    linha = tokenAtual.line;
    exigirTipo(parseExpressao(), T_LOG, linha, "condicao do se");
    casar(")");
    emitirDesvio("DSVF", rotuloSenao);
    casar("entao");
    parseListaComandos();
    if (tokenEh("senao")) {
        emitirDesvio("DSVS", rotuloFim);
        emitirRotulo(rotuloSenao);
        casar("senao");
        parseListaComandos();
    } else {
        emitirRotulo(rotuloSenao);
    }
    casar("fimse");
    emitirRotulo(rotuloFim);
}

// para i de A ate B [passo k] faca C fimpara
//      <A> ; ARMZ i ;
//      Lteste: NADA ; CRVL i ; <B> ; CMEG (ou CMAG se k < 0) ; DSVF Lfim ;
//      C ; CRVL i ; CRCT k ; SOMA ; ARMZ i ; DSVS Lteste ;
//      Lfim: NADA                          (mesmo padrao do "for" da aula)
void parseComandoPara(void)
{
    int rotuloTeste = proximo_rotulo();
    int rotuloFim = proximo_rotulo();
    int idx;
    int linha;
    int passo = 1;

    casar("para");
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    linha = tokenAtual.line;
    casarTipo(TOKEN_ID, "esperado identificador da variavel de controle");
    usarComoVariavel(idx, 0, linha);
    exigirTipo(tipoBase[idx], T_INT, linha, "variavel de controle do para");
    atribuida[idx] = 1;  // o para atribui valores a variavel de controle
    usada[idx] = 1;      // e tambem a le a cada volta

    casar("de");
    exigirTipo(parseExpressao(), T_INT, linha, "valor inicial do para");
    emitirVariavel("ARMZ", idx);

    emitirRotulo(rotuloTeste);
    emitirVariavel("CRVL", idx);
    casar("ate");
    exigirTipo(parseExpressao(), T_INT, linha, "valor final do para");

    if (tokenEh("passo")) {
        casar("passo");
        passo = parsePasso();
    }
    emitir(passo > 0 ? "CMEG" : "CMAG");
    emitirDesvio("DSVF", rotuloFim);

    casar("faca");
    parseListaComandos();
    casar("fimpara");

    emitirVariavel("CRVL", idx);
    emitirInt("CRCT", passo);
    emitir("SOMA");
    emitirVariavel("ARMZ", idx);
    emitirDesvio("DSVS", rotuloTeste);
    emitirRotulo(rotuloFim);
}

// O passo precisa ser uma constante inteira (com ou sem '-'), porque o
// sinal dele decide se o teste do laco e "<=" ou ">=" (README 6.3).
int parsePasso(void)
{
    int sinal = 1;
    int valor;

    if (tokenEh("-")) {
        casar("-");
        sinal = -1;
    }
    if (tokenAtual.type != TOKEN_NUM_INT) {
        erroSemantico(tokenAtual.lexema, "o passo do para deve ser uma constante inteira");
    }
    valor = sinal * tokenAtual.attribute.int_value;  // ler antes de consumir
    if (valor == 0) {
        erroSemantico(tokenAtual.lexema, "o passo do para nao pode ser zero");
    }
    casarTipo(TOKEN_NUM_INT, "esperado numero inteiro");
    return valor;
}

// enquanto (cond) faca C fimenquanto
//      Linicio: NADA ; <cond> ; DSVF Lfim ; C ; DSVS Linicio ; Lfim: NADA
//      (mesmo padrao do "while" da aula)
void parseComandoEnquanto(void)
{
    int rotuloInicio = proximo_rotulo();
    int rotuloFim = proximo_rotulo();
    int linha;

    casar("enquanto");
    emitirRotulo(rotuloInicio);
    casar("(");
    linha = tokenAtual.line;
    exigirTipo(parseExpressao(), T_LOG, linha, "condicao do enquanto");
    casar(")");
    emitirDesvio("DSVF", rotuloFim);
    casar("faca");
    parseListaComandos();
    casar("fimenquanto");
    emitirDesvio("DSVS", rotuloInicio);
    emitirRotulo(rotuloFim);
}

void parseComandoLeia(void)
{
    casar("leia");
    casar("(");
    parseVariavelLeia();
    while (tokenEh(",")) {
        casar(",");
        parseVariavelLeia();
    }
    casar(")");
}

// Uma variavel dentro do leia(...): simples ou posicao de vetor.
// Precisa estar declarada, e passa a contar como atribuida.
//   leia(x)    =>  LEIT ; ARMZ x
//   leia(v[i]) =>  <i> ; LEIT ; (gravacao em vetor, emitirEscritaVetor)
void parseVariavelLeia(void)
{
    int idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    int linha = tokenAtual.line;

    casarTipo(TOKEN_ID, "esperado identificador");
    if (tokenEh("[")) {
        usarComoVariavel(idx, 1, linha);
        parseIndiceVetor(idx);
        emitir("LEIT");
        emitirEscritaVetor(idx);
    } else {
        usarComoVariavel(idx, 0, linha);
        emitir("LEIT");
        emitirVariavel("ARMZ", idx);
    }
    atribuida[idx] = 1;
}

// escreva(e1, e2, ...) => para cada expressao: <e> ; IMPR
// A maquina MEPA so imprime numeros inteiros, entao uma cadeia
// sozinha ("texto") e aceita pela analise mas nao gera codigo
// (README 6.5).
void parseComandoEscreva(void)
{
    if (tokenEh("escreva")) {
        casar("escreva");
    } else {
        casar("escreval");
    }
    casar("(");
    if (!tokenEh(")")) {
        do {
            if (tokenEh(",")) {
                casar(",");
            }
            if (tokenAtual.type == TOKEN_STRING) {
                aviso(tokenAtual.line, "cadeia no escreva nao e impressa (IMPR da MEPA so imprime numero)");
                casarTipo(TOKEN_STRING, "esperado cadeia");
                if (!tokenEh(",") && !tokenEh(")")) {
                    erroSintatico(tokenAtual.lexema, "esperado ',' ou ')' depois da cadeia");
                }
            } else {
                parseExpressao();
                emitir("IMPR");
            }
        } while (tokenEh(","));
    }
    casar(")");
}

// retorne expr  =>  <expr> ; ARMZ <resultado da funcao> ; DSVS Lfim
void parseComandoRetorne(void)
{
    int linha = tokenAtual.line;
    TipoBase tipoExpr;

    casar("retorne");
    if (rotinaAtual == -1 || categoria[rotinaAtual] != CAT_FUNC) {
        erroSemanticoNaLinha(linha, "retorne", "retorne so pode ser usado dentro de funcao");
    }
    tipoExpr = parseExpressao();
    if (!podeAtribuir(tipoBase[rotinaAtual], tipoExpr)) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[rotinaAtual],
                             "tipo do retorno incompativel com o declarado na funcao");
    }
    funcaoTemRetorne = 1;
    emitirRetornoFuncao(rotinaAtual);
    emitirDesvio("DSVS", rotuloFimRotina);
}

// expressoes
// precedencia (menor pra maior): OU < E < relacional < + - < * / \ MOD
// (ver README secao 4.3)
//
// Fase 2: cada funcao devolve o TIPO da expressao que reconheceu e
// gera o codigo MEPA que deixa o valor dela no topo da pilha. O
// operador e emitido depois dos dois operandos (notacao pos-fixa), que
// e exatamente o que uma maquina de pilha precisa (README 6.5).

TipoBase parseExpressao(void)
{
    TipoBase tipo = parseExprE();
    int linha;

    while (tokenEh("OU")) {
        linha = tokenAtual.line;
        casar("OU");
        exigirTipo(tipo, T_LOG, linha, "operando do OU");
        exigirTipo(parseExprE(), T_LOG, linha, "operando do OU");
        emitir("DISJ");
        tipo = T_LOG;
    }
    return tipo;
}

TipoBase parseExprE(void)
{
    TipoBase tipo = parseExprRel();
    int linha;

    while (tokenEh("E")) {
        linha = tokenAtual.line;
        casar("E");
        exigirTipo(tipo, T_LOG, linha, "operando do E");
        exigirTipo(parseExprRel(), T_LOG, linha, "operando do E");
        emitir("CONJ");
        tipo = T_LOG;
    }
    return tipo;
}

// "if" e nao "while": no maximo um operador relacional por expressao
// Compatibilidade: numero com numero (inteiro e real se misturam),
// caractere com caractere e logico com logico (este so com = e <>).
TipoBase parseExprRel(void)
{
    TipoBase esq = parseExprArit();
    TipoBase dir;
    OpRelAtributo op;
    int linha;
    int compativel;
    // na mesma ordem do enum OpRelAtributo: <  <=  =  >  >=  <>
    const char *instrucoes[] = { "CMME", "CMEG", "CMIG", "CMMA", "CMAG", "CMDG" };
    const char *textoOperador[] = { "<", "<=", "=", ">", ">=", "<>" };

    if (tokenAtual.type != TOKEN_OP_REL) {
        return esq;
    }

    op = tokenAtual.attribute.op_code;  // ler antes de consumir
    linha = tokenAtual.line;
    avancar();
    dir = parseExprArit();

    compativel = (ehNumerico(esq) && ehNumerico(dir)) ||
                 (esq == T_CHAR && dir == T_CHAR) ||
                 (esq == T_LOG && dir == T_LOG && (op == OP_EQ || op == OP_NE));
    if (!compativel) {
        erroSemanticoNaLinha(linha, (char *) textoOperador[op], "comparacao entre tipos incompativeis");
    }

    emitir(instrucoes[op]);
    return T_LOG;
}

TipoBase parseExprArit(void)
{
    TipoBase tipo = parseTermo();
    char operador[TAM_LEXEMA];
    int linha;

    while (tokenEh("+") || tokenEh("-")) {
        strcpy(operador, tokenAtual.lexema);
        linha = tokenAtual.line;
        avancar();
        tipo = tipoAritmetico(tipo, parseTermo(), linha, operador);
        emitir(operador[0] == '+' ? "SOMA" : "SUBT");
    }
    return tipo;
}

// '/' da resultado real (como no VisuAlg) e vira DIVF, a divisao real
// da aula; '\' e MOD so com inteiros.
// A MEPA nao tem MOD, entao a MOD b vira a - (a \ b) * b, usando as
// duas posicoes temporarias para poder ler a e b mais de uma vez.
TipoBase parseTermo(void)
{
    TipoBase tipo = parseFator();
    TipoBase dir;
    char operador[TAM_LEXEMA];
    int linha;

    while (tokenEh("*") || tokenEh("/") || tokenEh("\\") || tokenEh("MOD")) {
        strcpy(operador, tokenAtual.lexema);
        linha = tokenAtual.line;
        avancar();
        dir = parseFator();

        if (strcmp(operador, "*") == 0) {
            tipo = tipoAritmetico(tipo, dir, linha, operador);
            emitir("MULT");
        }
        else if (strcmp(operador, "/") == 0) {
            tipoAritmetico(tipo, dir, linha, operador);
            tipo = T_REAL;
            aviso(linha, "'/' e divisao real, mas na execucao vira divisao inteira (DIVI)");
            emitirDivisaoReal();
        }
        else {
            exigirTipo(tipo, T_INT, linha, operador);
            exigirTipo(dir, T_INT, linha, operador);
            tipo = T_INT;
            if (strcmp(operador, "\\") == 0) {
                emitir("DIVI");
            } else {
                emitirTemporaria("ARMZ", TMP_VALOR);   // b
                emitirTemporaria("ARMZ", TMP_INDICE);  // a
                emitirTemporaria("CRVL", TMP_INDICE);
                emitirTemporaria("CRVL", TMP_INDICE);
                emitirTemporaria("CRVL", TMP_VALOR);
                emitir("DIVI");
                emitirTemporaria("CRVL", TMP_VALOR);
                emitir("MULT");
                emitir("SUBT");
            }
        }
    }
    return tipo;
}

TipoBase parseFator(void)
{
    TipoBase tipo = T_NENHUM;
    int codigo;
    int linha = tokenAtual.line;

    if (tokenEh("(")) {
        casar("(");
        tipo = parseExpressao();
        casar(")");
    }
    else if (tokenEh("-")) {
        casar("-");
        tipo = parseFator(); // permite "- - x"
        if (!ehNumerico(tipo)) {
            erroSemanticoNaLinha(linha, "-", "menos unario so pode ser usado com numero");
        }
        emitir("INVR");
    }
    else if (tokenAtual.type == TOKEN_ID) {
        tipo = parseIdentificadorEmExpressao();
    }
    else if (tokenAtual.type == TOKEN_NUM_INT) {
        emitirInt("CRCT", tokenAtual.attribute.int_value);
        casarTipo(TOKEN_NUM_INT, "esperado numero inteiro");
        tipo = T_INT;
    }
    else if (tokenAtual.type == TOKEN_NUM_FLOAT) {
        // CRCT so carrega numero inteiro: a parte fracionaria e perdida
        if (tokenAtual.attribute.float_value != (int) tokenAtual.attribute.float_value) {
            aviso(linha, "numero real truncado para inteiro (a MEPA so trabalha com inteiros)");
        }
        emitirInt("CRCT", (int) tokenAtual.attribute.float_value);
        casarTipo(TOKEN_NUM_FLOAT, "esperado numero real");
        tipo = T_REAL;
    }
    else if (tokenAtual.type == TOKEN_STRING) {
        // caractere na MEPA = codigo ASCII do primeiro caractere da cadeia
        // (o lexema guarda as aspas, entao o primeiro caractere e lexema[1])
        codigo = (tokenAtual.lexema[1] == '"') ? 0 : (unsigned char) tokenAtual.lexema[1];
        aviso(linha, "cadeia em expressao vira o codigo ASCII do primeiro caractere");
        emitirInt("CRCT", codigo);
        casarTipo(TOKEN_STRING, "esperado cadeia");
        tipo = T_CHAR;
    }
    else if (tokenEh("verdadeiro") || tokenEh("falso")) {
        emitirInt("CRCT", tokenEh("verdadeiro") ? 1 : 0);
        avancar();
        tipo = T_LOG;
    }
    else {
        erroSintatico(tokenAtual.lexema, "esperado um fator valido (identificador, numero, cadeia, 'verdadeiro', 'falso', '(' ou '-')");
    }
    return tipo;
}

// Identificador dentro de expressao: variavel, posicao de vetor ou
// chamada de funcao.
//   x       =>  CRVL x
//   v[i]    =>  <i> ; (leitura de vetor, emitirLeituraVetor)
//   f(args) =>  <args> ; (copia do corpo, expandirRotina) ; CRVL f
TipoBase parseIdentificadorEmExpressao(void)
{
    int idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    int linha = tokenAtual.line;

    casarTipo(TOKEN_ID, "esperado identificador");

    if (tokenEh("[")) {
        usarComoVariavel(idx, 1, linha);
        usada[idx] = 1;
        parseIndiceVetor(idx);
        emitirLeituraVetor(idx);
    }
    else if (tokenEh("(")) {
        usarComoRotina(idx, linha);
        if (categoria[idx] == CAT_PROC) {
            erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "procedimento nao pode ser usado em expressao");
        }
        if (modoCompleto) {
            emitirInt("AMEM", 1);  // espaco do resultado, antes dos argumentos
        }
        parseArgumentos(idx, linha);
        expandirRotina(idx, linha);
    }
    else {
        usarComoVariavel(idx, 0, linha);
        usada[idx] = 1;
        emitirVariavel("CRVL", idx);
    }
    return tipoBase[idx];
}

// Le "[ indice ]" depois do nome de um vetor (o indice tem que ser
// inteiro) e deixa na pilha:
//   modo aula:     o valor do indice (o acesso e feito por comparacao)
//   modo completo: o endereco da posicao: CREN v ; <i> ; CRCT limInf ; SUBT ; SOMA
void parseIndiceVetor(int idxVetor)
{
    int linha;

    if (modoCompleto) {
        emitirVariavel("CREN", idxVetor);
    }
    casar("[");
    linha = tokenAtual.line;
    exigirTipo(parseExpressao(), T_INT, linha, "indice do vetor");
    casar("]");
    if (modoCompleto) {
        emitirInt("CRCT", limInf[idxVetor]);
        emitir("SUBT");
        emitir("SOMA");
    }
}

// Analisador semantico (Fase 2)
// Tabela de simbolos = matriz tabelaSimbolos[][] + vetores paralelos
// (ver README secao 6.1)

// Erro semantico na linha do token atual.
noreturn void erroSemantico(char *identificador, char *motivo)
{
    erroSemanticoNaLinha(tokenAtual.line, identificador, motivo);
}

// Erro semantico numa linha informada. Usada quando o erro so e
// percebido depois de o parser ja ter avancado para o proximo token
// (ex.: "x <- v" com v vetor: o erro e da linha do v). Mostra a
// mensagem na tela e no arquivo de saida e encerra com exit(0).
noreturn void erroSemanticoNaLinha(int linha, char *identificador, char *motivo)
{
    printf("ERRO SEMANTICO | linha %d | sequencia '%s' | %s\n", linha, identificador, motivo);
    fprintf(saida, "ERRO SEMANTICO | linha %d | sequencia '%s' | %s\n", linha, identificador, motivo);
    fecharAnalisador();
    exit(0);
}

// Declara o identificador da posicao idx da tabela.
// Erro semantico se o nome ja tiver sido declarado (nome unico).
// Variaveis e vetores recebem o proximo endereco livre (ordem de
// declaracao). Parametros ganham uma posicao escondida em
// parseParametro(); rotinas nao ocupam memoria (endereco -1).
void declarar(int idx, Categoria cat, TipoBase tipo)
{
    if (declarada[idx]) {
        erroSemantico(tabelaSimbolos[idx], "identificador declarado mais de uma vez");
    }

    declarada[idx] = 1;
    categoria[idx] = cat;
    tipoBase[idx]  = tipo;
    linhaDecl[idx] = tokenAtual.line;
    atribuida[idx] = 0;
    usada[idx]     = 0;

    if (cat == CAT_VAR || cat == CAT_VETOR) {
        endereco[idx] = proxEndereco;
        proxEndereco++;
    } else {
        endereco[idx] = -1;
    }
}

// Funcao pedida no enunciado: recebe o lexema do ID e devolve o
// endereco da variavel. Se o identificador nao foi declarado,
// gera erro semantico e encerra a compilacao.
int busca_tabela_simbolos(char *id)
{
    int i;

    for (i = 0; i < qtdSimbolos; i++) {
        if (strcmp(tabelaSimbolos[i], id) == 0) {
            if (!declarada[i]) {
                break; // apareceu no texto, mas nunca foi declarado
            }
            return endereco[i];
        }
    }

    erroSemantico(id, "identificador nao declarado");
}

// Devolve o proximo rotulo da geracao de codigo MEPA: 1, 2, 3...
// Cuidado: como o parser e recursivo, guarde o retorno em variavel
// local ANTES de chamar outras funcoes parse* (ver enunciado).
int proximo_rotulo(void)
{
    contadorRotulos++;
    return contadorRotulos;
}

// Confere o uso de um identificador como VARIAVEL (lado esquerdo do
// <-, leia, para ou dentro de expressao).
//   temIndice = 1 quando aparece como nome[i]
//   linha     = linha onde o identificador apareceu
void usarComoVariavel(int idx, int temIndice, int linha)
{
    if (!declarada[idx]) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "identificador nao declarado");
    }
    if (categoria[idx] == CAT_PROC || categoria[idx] == CAT_FUNC) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "rotina usada como se fosse variavel");
    }
    if (temIndice && categoria[idx] != CAT_VETOR) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "indice usado em variavel que nao e vetor");
    }
    if (!temIndice && categoria[idx] == CAT_VETOR) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "vetor usado sem indice");
    }
}

// Confere o uso de um identificador como chamada de ROTINA.
void usarComoRotina(int idx, int linha)
{
    if (!declarada[idx]) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "rotina nao declarada");
    }
    if (categoria[idx] != CAT_PROC && categoria[idx] != CAT_FUNC) {
        erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "variavel usada como se fosse rotina");
    }
}

// Registra a rotina antes de ler os parametros e o corpo e marca-a
// como rotina em analise. A partir daqui o codigo gerado vai para o
// molde dela (destino), e os rotulos usados formam a faixa
// primeiroRotulo..ultimoRotulo, que sera renumerada a cada chamada.
void iniciarRotina(int idx, Categoria cat)
{
    declarar(idx, cat, T_NENHUM);  // funcao: tipo de retorno vem depois
    qtdParams[idx] = 0;
    rotinaAtual = idx;
    if (modoCompleto) {
        rotuloRotina[idx] = proximo_rotulo();
        rotuloFimRotina = proximo_rotulo();
        return;
    }
    destino = &codigoRotina[idx];
    primeiroRotulo[idx] = contadorRotulos + 1;
    rotuloFimRotina = proximo_rotulo();
    if (cat == CAT_FUNC) {
        posicaoResultado[idx] = proxOculta;
        proxOculta++;
    }
}

// Fim da rotina: os parametros saem de escopo (podem ser reusados em
// outra rotina ou como variavel global) e o codigo volta a ser gerado
// no programa principal.
void encerrarRotina(void)
{
    int i;

    for (i = 0; i < qtdParams[rotinaAtual]; i++) {
        declarada[paramDaRotina[rotinaAtual][i]] = 0;
    }
    ultimoRotulo[rotinaAtual] = contadorRotulos;
    rotinaAtual = -1;
    destino = &codigoPrincipal;
}

// Regras de tipos (README 6.3)

int ehNumerico(TipoBase tipo)
{
    return tipo == T_INT || tipo == T_REAL;
}

// Atribuicao, argumento e retorno: o tipo tem que ser o mesmo, com uma
// excecao: um inteiro pode ir para um real (o contrario perderia a
// parte fracionaria, entao e erro).
int podeAtribuir(TipoBase destino, TipoBase origem)
{
    return destino == origem || (destino == T_REAL && origem == T_INT);
}

// + - * : os dois lados precisam ser numeros; se um deles for real o
// resultado e real, senao e inteiro.
TipoBase tipoAritmetico(TipoBase esq, TipoBase dir, int linha, const char *operador)
{
    if (!ehNumerico(esq) || !ehNumerico(dir)) {
        erroSemanticoNaLinha(linha, (char *) operador, "operacao aritmetica com operando que nao e numero");
    }
    return (esq == T_REAL || dir == T_REAL) ? T_REAL : T_INT;
}

// Erro semantico se o tipo obtido nao for o esperado.
// Ex.: "condicao do se: esperado logico, encontrado inteiro"
void exigirTipo(TipoBase obtido, TipoBase esperado, int linha, const char *contexto)
{
    char mensagem[TAM_LINHA_MEPA];

    if (obtido != esperado) {
        snprintf(mensagem, sizeof(mensagem), "esperado %s, encontrado %s",
                 nomesTipos[esperado], nomesTipos[obtido]);
        erroSemanticoNaLinha(linha, (char *) contexto, mensagem);
    }
}

// Regra do enunciado: "toda variavel declarada e com valor atribuido a
// ela deve ser usada". Chamada no fim do programa, quando ja se sabe
// tudo o que aconteceu com cada variavel.
void verificarVariaveisNaoUsadas(void)
{
    int i;

    for (i = 0; i < qtdSimbolos; i++) {
        if (declarada[i] && (categoria[i] == CAT_VAR || categoria[i] == CAT_VETOR) &&
            atribuida[i] && !usada[i]) {
            erroSemanticoNaLinha(linhaDecl[i], tabelaSimbolos[i], "variavel recebe valor mas nunca e usada");
        }
    }
}

// Geracao de codigo MEPA (Fase 2) - ver README 6.5
//
// So usamos as instrucoes vistas na aula de MEPA:
//   CRCT CRVL ARMZ SOMA SUBT MULT DIVI DIVF INVR CONJ DISJ
//   CMME CMMA CMIG CMDG CMEG CMAG DSVS DSVF NADA AMEM LEIT IMPR INPP PARA
//
// O codigo e montado em memoria durante a analise e so vai para os
// arquivos no final, quando o programa foi aceito. Cada trecho de
// codigo e guardado em duas versoes (struct Codigo):
//   aula: "CRVL soma"  -> estilo da aula, variavel pelo nome (saida_mepa.txt)
//   exec: "CRVL 0, 2"  -> mesmo codigo no formato que o InterprMEPA le
//                         (saida_mepa_exec.txt)

// Apaga os arquivos MEPA de uma compilacao anterior, para que, se houver
// erro agora, nao sobre um arquivo velho que pareca ser deste programa.
void iniciarGeracao(void)
{
    remove(arqMepa);
    remove(arqMepaExec);
    destino = &codigoPrincipal;
}

// Aviso: algo que a analise aceita, mas que a MEPA nao consegue
// representar exatamente. NAO para a compilacao (README 6.5.7).
void aviso(int linha, const char *mensagem)
{
    printf("AVISO | linha %d | %s\n", linha, mensagem);
    fprintf(saida, "AVISO | linha %d | %s\n", linha, mensagem);
}

// Le os argumentos depois do programa fonte:
//   um nome de arquivo -> onde gravar o MEPA no estilo da aula; a versao
//                         exec ganha "_exec" antes da extensao
//   -completo          -> liga o modo completo (README 6.8)
void lerArgumentos(int argc, char *argv[])
{
    char *ponto;
    int i;

    snprintf(arqMepa, sizeof(arqMepa), "%s", ARQ_MEPA_PADRAO);
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-completo") == 0) {
            modoCompleto = 1;
        } else {
            snprintf(arqMepa, sizeof(arqMepa), "%s", argv[i]);
        }
    }

    // "saida_mepa.txt" -> "saida_mepa_exec.txt"; sem extensao: "nome_exec"
    snprintf(arqMepaExec, sizeof(arqMepaExec), "%s", arqMepa);
    ponto = strrchr(arqMepaExec, '.');
    if (ponto != NULL && strchr(ponto, '/') == NULL && strchr(ponto, '\\') == NULL) {
        *ponto = '\0';
        ponto = strrchr(arqMepa, '.');
        strncat(arqMepaExec, "_exec", sizeof(arqMepaExec) - strlen(arqMepaExec) - 1);
        strncat(arqMepaExec, ponto, sizeof(arqMepaExec) - strlen(arqMepaExec) - 1);
    } else {
        strncat(arqMepaExec, "_exec", sizeof(arqMepaExec) - strlen(arqMepaExec) - 1);
    }
}

// Acrescenta texto no fim do buffer, aumentando a memoria se precisar.
void anexarBuffer(Buffer *buffer, const char *texto)
{
    size_t tamTexto = strlen(texto);

    if (buffer->tamanho + tamTexto + 1 > buffer->capacidade) {
        size_t novaCapacidade = (buffer->capacidade == 0) ? 1024 : buffer->capacidade * 2;
        while (buffer->tamanho + tamTexto + 1 > novaCapacidade) {
            novaCapacidade *= 2;
        }
        buffer->texto = realloc(buffer->texto, novaCapacidade);
        if (buffer->texto == NULL) {
            printf("Erro: memoria insuficiente para gerar o codigo MEPA.\n");
            fecharAnalisador();
            exit(0);
        }
        buffer->capacidade = novaCapacidade;
    }
    memcpy(buffer->texto + buffer->tamanho, texto, tamTexto + 1);
    buffer->tamanho += tamTexto;
}

// Escreve uma linha nas duas versoes do codigo de destino.
void emitirLinhas(const char *linhaAula, const char *linhaExec)
{
    anexarBuffer(&destino->aula, linhaAula);
    anexarBuffer(&destino->exec, linhaExec);
}

// Instrucao sem parametro (ex.: SOMA, IMPR, LEIT)
void emitir(const char *instrucao)
{
    char linha[TAM_LINHA_MEPA];

    snprintf(linha, sizeof(linha), "     %s\n", instrucao);
    emitirLinhas(linha, linha);
}

// Instrucao com um numero (ex.: CRCT 5, AMEM 3)
void emitirInt(const char *instrucao, int valor)
{
    char linha[TAM_LINHA_MEPA];

    snprintf(linha, sizeof(linha), "     %s %d\n", instrucao, valor);
    emitirLinhas(linha, linha);
}

// "/" e divisao real: DIVF na aula. O InterprMEPA nao tem DIVF (so
// trabalha com inteiros), entao na versao exec vira DIVI.
void emitirDivisaoReal(void)
{
    emitirLinhas("     DIVF\n", "     DIVI\n");
}

// Rotulos: "L3: NADA" e "DSVF L3", como na aula. Dentro do corpo de uma
// rotina o rotulo e escrito "L#3", uma marca que expandirRotina() troca
// por um numero novo a cada chamada (ver README 6.5.5).
void emitirRotulo(int rotulo)
{
    char linha[TAM_LINHA_MEPA];

    if (destino == &codigoPrincipal) {
        snprintf(linha, sizeof(linha), "L%d: NADA\n", rotulo);
    } else {
        snprintf(linha, sizeof(linha), "L#%d: NADA\n", rotulo);
    }
    emitirLinhas(linha, linha);
}

// Rotulo com uma instrucao no lugar do NADA (modo completo: "L2: ENPR 1")
void emitirRotuloInstrucao(int rotulo, const char *instrucao)
{
    char linha[TAM_LINHA_MEPA];

    snprintf(linha, sizeof(linha), "L%d: %s\n", rotulo, instrucao);
    emitirLinhas(linha, linha);
}

void emitirDesvio(const char *instrucao, int rotulo)
{
    char linha[TAM_LINHA_MEPA];

    if (destino == &codigoPrincipal) {
        snprintf(linha, sizeof(linha), "     %s L%d\n", instrucao, rotulo);
    } else {
        snprintf(linha, sizeof(linha), "     %s L#%d\n", instrucao, rotulo);
    }
    emitirLinhas(linha, linha);
}

// Posicao "escondida" de memoria (nao e variavel do programa):
// temporarias, parametros e resultado de funcao. Elas ficam depois das
// variaveis globais, mas o numero de globais so e conhecido no fim; por
// isso a versao exec recebe a marca "@k@", trocada em gravarBuffer().
void emitirOculta(const char *instrucao, int posicao, const char *nomeAula)
{
    char linhaAula[TAM_LINHA_MEPA];
    char linhaExec[TAM_LINHA_MEPA];

    snprintf(linhaAula, sizeof(linhaAula), "     %s %s\n", instrucao, nomeAula);
    snprintf(linhaExec, sizeof(linhaExec), "     %s 0, @%d@\n", instrucao, posicao);
    emitirLinhas(linhaAula, linhaExec);
}

void emitirTemporaria(const char *instrucao, int temporaria)
{
    usouTemporaria = 1;
    emitirOculta(instrucao, temporaria, temporaria == TMP_INDICE ? "_tmp0" : "_tmp1");
}

// CRVL / ARMZ de uma variavel simples ou de um parametro.
//   variavel: endereco = ordem de declaracao (busca_tabela_simbolos)
//   parametro: posicao escondida; na aula aparece como "rotina_parametro"
void emitirVariavel(const char *instrucao, int idx)
{
    char linhaAula[TAM_LINHA_MEPA];
    char linhaExec[TAM_LINHA_MEPA];

    if (categoria[idx] == CAT_PARAM && modoCompleto) {
        // parametros ficam na pilha antes do registro de ativacao (6.8)
        snprintf(linhaAula, sizeof(linhaAula), "     %s %s_%s\n", instrucao,
                 tabelaSimbolos[rotinaAtual], tabelaSimbolos[idx]);
        snprintf(linhaExec, sizeof(linhaExec), "     %s 1, %d\n", instrucao,
                 endereco[idx] - (qtdParams[rotinaAtual] + 3));
        emitirLinhas(linhaAula, linhaExec);
        return;
    }
    if (categoria[idx] == CAT_PARAM) {
        snprintf(linhaAula, sizeof(linhaAula), "%s_%s", tabelaSimbolos[rotinaAtual], tabelaSimbolos[idx]);
        emitirOculta(instrucao, endereco[idx], linhaAula);
        return;
    }
    snprintf(linhaAula, sizeof(linhaAula), "     %s %s\n", instrucao, tabelaSimbolos[idx]);
    snprintf(linhaExec, sizeof(linhaExec), "     %s 0, %d\n", instrucao,
             busca_tabela_simbolos(tabelaSimbolos[idx]));
    emitirLinhas(linhaAula, linhaExec);
}

// CRVL / ARMZ de uma posicao FIXA do vetor (ex.: notas[3]).
// Endereco = endereco do vetor + (posicao - limite inferior)
void emitirElementoVetor(const char *instrucao, int idxVetor, int posicao)
{
    char linhaAula[TAM_LINHA_MEPA];
    char linhaExec[TAM_LINHA_MEPA];

    snprintf(linhaAula, sizeof(linhaAula), "     %s %s[%d]\n", instrucao, tabelaSimbolos[idxVetor], posicao);
    snprintf(linhaExec, sizeof(linhaExec), "     %s 0, %d\n", instrucao,
             endereco[idxVetor] + posicao - limInf[idxVetor]);
    emitirLinhas(linhaAula, linhaExec);
}

// A aula nao tem instrucao que leia um endereco calculado na hora, so
// CRVL/ARMZ com endereco fixo. Entao, com o indice no topo da pilha,
// testamos posicao por posicao (README 6.5.6):
//      ARMZ _tmp0
//      CRVL _tmp0 ; CRCT 1 ; CMIG ; DSVF Lp1 ; CRVL v[1] ; DSVS Lfim
//  Lp1: NADA
//      ... (uma vez para cada posicao) ...
//      CRCT 0                      (indice fora dos limites)
//  Lfim: NADA
void emitirLeituraVetor(int idxVetor)
{
    int rotuloFim;
    int rotuloProximo;
    int posicao;

    if (modoCompleto) {
        // modo completo: o endereco ja esta na pilha (README 6.8)
        emitirTemporaria("ARMZ", TMP_INDICE);
        emitirTemporaria("CRVI", TMP_INDICE);
        return;
    }
    rotuloFim = proximo_rotulo();

    emitirTemporaria("ARMZ", TMP_INDICE);
    for (posicao = limInf[idxVetor]; posicao <= limSup[idxVetor]; posicao++) {
        rotuloProximo = proximo_rotulo();
        emitirTemporaria("CRVL", TMP_INDICE);
        emitirInt("CRCT", posicao);
        emitir("CMIG");
        emitirDesvio("DSVF", rotuloProximo);
        emitirElementoVetor("CRVL", idxVetor, posicao);
        emitirDesvio("DSVS", rotuloFim);
        emitirRotulo(rotuloProximo);
    }
    emitirInt("CRCT", 0);
    emitirRotulo(rotuloFim);
}

// Mesma ideia para gravar: a pilha tem o indice e, em cima dele, o valor.
//      ARMZ _tmp1 (valor) ; ARMZ _tmp0 (indice)
//      CRVL _tmp0 ; CRCT 1 ; CMIG ; DSVF Lp1 ; CRVL _tmp1 ; ARMZ v[1] ; DSVS Lfim
//  Lp1: NADA ...
//  Lfim: NADA
void emitirEscritaVetor(int idxVetor)
{
    int rotuloFim;
    int rotuloProximo;
    int posicao;

    if (modoCompleto) {
        // modo completo: endereco embaixo, valor em cima (README 6.8)
        emitirTemporaria("ARMZ", TMP_VALOR);
        emitirTemporaria("ARMZ", TMP_INDICE);
        emitirTemporaria("CRVL", TMP_VALOR);
        emitirTemporaria("ARMI", TMP_INDICE);
        return;
    }
    rotuloFim = proximo_rotulo();

    emitirTemporaria("ARMZ", TMP_VALOR);
    emitirTemporaria("ARMZ", TMP_INDICE);
    for (posicao = limInf[idxVetor]; posicao <= limSup[idxVetor]; posicao++) {
        rotuloProximo = proximo_rotulo();
        emitirTemporaria("CRVL", TMP_INDICE);
        emitirInt("CRCT", posicao);
        emitir("CMIG");
        emitirDesvio("DSVF", rotuloProximo);
        emitirTemporaria("CRVL", TMP_VALOR);
        emitirElementoVetor("ARMZ", idxVetor, posicao);
        emitirDesvio("DSVS", rotuloFim);
        emitirRotulo(rotuloProximo);
    }
    emitirRotulo(rotuloFim);
}

// retorne e: o valor vai para a posicao escondida do resultado da
// funcao (na aula aparece com o nome da funcao) e o codigo pula para o
// fim do corpo.
void emitirRetornoFuncao(int idxFuncao)
{
    char linhaAula[TAM_LINHA_MEPA];
    char linhaExec[TAM_LINHA_MEPA];

    if (modoCompleto) {
        // o espaco do resultado (AMEM 1 de quem chamou) fica logo abaixo
        // dos parametros: -(qtdParams + 4)
        snprintf(linhaAula, sizeof(linhaAula), "     ARMZ %s\n", tabelaSimbolos[idxFuncao]);
        snprintf(linhaExec, sizeof(linhaExec), "     ARMZ 1, %d\n", -(qtdParams[idxFuncao] + 4));
        emitirLinhas(linhaAula, linhaExec);
        return;
    }
    emitirOculta("ARMZ", posicaoResultado[idxFuncao], tabelaSimbolos[idxFuncao]);
}

// Copia o "molde" (codigo do corpo de uma rotina) para o destino atual,
// trocando cada marca "L#n" por um rotulo novo: n - primeiro + base.
// Se o destino tambem e um molde (rotina chamando rotina), o rotulo novo
// continua marcado com "#", para ser trocado de novo quando essa outra
// rotina for expandida.
void copiarMolde(Buffer *saidaBuffer, Buffer *molde, int primeiro, int base)
{
    char numero[32];
    size_t i = 0;
    int rotulo;

    while (i < molde->tamanho) {
        if (molde->texto[i] == 'L' && i + 1 < molde->tamanho && molde->texto[i + 1] == '#') {
            i += 2;
            rotulo = 0;
            while (i < molde->tamanho && molde->texto[i] >= '0' && molde->texto[i] <= '9') {
                rotulo = rotulo * 10 + (molde->texto[i] - '0');
                i++;
            }
            snprintf(numero, sizeof(numero), (destino == &codigoPrincipal) ? "L%d" : "L#%d",
                     rotulo - primeiro + base);
            anexarBuffer(saidaBuffer, numero);
        } else {
            numero[0] = molde->texto[i];
            numero[1] = '\0';
            anexarBuffer(saidaBuffer, numero);
            i++;
        }
    }
}

// Chamada de procedimento/funcao. A aula nao tem instrucao de chamada de
// rotina, entao o corpo da rotina e copiado no lugar da chamada
// (README 6.5.5):
//      <argumentos> (ja empilhados por parseArgumentos)
//      ARMZ rotina_paramN ... ARMZ rotina_param1   (do topo para baixo)
//      <corpo da rotina com rotulos novos>
//      CRVL funcao                                  (so funcao: o resultado)
void expandirRotina(int idxRotina, int linha)
{
    int quantidade;
    int base;
    int i;
    char nome[TAM_LINHA_MEPA];

    if (modoCompleto) {
        // chamada de verdade: CHPR <rotulo da rotina>, <nivel de quem chama>
        snprintf(nome, sizeof(nome), "     CHPR L%d, %d\n", rotuloRotina[idxRotina],
                 rotinaAtual == -1 ? 0 : 1);
        emitirLinhas(nome, nome);
        return;
    }
    if (idxRotina == rotinaAtual) {
        erroGeracao(linha, tabelaSimbolos[idxRotina],
                    "recursao nao e suportada: a MEPA da aula nao tem instrucao de chamada de rotina");
    }

    for (i = qtdParams[idxRotina] - 1; i >= 0; i--) {
        snprintf(nome, sizeof(nome), "%s_%s", tabelaSimbolos[idxRotina],
                 tabelaSimbolos[paramDaRotina[idxRotina][i]]);
        emitirOculta("ARMZ", posicaoParam[idxRotina][i], nome);
    }

    quantidade = ultimoRotulo[idxRotina] - primeiroRotulo[idxRotina] + 1;
    base = contadorRotulos + 1;
    contadorRotulos += quantidade;  // reserva rotulos novos para esta copia
    copiarMolde(&destino->aula, &codigoRotina[idxRotina].aula, primeiroRotulo[idxRotina], base);
    copiarMolde(&destino->exec, &codigoRotina[idxRotina].exec, primeiroRotulo[idxRotina], base);

    if (categoria[idxRotina] == CAT_FUNC) {
        emitirOculta("CRVL", posicaoResultado[idxRotina], tabelaSimbolos[idxRotina]);
    }
}

// Mensagem de erro para o que a analise aceita mas a MEPA da aula nao
// consegue gerar (hoje so a recursao).
noreturn void erroGeracao(int linha, char *identificador, char *motivo)
{
    const char *formato = "ERRO DE GERACAO DE CODIGO | linha %d | sequencia '%s' | %s\n";

    printf(formato, linha, identificador, motivo);
    fprintf(saida, formato, linha, identificador, motivo);
    fecharAnalisador();
    exit(0);
}

// Grava o buffer no arquivo. Na versao exec cada "@k@" vira o endereco
// real da posicao escondida k (= quantidade de globais + k).
void gravarBuffer(FILE *arquivo, Buffer *buffer, int trocarOcultas)
{
    size_t i = 0;
    int posicao;

    while (i < buffer->tamanho) {
        if (trocarOcultas && buffer->texto[i] == '@') {
            i++;
            posicao = 0;
            while (buffer->texto[i] != '@') {
                posicao = posicao * 10 + (buffer->texto[i] - '0');
                i++;
            }
            i++;
            // se nenhuma temporaria foi usada, elas nao sao reservadas e
            // as outras posicoes escondidas andam QTD_TEMPORARIAS para tras
            if (!usouTemporaria) {
                posicao -= QTD_TEMPORARIAS;
            }
            fprintf(arquivo, "%d", proxEndereco + posicao);
        } else {
            fputc(buffer->texto[i], arquivo);
            i++;
        }
    }
}

// Escreve os dois arquivos MEPA, no formato da aula:
//      INPP
//      AMEM n        (variaveis globais + posicoes escondidas)
//      ... comandos do programa principal ...
//      PARA
void gravarCodigoMepa(void)
{
    const char *nomes[2] = { arqMepa, arqMepaExec };
    Buffer *buffers[2] = { &codigoPrincipal.aula, &codigoPrincipal.exec };
    int memoria = proxEndereco + proxOculta - (usouTemporaria ? 0 : QTD_TEMPORARIAS);
    FILE *arquivo;
    int i;

    for (i = 0; i < 2; i++) {
        arquivo = fopen(nomes[i], "w");
        if (arquivo == NULL) {
            printf("Erro ao criar o arquivo %s.\n", nomes[i]);
            fecharAnalisador();
            exit(0);
        }
        fprintf(arquivo, "     INPP\n");
        fprintf(arquivo, "     AMEM %d\n", memoria);
        gravarBuffer(arquivo, buffers[i], i == 1);
        if (modoCompleto) {
            fprintf(arquivo, "     DMEM %d\n", memoria);
        }
        fprintf(arquivo, "     PARA\n");
        fclose(arquivo);
    }
}
