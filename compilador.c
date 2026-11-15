//  COMPILADORES - PROJETO Fase 1 - MiniVisualg
//  Etapa 2 (Analisador Lexico) + Etapa 3 (Analisador Sintatico)
//  Integrantes:
//    - Henrique Ferreira Marciano - RA 10439797
//    - Pedro Casas Pequeno Junior - RA 10437031
//    - Pedro Gabriel Guimarães Fernandes - RA 10437465
//   Professora as explicacoes da gramatica e das decisoes de projeto estao no arquivo readme.txt.
//  Compilar: gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
//  Executar: ./compilador programa.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>

#define TAM_LEXEMA 256
#define MAX_SIMBOLOS 500
#define MAX_PARAMS 8

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

typedef enum { 
    CAT_VAR,
    CAT_VETOR,
    CAT_PARAM,
    CAT_PROC,
    CAT_FUNC 
} Categoria;

typedef enum {
    T_INT,
    T_REAL,
    T_CHAR,
    T_LOG,
    T_NENHUM
} TipoBase;


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

// Variaveis globais
FILE *fonte;
int linhaAtual = 1;
FILE *saida = NULL;
char tabelaSimbolos[MAX_SIMBOLOS][TAM_LEXEMA];
int  qtdSimbolos = 0;

// Tabela de simbolos estendida (Fase 2) - ver README 6.1
// a posicao i de cada vetor descreve o identificador tabelaSimbolos[i]
int       declarada[MAX_SIMBOLOS];
Categoria categoria[MAX_SIMBOLOS];
TipoBase  tipoBase[MAX_SIMBOLOS];
int       endereco[MAX_SIMBOLOS];
int       atribuida[MAX_SIMBOLOS];
int       usada[MAX_SIMBOLOS];
int       limInf[MAX_SIMBOLOS];
int       limSup[MAX_SIMBOLOS];
int       qtdParams[MAX_SIMBOLOS];
TipoBase  tipoParam[MAX_SIMBOLOS][MAX_PARAMS];

int proxEndereco = 0;   
int rotinaAtual = -1;
int paramsRotina[MAX_PARAMS];

void declarar(int idx, Categoria cat, TipoBase tipo);
int proximo_rotulo(void);
noreturn void erroSemantico(char *identificador, char *motivo);
int busca_tabela_simbolos(char *id);
void usarComoVariavel(int idx, int temIndice, int linha);
void usarComoRotina(int idx, int linha);
void iniciarRotina(int idx, Categoria cat);
void encerrarRotina(int enderecoAntes);
noreturn void erroSemanticoNaLinha(int linha, char *identificador, char *motivo);

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
void parseComandoSe(void);
void parseComandoPara(void);
void parseComandoEnquanto(void);
void parseComandoLeia(void);
void parseVariavelLeia(void);
void parseComandoEscreva(void);
void parseComandoRetorne(void);
void parseExpressao(void);
void parseExprE(void);
void parseExprRel(void);
void parseExprArit(void);
void parseTermo(void);
void parseFator(void);

// main
int main(int argc, char *argv[])
{

    if (argc < 2) {
        printf("Uso: compilador <arquivo_fonte>\n");
        return 0;
    }
    fonte = fopen(argv[1], "rb");
    if (fonte == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    iniciarAnalisador(fonte);

    avancar(); // pega o primeiro token (nextToken)
    parsePrograma();

    if (tokenAtual.type != TOKEN_EOF) {
        erroSintatico(tokenAtual.lexema, "codigo inesperado depois de 'fimalgoritmo'");
    }

    printf("\n>>> Programa aceito: a cadeia de tokens pertence a linguagem MiniVisualg.\n");

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

// Erro semantico na linha do token atual.
noreturn void erroSemantico(char *sequencia, char *motivo)
{
    erroSemanticoNaLinha(tokenAtual.line, sequencia, motivo);
}

// Erro semantico numa linha informada. Usada quando o erro so e
// percebido depois de o parser ja ter avancado para o proximo token
// (ex.: "x <- v" com v vetor: o erro e da linha do v). Mostra a
// mensagem na tela e no arquivo de saida e encerra com exit(0).
noreturn void erroSemanticoNaLinha(int linha, char *sequencia, char *motivo)
{
    printf("ERRO SEMANTICO | linha %d | sequencia '%s' | %s\n", linha, sequencia, motivo);
    fprintf(saida, "ERRO SEMANTICO | linha %d | sequencia '%s' | %s\n", linha, sequencia, motivo);
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

void parsePrograma(void)
{
    casar("algoritmo");
    casarTipo(TOKEN_STRING, "esperado o nome do algoritmo entre aspas");
    parseListaSubprogramas();
    parseSecaoVar();
    casar("inicio");
    parseListaComandos();
    casar("fimalgoritmo");
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

void parseProcedimento(void)
{
    int idx;
    int enderecoAntes = proxEndereco;

    casar("procedimento");
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    casarTipo(TOKEN_ID, "esperado o nome do procedimento");
    iniciarRotina(idx, CAT_PROC);            // antes do corpo: permite recursao
    parseParametrosOpc();
    casar("inicio");
    parseListaComandos();
    casar("fimprocedimento");
    encerrarRotina(enderecoAntes);
}

void parseFuncao(void)
{
    int idx;
    int enderecoAntes = proxEndereco;

    casar("funcao");
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    casarTipo(TOKEN_ID, "esperado o nome da funcao");
    iniciarRotina(idx, CAT_FUNC);            // antes do corpo: permite recursao
    parseParametrosOpc();
    casar(":");
    tipoBase[idx] = parseTipoBase();         // tipo de retorno da funcao
    casar("inicio");
    parseListaComandos();
    casar("fimfuncao");
    encerrarRotina(enderecoAntes);
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
// da rotina em analise (qtdParams / tipoParam), usada no passo 5.
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

    paramsRotina[n] = idx;
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
void parseAtribuicaoOuChamada(void)
{
    int idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    int linha = tokenAtual.line;
    int temIndice = 0;

    casarTipo(TOKEN_ID, "esperado identificador");

    if (tokenEh("[")) {
        casar("[");
        parseExpressao();
        casar("]");
        temIndice = 1;
    }

    if (tokenEh("<-")) {
        usarComoVariavel(idx, temIndice, linha);
        atribuida[idx] = 1;
        casar("<-");
        parseExpressao();
    }
    else if (temIndice) {
        erroSintatico(tokenAtual.lexema, "esperado '<-' depois do indice do vetor");
    }
    else if (tokenEh("(")) {
        usarComoRotina(idx, linha);
        casar("(");
        if (!tokenEh(")")) {
            parseExpressao();
            while (tokenEh(",")) {
                casar(",");
                parseExpressao();
            }
        }
        casar(")");
    }
    else {
        // nenhum dos dois: chamada de procedimento sem parametros
        usarComoRotina(idx, linha);
    }
}

void parseComandoSe(void)
{
    casar("se");
    casar("(");
    parseExpressao();
    casar(")");
    casar("entao");
    parseListaComandos();
    if (tokenEh("senao")) {
        casar("senao");
        parseListaComandos();
    }
    casar("fimse");
}

void parseComandoPara(void)
{
    int idx;
    int linha;

    casar("para");
    idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    linha = tokenAtual.line;
    casarTipo(TOKEN_ID, "esperado identificador da variavel de controle");
    usarComoVariavel(idx, 0, linha);
    atribuida[idx] = 1;  // o para atribui valores a variavel de controle
    usada[idx] = 1;      // e tambem a le a cada volta
    casar("de");
    parseExpressao();
    casar("ate");
    parseExpressao();
    if (tokenEh("passo")) {
        casar("passo");
        parseExpressao();
    }
    casar("faca");
    parseListaComandos();
    casar("fimpara");
}

void parseComandoEnquanto(void)
{
    casar("enquanto");
    casar("(");
    parseExpressao();
    casar(")");
    casar("faca");
    parseListaComandos();
    casar("fimenquanto");
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
void parseVariavelLeia(void)
{
    int idx = tokenAtual.attribute.table_index;  // ler antes de consumir
    int linha = tokenAtual.line;
    int temIndice = 0;

    casarTipo(TOKEN_ID, "esperado identificador");
    if (tokenEh("[")) {
        casar("[");
        parseExpressao();
        casar("]");
        temIndice = 1;
    }
    usarComoVariavel(idx, temIndice, linha);
    atribuida[idx] = 1;
}

void parseComandoEscreva(void)
{
    if (tokenEh("escreva")) {
        casar("escreva");
    } else {
        casar("escreval");
    }
    casar("(");
    if (!tokenEh(")")) {
        parseExpressao();
        while (tokenEh(",")) {
            casar(",");
            parseExpressao();
        }
    }
    casar(")");
}

void parseComandoRetorne(void)
{
    casar("retorne");
    parseExpressao();
}

// expressoes
// precedencia (menor pra maior): OU < E < relacional < + - < * / \ MOD
// (ver README secao 4.3)

void parseExpressao(void)
{
    parseExprE();
    while (tokenEh("OU")) {
        casar("OU");
        parseExprE();
    }
}

void parseExprE(void)
{
    parseExprRel();
    while (tokenEh("E")) {
        casar("E");
        parseExprRel();
    }
}

// "if" e nao "while": no maximo um operador relacional por expressao
void parseExprRel(void)
{
    parseExprArit();
    if (tokenAtual.type == TOKEN_OP_REL) {
        avancar();
        parseExprArit();
    }
}

void parseExprArit(void)
{
    parseTermo();
    while (tokenEh("+") || tokenEh("-")) {
        avancar();
        parseTermo();
    }
}

void parseTermo(void)
{
    parseFator();
    while (tokenEh("*") || tokenEh("/") || tokenEh("\\") || tokenEh("MOD")) {
        avancar();
        parseFator();
    }
}

void parseFator(void)
{
    if (tokenEh("(")) {
        casar("(");
        parseExpressao();
        casar(")");
    }
    else if (tokenEh("-")) {
        casar("-");
        parseFator(); // permite "- - x"
    }
    else if (tokenAtual.type == TOKEN_ID) {
        int idx = tokenAtual.attribute.table_index;  // ler antes de consumir
        int linha = tokenAtual.line;

        casarTipo(TOKEN_ID, "esperado identificador");
        if (tokenEh("[")) {
            usarComoVariavel(idx, 1, linha);
            usada[idx] = 1;
            casar("[");
            parseExpressao();
            casar("]");
        }
        else if (tokenEh("(")) {
            usarComoRotina(idx, linha);
            if (categoria[idx] == CAT_PROC) {
                erroSemanticoNaLinha(linha, tabelaSimbolos[idx], "procedimento nao pode ser usado em expressao");
            }
            casar("(");
            if (!tokenEh(")")) {
                parseExpressao();
                while (tokenEh(",")) {
                    casar(",");
                    parseExpressao();
                }
            }
            casar(")");
        }
        else {
            usarComoVariavel(idx, 0, linha);
            usada[idx] = 1;
        }
    }
    else if (tokenAtual.type == TOKEN_NUM_INT) {
        casarTipo(TOKEN_NUM_INT, "esperado numero inteiro");
    }
    else if (tokenAtual.type == TOKEN_NUM_FLOAT) {
        casarTipo(TOKEN_NUM_FLOAT, "esperado numero real");
    }
    else if (tokenAtual.type == TOKEN_STRING) {
        casarTipo(TOKEN_STRING, "esperado cadeia");
    }
    else if (tokenEh("verdadeiro") || tokenEh("falso")) {
        avancar();
    }
    else {
        erroSintatico(tokenAtual.lexema, "esperado um fator valido (identificador, numero, cadeia, 'verdadeiro', 'falso', '(' ou '-')");
    }
}


// Analisador semantico (Fase 2)
// Tabela de simbolos = matriz tabelaSimbolos[][] + vetores paralelos
// (ver README secao 6.1)

// Declara o identificador da posicao idx da tabela.
// Erro semantico se o nome ja tiver sido declarado (nome unico).
// Variaveis e parametros recebem o proximo endereco livre;
// procedimentos e funcoes nao ocupam memoria de dados (endereco -1).
void declarar(int idx, Categoria cat, TipoBase tipo)
{
    if (declarada[idx]) {
        erroSemantico(tabelaSimbolos[idx], "identificador declarado mais de uma vez");
    }

    declarada[idx] = 1;
    categoria[idx] = cat;
    tipoBase[idx]  = tipo;
    atribuida[idx] = 0;
    usada[idx]     = 0;

    if (cat == CAT_VAR || cat == CAT_PARAM || cat == CAT_VETOR) {
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
    static int contador = 0;

    contador++;
    return contador;
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

// Registra a rotina antes de ler os parametros e o corpo (assim ela
// pode chamar a si mesma) e marca-a como rotina em analise.
void iniciarRotina(int idx, Categoria cat)
{
    declarar(idx, cat, T_NENHUM);  // funcao: tipo de retorno vem depois
    qtdParams[idx] = 0;
    rotinaAtual = idx;
}

// Fim da rotina: os parametros saem de escopo (podem ser reusados em
// outra rotina ou como variavel global) e os enderecos que eles
// ocuparam sao devolvidos.
void encerrarRotina(int enderecoAntes)
{
    int i;

    for (i = 0; i < qtdParams[rotinaAtual]; i++) {
        declarada[paramsRotina[i]] = 0;
    }
    proxEndereco = enderecoAntes;
    rotinaAtual = -1;
}