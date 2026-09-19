#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAM_MAX 1024

typedef struct {
    //TokenNome type; // nome ddo token
    int line;   // tratamento ddde erro

    union {
        int table_index; //indice tabela dde simbolo
        int int_value; //valor literal convertido
        double float_value; //valor literal floar conmvertido
        //OpRelType op_code; //operador relacional especifico
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

const char *palavrasFixas[] = {
    "algoritmo", "var", "inicio", "fimalgoritmo",
    "caractere", "inteiro", "real", "logico",
    "verdadeiro", "falso",
    "leia", "escreva", "escreval"
    "se", "entao", "senao", "fimse"
    "para", "de", "ate", "passo", "faca", "fimpara",
    "enquanto", "fimenquanto",
    "vetor",
    "procedimento", "fimprocedimento",
    "funcao", "fimfuncao", "retorne"
    "MOD", "E", "OU",
    "<-", "+", "-", "*", "/", "(", ")", "[", "]", ":", ",", ".."
};
#define qtd_fixos (sizeof(palavrasFixas)/sizeof(char))



int main() {
    FILE *entrada, *saida;
    char linha[TAM_MAX];
    char palavra[TAM_MAX];
    int tam_palavra;

    entrada = fopen("linguagem_teste.txt", "r");
    if (entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        return 1;
    }

    saida = fopen("saida.txt", "w");
    if (saida == NULL) {
        printf("Erro ao criar o arquivo de saida.\n");
        fclose(entrada);
        return 1;
    }

    while (fgets(linha, TAM_MAX, entrada) != NULL) {
        tam_palavra = 0;

        for (int i = 0; linha[i] != '\0'; i++) {
            char c = linha[i];

            if (isspace(c)) {
                // espaco/tab/quebra de linha: fecha a palavra atual, se houver
                if (tam_palavra > 0) {
                    palavra[tam_palavra] = '\0';
                    fprintf(saida, "%s\n", palavra);
                    tam_palavra = 0;
                }
            }
            // else if (ehPontuacao(c)) {
            //     // fecha a palavra atual, se houver
            //     if (tam_palavra > 0) {
            //         palavra[tam_palavra] = '\0';
            //         fprintf(saida, "%s\n", palavra);
            //         tam_palavra = 0;
            //     }
            //     // escreve a pontuacao como token individual
            //     fprintf(saida, "%c\n", c);
            // }
            else {
                // caractere normal: acumula na palavra atual
                palavra[tam_palavra++] = c;
            }
        }

        // fecha a ultima palavra da linha, se houver
        if (tam_palavra > 0) {
            palavra[tam_palavra] = '\0';
            fprintf(saida, "%s\n", palavra);
            tam_palavra = 0;
        }
    }

    fclose(entrada);
    fclose(saida);

    printf("Tokens escritos com sucesso!\n");

    return 0;
}