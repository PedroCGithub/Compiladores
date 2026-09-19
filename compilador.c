#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_MAX 1024

typedef struct {
    TokenNome type; // nome ddo token
    int line;   // tratamento ddde erro

    union {
        int table_index; //indice tabela dde simbolo
        int int_value; //valor literal convertido
        double float_value; //valor literal floar conmvertido
        OpRelType op_code; //operador relacional especifico
    } attribute
} Token;

typedef enum {
    TOKEN_EOF = 0, 
    TOKEN_ID, //identificadores, variaveis/funcoes
    TOKEN_NUM_INT, //numeros int
    TOKEN_NUM_FLOAT, //numeros reais
    TOKEN_OP_REL, //operadores relacionais
    TOKEN_KEYWORD // palavras reservadas
} TokenNome;

typedef enum {
    OP_LT, // <
    OP_LE, // <=
    OP_EQ, // ==
    OP_GT, // >
    OP_GE // >=
} OpRelAtributo;

int main() {
    FILE *entrada, *saida;
    char linha[TAM_MAX];
    char *token;

    // Abre o arquivo de entrada para leitura
    entrada = fopen("linguagem_teste.txt", "r");
    if (entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        return 1;
    }

    // Abre (ou cria) o arquivo de saida para escrita
    saida = fopen("saida.txt", "w");
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida.\n");
        fclose(entrada);
        return 1;
    }
    fclose(entrada);
    fclose(saida);
}