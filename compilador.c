// ====================================================================
//  COMPILADORES - PROJETO Fase 1 - MiniVisualg
//  Etapa 2: Analisador Lexico
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

#define TAM_LEXEMA 256
#define MAX_SIMBOLOS 500

// ====================================================================
// Definicao de tipos
// ====================================================================

//movido pra cima pois precisa ser declarado antes de ser usado
//acrescentei TOKEN_STRING (cadeias entre aspas) e TOKEN_SIMBOLO (<- + - * / \ ( ) [ ] : , ..) pois nao cabia em nenhum outro
typedef enum {
    TOKEN_EOF = 0, 
    TOKEN_ID, //identificadores, variaveis/funcoes
    TOKEN_NUM_INT, //numeros int
    TOKEN_NUM_FLOAT, //numeros reais
    TOKEN_OP_REL, //operadores relacionais
    TOKEN_KEYWORD, // palavras reservadas
    TOKEN_STRING, //cadeias de palavras (adicionado)
    TOKEN_SIMBOLO //atribuição, aritmeticos e delimitadores (adicionado)
} TokenNome;

//nomes dos tipos, na mesma ordem do enum
const char *nomesTokens[] = {
    "EOF", "ID", "NUM_INT", "NUM_FLOAT", "OP_REL",
    "KEYWORD", "STRING", "SIMBOLO"
};

typedef enum {
    OP_LT, // <
    OP_LE, // <=
    OP_EQ, // =
    OP_GT, // >
    OP_GE // >=
} OpRelAtributo;

const char *nomesOpRel[] = { "OP_LT", "OP_LE", "OP_EQ", "OP_GT", "OP_GE" };

//acrescentei char lexema pra imprimir as strings e facilitar o parser da etapa 3
typedef struct {
    TokenNome type; // nome ddo token
    int line;   // tratamento ddde erro
    char lexema[TAM_LEXEMA]; //tamanho original do texto

    union {
        int table_index; //indice tabela dde simbolo
        int int_value; //valor literal convertido
        double float_value; //valor literal floar conmvertido
        OpRelAtributo op_code; //operador relacional especifico
    } attribute;
} Token;

//lista de palavras corrijida e troquei para palavras reservadas e simbolos serem separados para ficar mais facil de usar
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

//os simbolos de 2 caracteres vem primeiro, para "<-" nao ser lido  como "<" seguido de "-"
//os relacionais ficam fora desta lista porque tem tratamento proprio (precisam do op_code)
const char *simbolos[] = {
    "<-", "..",
    "+", "-", "*", "/", "\\",
    "(", ")", "[", "]", ":", ","
};
#define QTD_SIMBOLOS (sizeof(simbolos) / sizeof(simbolos[0]))

// ====================================================================
// Funcoes principais
// ====================================================================
void iniciarAnalisador(FILE *arquivo);
int fimDoArquivo();
Token obterToken();
TokenNome classificarLexema(char *lexema);
void imprimirToken(Token token);
void fecharAnalisador();

// ====================================================================
// Funcoes auxiliares
// ====================================================================
int ehDelimitador(int c);
int ehOperador(int c);
int ehLetra(int c);
int ehDigito(int c);
int peek();
void voltar();
void pularEspacosEComentarios();
void guardar(Token *token, int *i, int c);
int inserirSimbolo(char *nome);
void erroLexico(char *sequencia, char *motivo);

// ====================================================================
// Variaveis globais
// ====================================================================

//variaveis globais
FILE *fonte;            // arquivo fonte
int linhaAtual = 1;     // linha atual do arquivo fonte
FILE *saida = NULL; //arquivo de tokens
char tabelaSimbolos[MAX_SIMBOLOS][TAM_LEXEMA]; //identificadores
int  qtdSimbolos = 0; 

// ====================================================================
// main
// ====================================================================
int main(int argc, char *argv[])
{
#ifdef _WIN32
    system("chcp 65001 > nul");   // mostra acentos no terminal do Windows
#endif

    // 1. Abrir arquivo fonte (nome vem da linha de comando)
    if (argc < 2) {
        printf("Uso: compilador <arquivo_fonte>\n");
        return 0;
    }
    fonte = fopen(argv[1], "rb");
    if (fonte == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return 0;
    }

    // 2. Inicializar analisador
    iniciarAnalisador(fonte);

    // 3. Loop principal de analise
    while (!fimDoArquivo()) {
        Token token = obterToken();   // 4. Extrair proximo token
        imprimirToken(token);         // 5. Exibir token formatado
    }

    // 6. Finalizar analisador
    fecharAnalisador();
    return 0;
}

// ====================================================================
// 1. iniciarAnalisador
//    Objetivo: prepara as variaveis e abre o arquivo de saida
//    Recebe: ponteiro para o arquivo fonte
//    Retorna: nada
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

// ====================================================================
// 2. fimDoArquivo
//    Objetivo: verifica se o fim do arquivo foi alcancado
//    (antes pula espacos e comentarios que sobraram no final)
//    Recebe: nada
//    Retorna: 1 (verdadeiro) ou 0 (falso)
// ====================================================================
int fimDoArquivo()
{
    pularEspacosEComentarios();
    return peek() == EOF;
}

// ====================================================================
// 3. obterToken  (e o "proximoToken" dos slides; o enunciado pede esse nome)
//    Objetivo: le caracteres do arquivo e forma o proximo token
//    Recebe: nada
//    Retorna: estrutura Token
// ====================================================================
Token obterToken()
{
    Token token;
    int c;
    int i = 0;

    // Ignorar espacos e comentarios
    pularEspacosEComentarios();

    token.line = linhaAtual;
    token.attribute.int_value = 0;

    c = fgetc(fonte);

    // Fim do arquivo
    if (c == EOF) {
        token.type = TOKEN_EOF;
        strcpy(token.lexema, "EOF");
        return token;
    }

    // Verifica tipo de lexema
    if (ehLetra(c)) {
        // Identificador ou palavra reservada
        guardar(&token, &i, c);
        while (ehLetra(peek()) || ehDigito(peek()) || peek() == '_') {
            guardar(&token, &i, fgetc(fonte));
        }
    }
    else if (ehDigito(c)) {
        // Numero inteiro
        guardar(&token, &i, c);
        while (ehDigito(peek())) {
            guardar(&token, &i, fgetc(fonte));
        }

        // Numero real: ponto seguido de digito (ex: 1.60)
        if (peek() == '.') {
            fgetc(fonte);               // le o ponto
            if (ehDigito(peek())) {
                guardar(&token, &i, '.');
                while (ehDigito(peek())) {
                    guardar(&token, &i, fgetc(fonte));
                }
            } else {
                voltar();               // era o ".." de vetor[1..3]: devolve o ponto
            }
        }

        // Numero grudado em letra (ex: 2abc) e erro
        if (ehLetra(peek()) || peek() == '_') {
            while (ehLetra(peek()) || ehDigito(peek()) || peek() == '_') {
                guardar(&token, &i, fgetc(fonte));
            }
            token.lexema[i] = '\0';
            erroLexico(token.lexema, "numero malformado");
        }
    }
    else if (c == '"') {
        // Cadeia: tudo ate a proxima aspa, mesmo com espacos
        guardar(&token, &i, c);
        while (peek() != '"') {
            if (peek() == '\n' || peek() == EOF) {
                token.lexema[i] = '\0';
                erroLexico(token.lexema, "cadeia nao foi fechada");
            }
            guardar(&token, &i, fgetc(fonte));
        }
        guardar(&token, &i, fgetc(fonte));   // aspa de fechamento
    }
    else if (ehOperador(c)) {
        // Operador relacional, aritmetico ou atribuicao
        guardar(&token, &i, c);
        // operadores de dois caracteres: <= <- >=
        if ((c == '<' && (peek() == '=' || peek() == '>' || peek() == '-'))
            || (c == '>' && peek() == '=')) {
            guardar(&token, &i, fgetc(fonte));
        }
    }
    else if (ehDelimitador(c)) {
        // Delimitador ( ) [ ] : , ..
        guardar(&token, &i, c);
        if (c == '.') {
            // o ponto sozinho nao existe, so ".."
            if (peek() != '.') {
                token.lexema[i] = '\0';
                erroLexico(token.lexema, "ponto sozinho (esperado ..)");
            }
            guardar(&token, &i, fgetc(fonte));
        }
    }
    else {
        // Caractere que nao pertence a linguagem
        guardar(&token, &i, c);
        token.lexema[i] = '\0';
        erroLexico(token.lexema, "caractere nao pertence a linguagem");
    }

    token.lexema[i] = '\0';

    // Classificar lexema
    token.type = classificarLexema(token.lexema);

    // Preencher o atributo
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
    }

    return token;
}

// ====================================================================
// 4. classificarLexema
//    Objetivo: determina o tipo do token com base no lexema
//    Recebe: string do lexema
//    Retorna: tipo do token (enum)
// ====================================================================
TokenNome classificarLexema(char *lexema)
{
    int i;

    // Palavras reservadas
    for (i = 0; i < (int) QTD_PALAVRAS; i++) {
        if (strcmp(lexema, palavrasReservadas[i]) == 0)
            return TOKEN_KEYWORD;
    }

    // Operadores relacionais
    if (strcmp(lexema, "<") == 0 || strcmp(lexema, "<=") == 0 ||
        strcmp(lexema, "=") == 0 ||strcmp(lexema, ">") == 0 || 
        strcmp(lexema, ">=") == 0)
        return TOKEN_OP_REL;

    // Atribuicao, operadores aritmeticos e delimitadores
    for (i = 0; i < (int) QTD_SIMBOLOS; i++) {
        if (strcmp(lexema, simbolos[i]) == 0)
            return TOKEN_SIMBOLO;
    }

    // Cadeia (comeca com aspas)
    if (lexema[0] == '"')
        return TOKEN_STRING;

    // Numero (comeca com digito): real se tiver ponto
    if (ehDigito(lexema[0])) {
        if (strchr(lexema, '.') != NULL)
            return TOKEN_NUM_FLOAT;
        return TOKEN_NUM_INT;
    }

    // Identificador
    return TOKEN_ID;
}

// ====================================================================
// 5. imprimirToken
//    Objetivo: exibe o token no formato  linha# TIPO | atributo
//    (na tela e no arquivo saida.txt)
//    Recebe: estrutura Token
//    Retorna: nada
// ====================================================================
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

// ====================================================================
// 6. Funcoes auxiliares para classificar caracteres
//    Recebem: um caractere
//    Retornam: 1 (verdadeiro) ou 0 (falso)
// ====================================================================
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

// ====================================================================
// Outras funcoes auxiliares
// ====================================================================

// Olha o proximo caractere SEM consumir (le e depois volta)
int peek()
{
    int c = fgetc(fonte);
    if (c != EOF) {
        voltar();
    }
    return c;
}

// Volta uma posicao no arquivo (e o "retract()" do slide do automato)
void voltar()
{
    fseek(fonte, -1, SEEK_CUR);
}

// Pula espacos, tabulacoes, quebras de linha e comentarios //
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
            // comentario: ignora tudo ate o fim da linha
            while (c != '\n' && c != EOF) {
                c = fgetc(fonte);
            }
            if (c == '\n') {
                linhaAtual++;
            }
        }
        else {
            // achou o inicio de um token (ou o fim do arquivo)
            if (c != EOF) {
                voltar();
            }
            break;
        }
    }
}

// Coloca o caractere c no lexema do token (sem passar do tamanho maximo)
void guardar(Token *token, int *i, int c)
{
    if (*i >= TAM_LEXEMA - 1) {
        token->lexema[*i] = '\0';
        erroLexico(token->lexema, "lexema muito grande");
    }
    token->lexema[*i] = (char) c;
    (*i)++;
}

// Tabela de simbolos: se o nome ja existe, devolve a posicao dele;
// senao, coloca no final e devolve a posicao nova
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

// Erro lexico: mostra a linha e a sequencia errada e encerra o programa
// (exit(0) porque o enunciado desconta ponto se o retorno nao for 0)
void erroLexico(char *sequencia, char *motivo)
{
    printf("ERRO LÉXICO | linha %d | sequência '%s' | %s\n", linhaAtual, sequencia, motivo);
    fprintf(saida, "ERRO LÉXICO | linha %d | sequência '%s' | %s\n", linhaAtual, sequencia, motivo);
    fecharAnalisador();
    exit(0);
}

// ====================================================================
// 7. fecharAnalisador
//    Objetivo: fecha os arquivos
//    Recebe: nada
//    Retorna: nada
// ====================================================================
void fecharAnalisador()
{
    fclose(fonte);
    fclose(saida);
}

// ====================================================================
// TODO - Etapa 3: analisador sintatico
// TODO - readme.txt
// ====================================================================

// até onde eu entendi, tem que passar as nossas expressões para funções, então vai ter no minimo isso
// link que eu usei para entender isso: https://github.com/lotabout/write-a-C-interpreter/blob/master/tutorial/pt-br/4-Top-down-Parsing.md
// aproveita e guarda isso para colocar no relatorio
// mudei o teste pq n tava rodando na minha maquina com os acentos e ç não sei o pq
/*
expressao   -> expr_e (OU expr_e)*
expr_e      -> expr_rel (E expr_rel)*
expr_rel    -> expr_arit (relop expr_arit)?
expr_arit   -> termo (('+' | '-') termo)*
termo       -> fator (('*' | '/' | '\' | MOD) fator)*
fator       -> '(' expressao ')'
             | '-' fator
             | id ('[' expressao ']' | '(' (expressao (',' expressao)*)? ')')?
             | num_int | num_real | cadeia | verdadeiro | falso
*/
// coloquei etapa 3 no nome só para diferenciar se tiver outra com esse nome
void etapa3_expressao(void) {}
void etapa3_expr_e(void) {}
void etapa3_expr_rel(void) {}
void etapa3_expr_arit(void) {}
void etapa3_termo(void) {}
void etapa3_fator(void) {}

// adaptei o erro lexico para o sintatico
void erroSintatico(char *esperado, char *motivo) {
    printf("ERRO SINTÁTICO | linha %d | token '%s' | motivo: %s\n", linhaAtual, esperado, motivo);
    fprintf(saida, "ERRO SINTÁTICO | linha %d | token '%s' | motivo: %s\n", linhaAtual, esperado, motivo);
    fecharAnalisador(); // acho que aqui continua isso, não precisa mudar
    exit(0);
}

// função para verificar se o proximo token é oq queremos, exemplo: se rodar proxTokenEh("+") ele vai retornar true se o proximo token for + ou false se não for
bool proxTokenEh(char *lexema) {
    if (strcmp(tokenAtual.lexema, lexema) == 0) {
        return true;
    }
    return false;
}
