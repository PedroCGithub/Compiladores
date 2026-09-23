// ====================================================================
//  COMPILADORES - PROJETO Fase 1 - MiniVisualg
//  Etapa 2 (Analisador Lexico) + Etapa 3 (Analisador Sintatico)
//
//  Integrantes:
//    - Henrique Ferreira Marciano - RA 10439797
//    - Pedro Casas Pequeno Junior - RA 10437031
//    - Pedro Gabriel Guimarães Fernandes - RA 10437465

//
//   Professora as explicacoes da gramatica e das decisoes de projeto estao no arquivo readme.txt.
//
//  Compilar: gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
//  Executar: ./compilador programa.txt
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

// ====================================================================
// Funcoes do analisador lexico (Etapa 2)
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
// Funcoes do analisador sintatico (Etapa 3)
// Uma funcao por nao-terminal da gramatica (ver README, secao 3).
// ====================================================================

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

    avancar(); // pega o primeiro token (nextToken)
    parsePrograma();

    if (tokenAtual.type != TOKEN_EOF) {
        erroSintatico(tokenAtual.lexema, "codigo inesperado depois de 'fimalgoritmo'");
    }

    printf("\n>>> Programa aceito: a cadeia de tokens pertence a linguagem MiniVisualg.\n");

    fecharAnalisador();
    return 0;
}

// ====================================================================
// Analisador lexico (Etapa 2)
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

// ====================================================================
// Analisador sintatico (Etapa 3)
// ====================================================================

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

// -- programa / subprogramas / declaracoes -----------------------------

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
    casar("procedimento");
    casarTipo(TOKEN_ID, "esperado o nome do procedimento");
    parseParametrosOpc();
    casar("inicio");
    parseListaComandos();
    casar("fimprocedimento");
}

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

void parseParametro(void)
{
    casarTipo(TOKEN_ID, "esperado nome de parametro");
    casar(":");
    parseTipo();
}

void parseSecaoVar(void)
{
    if (tokenEh("var")) {
        casar("var");
        parseListaDeclaracoes();
    }
}

void parseListaDeclaracoes(void)
{
    while (tokenAtual.type == TOKEN_ID) {
        parseDeclaracao();
    }
}

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
        parseTipo();
    }
    else {
        erroSintatico(tokenAtual.lexema, "esperado um tipo (inteiro, real, caractere, logico ou vetor)");
    }
}

// -- comandos -----------------------------------------------------------

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
// em vetor) e chamada (com ou sem argumentos) -- ver README secao 4.4
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
    // nenhum dos dois: chamada de procedimento sem parametros
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

// -- expressoes -----------------------------------------------------------
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