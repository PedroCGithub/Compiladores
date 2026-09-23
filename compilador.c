// ====================================================================
//  COMPILADORES - PROJETO Fase 1 - MiniVisualg
//  Etapa 2 (Analisador Lexico) + Etapa 3 (Analisador Sintatico)
//
//  Integrantes:
//    - NOME COMPLETO 1 - RA
//    - NOME COMPLETO 2 - RA
//    - NOME COMPLETO 3 - RA
//
//  Compilar: gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
//  Executar: compilador programa.txt
// ====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>

#define TAM_LEXEMA 256
#define MAX_SIMBOLOS 500

// ====================================================================
// Definicao de tipos
// ====================================================================

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

// Acrescentamos OP_NE (o operador "<>") -- ver explicacao mais abaixo,
// em classificarLexema(), sobre um bug pequeno que isso corrige.
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

// ====================================================================
// Funcoes do analisador lexico (etapa 2 -- praticamente sem mudancas)
// ====================================================================
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

// ====================================================================
// Variaveis globais
// ====================================================================
FILE *fonte;
int linhaAtual = 1;
FILE *saida = NULL;
char tabelaSimbolos[MAX_SIMBOLOS][TAM_LEXEMA];
int  qtdSimbolos = 0;

// ====================================================================
// ETAPA 3 - Analisador Sintatico Descendente Recursivo Preditivo
// ====================================================================
//
// Ideia basica, pra quem esta vendo isso pela primeira vez:
//
// Um analisador sintatico "descendente recursivo" e, na pratica, uma
// funcao em C para CADA simbolo nao-terminal da gramatica. Por
// exemplo, se a gramatica tem a regra
//
//     comando_se -> 'se' '(' expressao ')' 'entao' lista_comandos 'fimse'
//
// entao criamos uma funcao parseComandoSe() que:
//   1) confere se o token atual e mesmo 'se' e consome ele,
//   2) confere se e '(' e consome,
//   3) CHAMA a funcao parseExpressao() pra tratar a parte "expressao"
//      (e por isso que se chama "recursivo": uma funcao de parsing
//      chama outra),
//   4) e assim por diante.
//
// Se em algum passo o token nao for o esperado, e porque o codigo
// fonte tem um erro de sintaxe -> chamamos erroSintatico() e paramos.
//
// "Preditivo" quer dizer que a gente NUNCA precisa voltar atras ou
// tentar mais de uma opcao: só de olhar o token atual (1 unico
// simbolo de lookahead -> por isso "LL(1)") ja da pra saber qual
// producao da gramatica aplicar. Pra isso funcionar, a gramatica
// precisa ser:
//   - nao ambigua
//   - fatorada a esquerda (nao pode ter duas producoes do mesmo
//     nao-terminal comecando com o mesmo token)
//   - sem recursao a esquerda (uma producao "A -> A + B" trava um
//     parser recursivo em loop infinito)
//
// A gramatica que usamos abaixo ja nasce assim, seguindo o mesmo
// truque mostrado no slide da disciplina para eliminar recursao a
// esquerda em expressoes aritmeticas (E -> T E', E' -> + T E' | eps):
// em vez de criar um nao-terminal extra so pra isso, usamos um laco
// "while" -- da exatamente no mesmo resultado, mas fica mais direto
// de ler em C.
//
// Como o parser conversa com o lexico (o "nextToken()" do enunciado):
//
//   - Existe uma variavel global tokenAtual: e o "1 token de
//     lookahead" que o parser sempre enxerga.
//   - A funcao avancar() faz o papel do nextToken(): ela pede o
//     proximo token pro lexico (chamando obterToken(), a mesma
//     funcao da etapa 2) e guarda em tokenAtual. De quebra, ja
//     imprime o token, entao a etapa 2 continua funcionando
//     normalmente -- so que agora quem manda buscar cada token e o
//     proprio parser, token por token, em vez de um loop separado.
//
// Gramatica usada (ja fatorada e sem recursao a esquerda). Essa
// gramatica foi construida a partir dos exemplos do anexo do
// enunciado (ANEXO I), cobrindo: algoritmo, declaracao de variaveis,
// vetores, procedimentos, funcoes, se/senao, para, enquanto, leia,
// escreva/escreval, atribuicao, chamada de procedimento/funcao e
// expressoes aritmeticas/relacionais/logicas.
//
//   programa           -> 'algoritmo' cadeia lista_subprogramas
//                          secao_var 'inicio' lista_comandos 'fimalgoritmo'
//
//   lista_subprogramas -> (procedimento | funcao)*
//   procedimento       -> 'procedimento' id parametros_opc
//                          'inicio' lista_comandos 'fimprocedimento'
//   funcao             -> 'funcao' id parametros_opc ':' tipo
//                          'inicio' lista_comandos 'fimfuncao'
//   parametros_opc     -> ( '(' lista_parametros? ')' )?
//   lista_parametros   -> parametro (',' parametro)*
//   parametro          -> id ':' tipo
//
//   secao_var          -> ( 'var' lista_declaracoes )?
//   lista_declaracoes  -> declaracao*
//   declaracao         -> lista_ids ':' tipo
//   lista_ids          -> id (',' id)*
//   tipo               -> 'inteiro' | 'real' | 'caractere' | 'logico'
//                        | 'vetor' '[' num '..' num ']' 'de' tipo
//
//   lista_comandos     -> comando*
//   comando            -> atrib_ou_chamada | comando_se | comando_para
//                        | comando_enquanto | comando_leia
//                        | comando_escreva | comando_retorne
//
//   atrib_ou_chamada   -> id ( '[' expressao ']' )? ( '<-' expressao
//                        | '(' lista_args? ')' )?
//   comando_se         -> 'se' '(' expressao ')' 'entao' lista_comandos
//                          ( 'senao' lista_comandos )? 'fimse'
//   comando_para       -> 'para' id 'de' expressao 'ate' expressao
//                          ( 'passo' expressao )? 'faca'
//                          lista_comandos 'fimpara'
//   comando_enquanto   -> 'enquanto' '(' expressao ')' 'faca'
//                          lista_comandos 'fimenquanto'
//   comando_leia       -> 'leia' '(' idx_ou_id (',' idx_ou_id)* ')'
//   comando_escreva    -> ('escreva'|'escreval') '(' lista_expressoes? ')'
//   comando_retorne    -> 'retorne' expressao
//
//   expressao   -> expr_e ('OU' expr_e)*
//   expr_e      -> expr_rel ('E' expr_rel)*
//   expr_rel    -> expr_arit (relop expr_arit)?
//   expr_arit   -> termo (('+' | '-') termo)*
//   termo       -> fator (('*' | '/' | '\' | 'MOD') fator)*
//   fator       -> '(' expressao ')'
//                | '-' fator
//                | id ('[' expressao ']' | '(' lista_args? ')')?
//                | num_int | num_real | cadeia | 'verdadeiro' | 'falso'
//
// Obs: o projeto (PROJETO_1) so exige: (1) reconhecer se a cadeia de
// tokens pertence a linguagem, e (2) emitir "ERRO SINTATICO" com o
// token e a linha quando nao pertencer. Nao pedimos pra montar arvore
// de derivacao aqui -- cada funcao de parsing so PRECISA consumir os
// tokens corretos; se conseguir chegar ate o fim sem cair em nenhum
// erroSintatico(), o programa e sintaticamente valido.
//
// ====================================================================

// tokenAtual = o token que o parser esta "olhando" agora (lookahead)
Token tokenAtual;

void avancar(void);
int tokenEh(const char *lexema);
void casar(const char *lexemaEsperado);
void casarTipo(TokenNome tipoEsperado, const char *descricao);
int inicioDeComando(void);
noreturn void erroSintatico(char *tokenEncontrado, char *motivo);

// uma funcao para cada nao-terminal da gramatica acima
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
void parseListaIds(void);
void parseTipo(void);
void parseListaComandos(void);
void parseComando(void);
void parseAtribuicaoOuChamada(void);
void parseComandoSe(void);
void parseComandoPara(void);
void parseComandoEnquanto(void);
void parseComandoLeia(void);
void parseComandoEscreva(void);
void parseComandoRetorne(void);
void parseExpressao(void);
void parseExprE(void);
void parseExprRel(void);
void parseExprArit(void);
void parseTermo(void);
void parseFator(void);

// ====================================================================
// main
// ====================================================================
int main(int argc, char *argv[])
{
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

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

    // avancar() busca o 1o token do arquivo (e ja imprime, como pede
    // a etapa 2). A partir daqui, quem chama avancar() de novo e o
    // proprio parser, sempre que "casa" (consome) um token.
    avancar();

    parsePrograma();

    // Se sobrou algo depois do 'fimalgoritmo' (ex: lixo no arquivo,
    // ou um segundo "algoritmo"), isso tambem e erro sintatico: o
    // programa deveria ter acabado exatamente aqui (tokenAtual deve
    // ser TOKEN_EOF).
    if (tokenAtual.type != TOKEN_EOF) {
        erroSintatico(tokenAtual.lexema, "codigo inesperado depois de 'fimalgoritmo'");
    }

    printf("\n>>> Programa aceito: a cadeia de tokens pertence a linguagem MiniVisualg.\n");

    fecharAnalisador();
    return 0;
}

// ====================================================================
// Analisador lexico (etapa 2) -- igual ao original, so com o ajuste
// comentado em classificarLexema()
// ====================================================================
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

    // Acrescentamos "<>" aqui embaixo. Detalhe: o lexico (em
    // obterToken, la em cima) ja SABIA ler "<>" como um unico lexema
    // de 2 caracteres, mas essa funcao classificarLexema nao tinha
    // "<>" nessa lista de comparacao -- entao ele caia no "return
    // TOKEN_ID" la no final e virava, por engano, um identificador
    // chamado "<>". Isso quebrava qualquer expressao tipo
    // nome <> "Joao" (que aparece no proprio anexo do enunciado).
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

// ====================================================================
// ETAPA 3 - Implementacao do parser
// ====================================================================

// avancar() e o "nextToken()" do enunciado: pede o proximo token pro
// lexico e ja imprime ele (isso mantem o requisito da etapa 2 de
// mostrar cada token reconhecido).
void avancar(void)
{
    tokenAtual = obterToken();
    imprimirToken(tokenAtual);
}

// Compara o token atual com um texto fixo. Serve pra palavras
// reservadas e simbolos, que sempre tem o mesmo lexema (ex: "se",
// "(", "<-"). NAO serve pra identificador/numero/cadeia, porque o
// texto deles muda a cada vez (o proximo id pode ser "x", "idade",
// etc) -- pra esses casos usamos casarTipo() em vez de casar().
int tokenEh(const char *lexema)
{
    return strcmp(tokenAtual.lexema, lexema) == 0;
}

// "Casar" um token = conferir que ele e o que a gramatica esperava
// naquele ponto e, se for, consumir (avancar para o proximo). Se nao
// for, a cadeia de tokens NAO segue a gramatica -> erro sintatico.
void casar(const char *lexemaEsperado)
{
    if (tokenEh(lexemaEsperado)) {
        avancar();
    } else {
        erroSintatico(tokenAtual.lexema, tokenAtual.lexema);
    }
}

// Mesma ideia de casar(), mas conferindo o TIPO do token (TOKEN_ID,
// TOKEN_NUM_INT, TOKEN_STRING...) em vez do lexema exato.
void casarTipo(TokenNome tipoEsperado, const char *descricao)
{
    if (tokenAtual.type == tipoEsperado) {
        avancar();
    } else {
        erroSintatico(tokenAtual.lexema, (char *) descricao);
    }
}

// FIRST(comando): a lista de tokens que podem ser o PRIMEIRO token de
// algum comando. Usamos isso em lista_comandos para saber quando
// parar de ler comandos (quando o token atual nao inicia nenhum
// comando, normalmente e porque chegamos num "fim..." ou "senao").
int inicioDeComando(void)
{
    return tokenAtual.type == TOKEN_ID ||
           tokenEh("se") || tokenEh("para") || tokenEh("enquanto") ||
           tokenEh("leia") || tokenEh("escreva") || tokenEh("escreval") ||
           tokenEh("retorne");
}

// Mensagem de erro no formato pedido pelo enunciado: linha + token +
// motivo. exit(0) porque o enunciado desconta ponto se o programa nao
// terminar com retorno 0.
noreturn void erroSintatico(char *tokenEncontrado, char *motivo)
{
    printf("ERRO SINTATICO | linha %d | token '%s' | motivo: %s\n", tokenAtual.line, tokenEncontrado, motivo);
    fprintf(saida, "ERRO SINTATICO | linha %d | token '%s' | motivo: %s\n", tokenAtual.line, tokenEncontrado, motivo);
    fecharAnalisador();
    exit(0);
}

// -----------------------------------------------------------------
// programa / subprogramas / declaracoes de variaveis
// -----------------------------------------------------------------

// Ponto de entrada da gramatica inteira. Todo programa MiniVisualg
// comeca com 'algoritmo "nome"' e termina com 'fimalgoritmo'.
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

// Um programa pode ter zero ou mais procedimentos/funcoes declarados
// antes da secao "var" (foi assim nos exemplos do anexo, como em
// "RotinasComRetorno"). Por isso usamos um "while": enquanto o token
// atual for 'procedimento' ou 'funcao', continuamos lendo mais um.
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
    casar("procedimento");
    casarTipo(TOKEN_ID, "esperado o nome do procedimento");
    parseParametrosOpc();
    casar("inicio");
    parseListaComandos();
    casar("fimprocedimento");
}

// Funcao e igual a procedimento, so que declara o tipo de retorno
// depois do ":" (ex: "funcao somar(...): inteiro").
void parseFuncao(void)
{
    casar("funcao");
    casarTipo(TOKEN_ID, "esperado o nome da funcao");
    parseParametrosOpc();
    casar(":");
    parseTipo();
    casar("inicio");
    parseListaComandos();
    casar("fimfuncao");
}

// Os parenteses de parametros so aparecem SE houver parametros. O
// proprio arquivo original ja tinha essa observacao em comentario:
// procedimento sem parametro nao usa parenteses nem na declaracao nem
// na chamada (ex: "linha_decorativa" no anexo). Por isso o '(' aqui e
// opcional.
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

void parseParametro(void)
{
    casarTipo(TOKEN_ID, "esperado nome de parametro");
    casar(":");
    parseTipo();
}

// A secao "var" e opcional (um algoritmo pode nao declarar nenhuma
// variavel, como no exemplo "PrimeiroPasso" do anexo).
void parseSecaoVar(void)
{
    if (tokenEh("var")) {
        casar("var");
        parseListaDeclaracoes();
    }
}

// Zero ou mais declaracoes. Uma declaracao sempre comeca com um
// identificador (o nome da variavel), entao usamos isso como
// condicao de parada do laço: assim que o token atual deixar de ser
// TOKEN_ID, significa que acabaram as declaracoes (o proximo deve
// ser 'inicio').
void parseListaDeclaracoes(void)
{
    while (tokenAtual.type == TOKEN_ID) {
        parseDeclaracao();
    }
}

// Ex: "n1, n2: inteiro"
void parseDeclaracao(void)
{
    parseListaIds();
    casar(":");
    parseTipo();
}

void parseListaIds(void)
{
    casarTipo(TOKEN_ID, "esperado identificador");
    while (tokenEh(",")) {
        casar(",");
        casarTipo(TOKEN_ID, "esperado identificador");
    }
}

// tipo -> inteiro | real | caractere | logico | vetor[n..n] de tipo
void parseTipo(void)
{
    if (tokenEh("inteiro") || tokenEh("real") || tokenEh("caractere") || tokenEh("logico")) {
        avancar();
    }
    else if (tokenEh("vetor")) {
        casar("vetor");
        casar("[");
        casarTipo(TOKEN_NUM_INT, "esperado numero inteiro (limite inferior do vetor)");
        casar("..");
        casarTipo(TOKEN_NUM_INT, "esperado numero inteiro (limite superior do vetor)");
        casar("]");
        casar("de");
        parseTipo(); // "vetor de X" -- X pode ser qualquer tipo base
    }
    else {
        erroSintatico(tokenAtual.lexema, "esperado um tipo (inteiro, real, caractere, logico ou vetor)");
    }
}

// -----------------------------------------------------------------
// comandos
// -----------------------------------------------------------------

// Zero ou mais comandos seguidos, ate aparecer um token que nao inicia
// comando nenhum (tipicamente um "fim..." ou "senao").
void parseListaComandos(void)
{
    while (inicioDeComando()) {
        parseComando();
    }
}

// Aqui e onde o parser "escolhe" qual regra da gramatica aplicar,
// olhando so o token atual -- essa e a parte "preditiva" do LL(1).
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

// Esse e o unico lugar onde precisamos "espiar mais um pouco" depois
// de consumir o identificador, pra saber qual das 4 formas e:
//
//   x <- expressao            -> atribuicao simples
//   x[i] <- expressao         -> atribuicao numa posicao de vetor
//   f(a, b)                   -> chamada de procedimento/funcao com args
//   procedimento_sem_param    -> chamada sem parenteses nenhum
//
// Repara que continua sendo so 1 token de lookahead por vez (o que
// vem logo depois do ID) -- entao a gramatica continua LL(1), so
// ficou com 4 "finais" possiveis pra mesma regra.
void parseAtribuicaoOuChamada(void)
{
    casarTipo(TOKEN_ID, "esperado identificador");

    if (tokenEh("[")) {
        casar("[");
        parseExpressao();
        casar("]");
    }

    if (tokenEh("<-")) {
        casar("<-");
        parseExpressao();
    }
    else if (tokenEh("(")) {
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
    // Se nao caiu em nenhum "if" acima, e chamada de procedimento sem
    // parametro (ex: "linha_decorativa") -- nao ha mais nada pra
    // consumir, entao simplesmente nao fazemos nada aqui.
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
    casar("para");
    casarTipo(TOKEN_ID, "esperado identificador da variavel de controle");
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

    casarTipo(TOKEN_ID, "esperado identificador");
    if (tokenEh("[")) {
        casar("[");
        parseExpressao();
        casar("]");
    }
    while (tokenEh(",")) {
        casar(",");
        casarTipo(TOKEN_ID, "esperado identificador");
        if (tokenEh("[")) {
            casar("[");
            parseExpressao();
            casar("]");
        }
    }

    casar(")");
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

// -----------------------------------------------------------------
// expressoes
// -----------------------------------------------------------------
//
// Essa parte segue a mesma logica do slide da disciplina pra tirar a
// recursao a esquerda (E -> T E', E' -> + T E' | eps). Em vez de
// criar um nao-terminal extra so pra "empilhar mais um +T", usamos um
// "while": funciona exatamente igual, mas evita ficar criando
// nao-terminais artificiais (tipo E', T') so pra satisfazer a regra
// de "sem recursao a esquerda".
//
// A ordem das funcoes abaixo (expressao -> expr_e -> expr_rel ->
// expr_arit -> termo -> fator) e o que define a PRECEDENCIA dos
// operadores: quem esta "mais embaixo" na cadeia de chamadas (fator)
// e calculado primeiro, entao * e / tem mais precedencia que + e -,
// que por sua vez tem mais precedencia que os relacionais (<, =...),
// que tem mais precedencia que E, que tem mais precedencia que OU.

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

// So pode existir NO MAXIMO um operador relacional por expressao (a
// linguagem nao permite "a < b < c"), por isso aqui usamos "if" e nao
// "while" como nas outras regras de expressao.
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

// fator e o "nivel mais baixo" da expressao: um valor sozinho, uma
// expressao entre parenteses, um numero negativo, um identificador
// (que pode ser variavel, posicao de vetor ou chamada de funcao),
// numero, cadeia ou verdadeiro/falso.
void parseFator(void)
{
    if (tokenEh("(")) {
        casar("(");
        parseExpressao();
        casar(")");
    }
    else if (tokenEh("-")) {
        casar("-");
        parseFator(); // permite "- - x", nao so "-x"
    }
    else if (tokenAtual.type == TOKEN_ID) {
        casarTipo(TOKEN_ID, "esperado identificador");
        if (tokenEh("[")) {
            casar("[");
            parseExpressao();
            casar("]");
        }
        else if (tokenEh("(")) {
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