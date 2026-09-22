#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAM_MAX 1024

/* ============================================================================
 * TODO 1 - Definir a struct Token com TokenNome type, int line e a union
 * de atributos (conforme Figura 2 do enunciado)
 * ----------------------------------------------------------------------------
 * O QUE FAZER: a struct precisa guardar:
 *   (a) o TIPO do token — para o parser (Etapa#3) saber se é um
 *       identificador, um número, um operador etc;
 *   (b) a LINHA onde ele apareceu — para as mensagens de erro;
 *   (c) o VALOR do atributo, que muda de significado dependendo do tipo
 *       do token — por isso é uma union: ela ocupa o espaço de só UM
 *       desses campos por vez (economiza memória, já que nunca vai
 *       precisar de int_value E float_value ao mesmo tempo, por exemplo).
 *
 * ERRO NO SEU CÓDIGO:
 *   - A linha `//TokenNome type;` está COMENTADA. Sem esse campo, um Token
 *     não tem como dizer "eu sou um identificador" ou "eu sou um número".
 *     Isso vai quebrar a Etapa#3 inteira, porque o parser precisa
 *     perguntar "qual o tipo do token atual?" o tempo todo. Descomente
 *     essa linha.
 *   - `//OpRelType op_code;` também está comentado, mas o enunciado pede
 *     um campo pra guardar qual operador relacional específico foi lido
 *     (<, <=, =, <>, >, >=). Sem ele, TOKEN_OP_REL não tem como carregar
 *     essa informação. Repare que o TIPO usado aqui também está errado:
 *     você criou o enum como `OpRelAtributo` (linha de baixo), mas
 *     comentou o campo como `OpRelType` — nome que não existe no seu
 *     código. Precisa ser `OpRelAtributo op_code;`.
 * ==========================================================================*/
typedef struct {
    //TokenNome type; // nome ddo token
    int line;   // tratamento ddde erro

    union {
        int table_index; //indice tabela dde simbolo
        int int_value; //valor literal convertido
        double float_value; //valor literal floar conmvertido
        //OpRelType op_code; //operador relacional especifico
    } attribute;
} Token;

//  Implementar o enum TokenNome (TOKEN_EOF, TOKEN_ID, TOKEN_NUM_INT, TOKEN_NUM_FLOAT, TOKEN_OP_REL, TOKEN_KEYWORD)
/* Esta parte está OK. Só um detalhe pra pensar: o Anexo I também tem
 * STRINGS (ex: "Olá, mundo!") e SÍMBOLOS soltos (parênteses, colchetes,
 * vírgula, "..", "<-"). Nenhum desses cabe perfeitamente em TOKEN_KEYWORD
 * nem em TOKEN_OP_REL. Vale decidir se você vai tratá-los como
 * TOKEN_KEYWORD "genérico" ou se vai adicionar categorias novas ao enum
 * (ex: TOKEN_STRING, TOKEN_SIMBOLO) — sem isso, algum lexema vai ficar
 * sem "casa" no seu tipo de token. */
typedef enum {
    TOKEN_EOF = 0, 
    TOKEN_ID, //identificadores, variaveis/funcoes
    TOKEN_NUM_INT, //numeros int
    TOKEN_NUM_FLOAT, //numeros reais
    TOKEN_OP_REL, //operadores relacionais
    TOKEN_KEYWORD // palavras reservadas
} TokenNome;

//  Implementar o enum OpRelAtributo (OP_LT, OP_LE, OP_EQ, OP_GT, OP_GE)
/* Esta parte também está OK — os 5 sub-códigos batem com o que o
 * enunciado pede (Figura 2). É esse enum que o campo `op_code` da union
 * (item acima) deveria usar. */
typedef enum {
    OP_LT, // <
    OP_LE, // <=
    OP_EQ, // ==
    OP_GT, // >
    OP_GE // >=
} OpRelAtributo;

/* ============================================================================
 * TODO 3 (implícito) - Lista de palavras reservadas / símbolos da linguagem
 * ----------------------------------------------------------------------------
 * O QUE FAZER: listar as palavras-chave da linguagem para que, ao ler um
 * identificador, você consiga checar "isso é uma palavra reservada ou é
 * um nome de variável?".
 *
 * ERROS NO SEU CÓDIGO (bem sutis, e por isso perigosos):
 *   1) FALTAM VÍRGULAS entre vários elementos:
 *        "escreval"          "se"
 *        "fimse"             "para"
 *        "retorne"           "MOD"
 *      Em C, duas strings literais lado a lado SEM vírgula são
 *      CONCATENADAS automaticamente pelo compilador — não dá erro, dá
 *      um bug silencioso. Ou seja, "escreval" "se" na prática vira UM
 *      elemento só: "escrevalse". Isso significa que, no seu array atual,
 *      as palavras "escreval", "se", "fimse", "para" e "retorne" NÃO
 *      EXISTEM como strings separadas — elas viraram "escrevalse",
 *      "fimsepara" e "retorneMOD". Qualquer programa MiniVisualg que use
 *      "se" ou "para" vai falhar em ser reconhecido como palavra
 *      reservada.
 *   2) Você misturou PALAVRAS RESERVADAS com SÍMBOLOS/OPERADORES no
 *      mesmo array ("<-", "+", "-", "(", ")", ".." etc). Isso é
 *      problemático porque identificadores e palavras reservadas são
 *      lidos de um jeito (sequência de letras/dígitos), e símbolos são
 *      lidos de outro jeito (um ou dois caracteres de pontuação). Se
 *      você tentar comparar um símbolo lido contra esse array junto com
 *      as palavras, sua lógica de leitura vai precisar ficar mais
 *      confusa do que precisa. Vale separar em duas listas: uma de
 *      palavras reservadas (para comparar depois de ler um identificador
 *      completo) e outra de símbolos (tratados caractere a caractere).
 *   3) O nome do array, `palavrasFixas`, não é usado em nenhuma outra
 *      parte do código ainda (nenhuma função consulta essa lista) —
 *      então, mesmo corrigindo as vírgulas, falta a função que de fato
 *      PERCORRE esse array e compara com o lexema lido.
 * ==========================================================================*/
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
/* ============================================================================
 * ERRO: `sizeof(palavrasFixas) / sizeof(char)`
 * ----------------------------------------------------------------------------
 * `palavrasFixas` é um array de PONTEIROS (`const char *`), não um array
 * de `char`. `sizeof(char)` é sempre 1 byte. Então essa conta não dá "quantos
 * elementos existem no array" — ela dá "quantos BYTES o array ocupa",
 * porque está dividindo pelo tamanho errado. Em uma máquina de 64 bits,
 * cada ponteiro ocupa 8 bytes, então qtd_fixos vai ficar 8x maior do que
 * deveria. A forma correta é dividir pelo tamanho de UM ELEMENTO do
 * array, ou seja, `sizeof(palavrasFixas[0])` (o tamanho de um
 * `const char *`), não `sizeof(char)`.
 * ==========================================================================*/
#define qtd_fixos (sizeof(palavrasFixas)/sizeof(char))

/* ============================================================================
 * TODO 4 - Implementar a função obterToken() do módulo Analisador Léxico
 * (scanner), lendo char *buffer
 * ----------------------------------------------------------------------------
 * ESTA FUNÇÃO NÃO EXISTE NO SEU CÓDIGO — só tem o comentário do que
 * fazer, mas a implementação nunca foi escrita. É a peça central de todo
 * o analisador léxico, e é ela que falta pra ligar tudo (struct Token,
 * enums, palavras reservadas, tabela de símbolos) numa função só.
 *
 * O QUE FAZER (alto nível, sem escrever o código):
 *   - A assinatura precisa receber um `char *buffer` (o conteúdo do
 *     arquivo fonte já carregado em memória) e alguma forma de lembrar
 *     "por onde eu parei" entre uma chamada e outra — normalmente um
 *     ponteiro para a posição atual (ex: `int *pos`) e um ponteiro para
 *     a linha atual (`int *linha`), já que o parser vai chamar
 *     obterToken() repetidas vezes, uma por token, e cada chamada
 *     precisa continuar de onde a anterior parou.
 *   - Ela deve devolver um `Token` preenchido (type, line, attribute).
 *   - Dentro dela, você precisa: pular espaços/quebras de linha
 *     (atualizando o contador de linha a cada '\n'); decidir se o
 *     próximo caractere começa uma palavra (letra), um número (dígito),
 *     um símbolo, ou se chegou ao fim do buffer.
 *   - Note que HOJE seu programa nem carrega o arquivo inteiro num
 *     buffer — ele lê linha a linha com `fgets` (veja o `main` mais
 *     abaixo). Pra existir um `obterToken(char *buffer, ...)` de
 *     verdade, seguindo o diagrama do enunciado, é preciso primeiro
 *     mudar a forma de leitura do arquivo: carregar tudo de uma vez
 *     num buffer (com `fopen` + `fseek`/`ftell` para saber o tamanho,
 *     ou `malloc` + `fread`), em vez de processar linha por linha.
 * ==========================================================================*/



int ehPontuacao(char c) {
    return (c == ',' || c == '.' || c == '(' || c == ')' ||
            c == ';' || c == ':' || c == '!' || c == '?');
}
/* ============================================================================
 * ERRO/LIMITAÇÃO em ehPontuacao():
 * ----------------------------------------------------------------------------
 *   - Faltam vários símbolos que aparecem no Anexo I: colchetes `[` `]`
 *     (usados em vetores), o `<-` de atribuição, os operadores
 *     aritméticos `+ - * / \`, os operadores relacionais `< <= = <> > >=`,
 *     e o `..` de intervalo de vetor (`vetor[1..3]`). Do jeito que está,
 *     nenhum desses viraria um token próprio — eles cairiam no "else"
 *     do seu laço principal e ficariam GRUDADOS na palavra ao lado
 *     (ex: "n1+n2" tentaria virar uma palavra só, já que nem espaço nem
 *     `ehPontuacao` reconhecem o `+`).
 *   - `;` e `!` e `?` estão na lista mas não aparecem em nenhum exemplo
 *     do Anexo I — não é exatamente um erro, mas vale conferir se são
 *     realmente usados no MiniVisualg ou se sobraram de outra linguagem.
 * ==========================================================================*/

/* ============================================================================
 * Os TODOs abaixo também não têm nenhuma implementação no seu código
 * ainda — só os comentários. Deixo a explicação de cada um:
 * ==========================================================================*/

//  Implementar leitura do arquivo fonte informado por linha de comando (argv)
/* ERRO: o main() abaixo abre sempre "linguagem_teste.txt", um nome FIXO
 * no código (`fopen("linguagem_teste.txt", "r")`). O enunciado exige que
 * o nome do arquivo venha por linha de comando (`argv[1]`), ou seja,
 * `main` precisa receber `(int argc, char *argv[])` — hoje ele é
 * `int main()`, sem parâmetros nenhum, então não tem como acessar o
 * nome do arquivo digitado pelo usuário. */

//  Tratar tokenização considerando que lexemas estão separados por espaço
/* CUIDADO com essa suposição: olhando o Anexo I, tem STRINGS com espaço
 * DENTRO das aspas, por exemplo `escreva("Digite a nota do aluno ", i, ...)`.
 * O seu laço atual (dentro do `while (fgets...)`) fecha a "palavra" toda
 * vez que encontra um espaço — isso vai quebrar a string
 * "Digite a nota do aluno " em vários tokens errados ("Digite", "a",
 * "nota"...), quando deveria ser UM token só. Você vai precisar de um
 * tratamento especial pra tudo que estiver entre aspas, tratando o
 * conteúdo como um bloco único independente de ter espaços dentro. */

//  Gerar tabela de símbolos (para identificadores) e associar table_index ao token
/* ESTA PARTE NÃO EXISTE NO SEU CÓDIGO ainda. Falta: um array (ou outra
 * estrutura) pra guardar os nomes de identificadores já vistos, e uma
 * função que, ao ler um identificador, primeiro CHECA se ele já está
 * nessa lista (reaproveitando o índice se já existir) e só insere um
 * novo se for a primeira vez. É esse índice que deveria ir para
 * `attribute.table_index` no Token. */

//  Implementar conversão de literais (inteiro/real) e preenchimento de int_value / float_value
/* ESTA PARTE TAMBÉM NÃO EXISTE. No `main` atual, tudo que é lido vira uma
 * string e é jogado direto pro arquivo de saída (`fprintf(saida, "%d# %s\n", ...)`)
 * — nenhum número é de fato CONVERTIDO para `int` ou `double` (ex: usando
 * `atoi`/`atof` ou equivalente) nem guardado em `int_value`/`float_value`.
 * Também falta decidir COMO diferenciar um número inteiro (`42`) de um
 * real (`3.14`) ao ler os dígitos — hoje o código não olha pra isso, ele
 * só separa "palavras" por espaço/pontuação, sem examinar o conteúdo. */

//  Implementar saída no formato: NúmeroDaLinha# NomeToken | Atributo (tanto em arquivo quanto na tela)
/* ERRO: o formato de saída atual é `fprintf(saida, "%d# %s\n", num_linha, palavra)`
 * — ele imprime o LEXEMA BRUTO (a palavra como apareceu no código), não o
 * NOME DO TOKEN (ex: "IDENTIFICADOR", "NUM_INT") nem o atributo separado
 * por "|", como o enunciado pede (`11# IDENTIFICADOR | 1`). Também falta
 * o `printf` correspondente pra tela — hoje só se escreve no arquivo
 * `saida`, nunca no console. Além disso, essa parte do código nunca
 * chega a saber o TIPO do token (porque não existe `obterToken()` nem o
 * campo `type` na struct), então não tem como montar essa saída do jeito
 * pedido ainda. */

//  Implementar tratamento de erro léxico: imprimir "ERRO LÉXICO", linha e sequência errada, e encerrar o processo
/* ESTA PARTE NÃO EXISTE. Hoje, qualquer caractere que não seja espaço
 * nem pontuação reconhecida por `ehPontuacao` é silenciosamente
 * acumulado na "palavra" — não existe nenhum caminho no código que
 * detecte "essa sequência é inválida" e pare a execução. Falta decidir
 * quais caracteres são válidos na linguagem e, ao encontrar algo fora
 * disso, imprimir a mensagem de erro no formato pedido e chamar algo
 * como `exit(1)` pra encerrar o programa. */

//  Testar com todos os exemplos do Anexo I (esqueleto, comentários, variáveis, operadores, estruturas de controle, repetição, vetores, procedimentos, funções)
/* Ainda não dá pra testar de verdade porque faltam as partes acima. Um
 * ponto de atenção pra quando for testar: seu código também não trata
 * COMENTÁRIOS (`// ...`) — olhando o `main` atual, o `//` seria lido
 * como dois símbolos soltos e o texto do comentário viraria token
 * normal, quando deveria ser todo descartado até o fim da linha. */

//  Compilar com gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador e garantir zero warnings
/* Ainda não testei a compilação deste arquivo específico — como pedido,
 * não fiz nenhuma alteração de código, só comentários. Rode esse comando
 * você mesmo depois de implementar cada TODO e confira se aparece algum
 * warning (ex: variável declarada e não usada, comparação de tipos
 * incompatíveis) — cada warning custa 1 ponto, segundo o critério de
 * avaliação. */

//  Escrever readme.txt da Etapa#2 (o que foi feito, como executar, bugs conhecidos, decisões de design)
/* Isso é um arquivo separado (readme.txt), não faz parte do .c — só
 * lembrando que ele também é obrigatório na entrega. */

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
    int num_linha = 1;
    while (fgets(linha, TAM_MAX, entrada) != NULL) {
        tam_palavra = 0;

        for (int i = 0; linha[i] != '\0'; i++) {
            char c = linha[i];

            if (isspace(c)) {
                // espaco/tab/quebra de linha: fecha a palavra atual, se houver
                if (tam_palavra > 0) {
                    palavra[tam_palavra] = '\0';
                    fprintf(saida, "%d# %s\n", num_linha, palavra);
                    tam_palavra = 0;
                }
            }
            else if (ehPontuacao(c)) {
                // fecha a palavra atual, se houver
                if (tam_palavra > 0) {
                    palavra[tam_palavra] = '\0';
                    fprintf(saida, "%d# %s\n", num_linha, palavra);
                    tam_palavra = 0;
                }
                // escreve a pontuacao como token individual
                fprintf(saida, "%d# %c\n", num_linha, c);
                
            }
            else {
                // caractere normal: acumula na palavra atual
                palavra[tam_palavra++] = c;
            }
        }

        // fecha a ultima palavra da linha, se houver
        if (tam_palavra > 0) {
            palavra[tam_palavra] = '\0';
            fprintf(saida, "%d# %s\n", num_linha, palavra);
            tam_palavra = 0;
        }
        num_linha++;
    }

    fclose(entrada);
    fclose(saida);

    printf("Tokens escritos com sucesso!\n");

    return 0;
}
/* ============================================================================
 * ERRO GERAL no main() acima:
 * ----------------------------------------------------------------------------
 *   - `int main()` sem parâmetros: não recebe o nome do arquivo por
 *     linha de comando (veja o TODO de argv acima).
 *   - Nunca usa `Token`, `TokenNome`, `OpRelAtributo`, nem
 *     `palavrasFixas` — o laço de leitura trabalha só com strings
 *     brutas (`palavra`), sem nunca classificar o que foi lido. Ou seja,
 *     as duas metades do arquivo (as definições de tipos lá em cima e a
 *     lógica de leitura aqui embaixo) ainda não estão conectadas.
 *   - Não há chamada a `obterToken()` porque ela não existe — o main
 *     reimplementa uma tokenização simples "na mão", diferente do que o
 *     enunciado pede (uma função `obterToken()` reutilizável, chamada
 *     pelo parser via `nextToken()` na Etapa#3).
 * ==========================================================================*/