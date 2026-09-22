#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define TAM_MAX 1024

int ehPontuacao(char c) {
    return (c == ',' || c == '.' || c == '(' || c == ')' ||
            c == ';' || c == ':' || c == '!' || c == '?');
}

int main() {
    FILE *entrada, *saida;
    char linha[TAM_MAX];
    char palavra[TAM_MAX];
    int tam_palavra;

    entrada = fopen("entrada.txt", "r");
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
            else if (ehPontuacao(c)) {
                // fecha a palavra atual, se houver
                if (tam_palavra > 0) {
                    palavra[tam_palavra] = '\0';
                    fprintf(saida, "%s\n", palavra);
                    tam_palavra = 0;
                }
                // escreve a pontuacao como token individual
                fprintf(saida, "%c\n", c);
            }
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