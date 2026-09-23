====================================================================
COMPILADORES - PROJETO Fase 1 - MiniVisualg
README - Etapas 1, 2 e 3
====================================================================

Integrantes:
  - Henrique Ferreira Marciano - RA 10439797
  - Pedro Casas Pequeno Junior - RA 10437031
  - Pedro Gabriel Guimarães Fernandes - RA 10437465


--------------------------------------------------------------------
1. COMO COMPILAR E EXECUTAR
--------------------------------------------------------------------

Compilar:
    gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador

Executar:
    compilador <arquivo_fonte>

Saida gerada:
    - Impressao na tela de cada token reconhecido, no formato
      "linha# TIPO | atributo" (Etapa 2).
    - Arquivo saida.txt com o mesmo conteudo impresso na tela.
    - Se o programa fonte for sintaticamente valido, a mensagem:
        ">>> Programa aceito: a cadeia de tokens pertence a
        linguagem MiniVisualg."
    - Se houver erro lexico, a mensagem "ERRO LEXICO" com a linha e
      a sequencia invalida, e o programa encerra com exit(0).
    - Se houver erro sintatico, a mensagem "ERRO SINTATICO" com a
      linha e o token que causou o erro, e o programa encerra com
      exit(0).


--------------------------------------------------------------------
2. O QUE FOI IMPLEMENTADO
--------------------------------------------------------------------

Etapa#1 - Gramatica livre de contexto: concluida (ver secao 3).
Etapa#2 - Analisador lexico: concluido.
Etapa#3 - Analisador sintatico: concluido, usando o metodo
          descendente recursivo preditivo.

Os analisadores lexico e sintatico funcionam em conjunto: o parser
chama a funcao avancar(), que e o "nextToken()" citado no enunciado,
e essa funcao chama obterToken() (a funcao do analisador lexico) para
buscar o proximo token sob demanda. Ou seja, o lexico so roda quando
o sintatico pede o proximo token, exatamente como no diagrama do
enunciado (analisador lexico <-> analisador sintatico).


--------------------------------------------------------------------
3. GRAMATICA UTILIZADA (Etapa#1)
--------------------------------------------------------------------

Esta e a mesma gramatica entregue na Etapa#1, usada sem alteracoes
como base do parser da Etapa#3.

3.1 Tokens (expressoes regulares)
----------------------------------
id          -> [a-zA-Z][a-zA-Z0-9_]*
num_int     -> [0-9]+
num_real    -> [0-9]+\.[0-9]+
cadeia      -> "[^"]*"
comentario  -> //[^\n]*        (ignorado)
ws          -> [ \t\n]+        (ignorado)
relop       -> < | <= | = | <> | > | >=
atrib       -> <-
aritop      -> + | - | * | / | \
delim       -> ( | ) | [ | ] | : | , | ..

Palavras reservadas (reconhecidas como "id" e depois conferidas numa
tabela de palavras reservadas):
algoritmo, var, inicio, fimalgoritmo, caractere, inteiro, real,
logico, verdadeiro, falso, leia, escreva, escreval, se, entao, senao,
fimse, para, de, ate, passo, faca, fimpara, enquanto, fimenquanto,
vetor, procedimento, fimprocedimento, funcao, fimfuncao, retorne,
MOD, E, OU

Nota de design: o sinal de "-" nao faz parte da expressao regular de
num_int/num_real. Numero negativo e tratado na gramatica como
operador unario (fator -> '-' fator), evitando ambiguidade entre
subtracao (a - 3.5) e literal negativo (-3.5).

3.2 Estrutura geral do programa
--------------------------------
programa    -> algoritmo cadeia declaracao* inicio comando* fimalgoritmo
declaracao  -> decl_var | decl_proc | decl_func

3.3 Declaracao de variaveis
----------------------------
decl_var   -> var decl_lista+
decl_lista -> id_lista ':' tipo
id_lista   -> id (',' id)*
tipo       -> tipo_base | vetor '[' num_int '..' num_int ']' de tipo_base
tipo_base  -> inteiro | real | caractere | logico

3.4 Procedimentos e funcoes
-----------------------------
decl_proc  -> procedimento id ('(' parametros ')')? inicio comando* fimprocedimento
decl_func  -> funcao id '(' parametros? ')' ':' tipo_base inicio comando* fimfuncao
parametros -> parametro (',' parametro)*
parametro  -> id ':' tipo_base

Procedimentos sem parametro nao usam parenteses -- nem na
declaracao, nem na chamada (ex: "linha_decorativa" no Anexo I).

3.5 Comandos
--------------
comando  -> atribuicao | leitura | escrita | condicional
          | repeticao_para | repeticao_enquanto | chamada | retorno

atribuicao        -> variavel '<-' expressao
variavel          -> id ('[' expressao ']')?
leitura           -> leia '(' variavel ')'
escrita           -> (escreva | escreval) '(' expressao (',' expressao)* ')'
condicional       -> se '(' expressao ')' entao comando* (senao comando*)? fimse
repeticao_para    -> para id de expressao ate expressao (passo expressao)? faca comando* fimpara
repeticao_enquanto -> enquanto '(' expressao ')' faca comando* fimenquanto
chamada           -> id ('(' (expressao (',' expressao)*)? ')')?
retorno           -> retorne expressao

3.6 Expressoes
----------------
Precedencia (do menor para o maior): OU < E < relacional < + - < * / \ MOD < unario

expressao -> expr_e (OU expr_e)*
expr_e    -> expr_rel (E expr_rel)*
expr_rel  -> expr_arit (relop expr_arit)?
expr_arit -> termo (('+' | '-') termo)*
termo     -> fator (('*' | '/' | '\' | MOD) fator)*
fator     -> '(' expressao ')'
           | '-' fator
           | id ('[' expressao ']' | '(' (expressao (',' expressao)*)? ')')?
           | num_int | num_real | cadeia | verdadeiro | falso

"id (...)?" cobre tres casos: variavel simples (nome), acesso a
vetor (nomes[i]) e chamada de funcao (eh_par(num)).

Observacao de design (ja valida desde a Etapa#1): a gramatica nao
possui recursao a esquerda, o que permite implementar um parser
descendente recursivo na Etapa#3 -- cada nao-terminal vira
diretamente uma funcao parse_X() em C.


--------------------------------------------------------------------
4. IMPLEMENTACAO DO PARSER (Etapa#3)
--------------------------------------------------------------------

4.1 Por que analisador descendente recursivo preditivo
----------------------------------------------------------
Cada nao-terminal da gramatica da secao 3 virou uma funcao em C
(parsePrograma, parseComandoSe, parseExpressao, etc). Cada funcao:
  1) confere se o token atual e o esperado naquele ponto da regra
     (funcoes casar()/casarTipo()),
  2) se for, consome o token (chama avancar());
  3) quando a regra pede outro nao-terminal, chama a funcao de
     parsing correspondente -- e por isso o metodo se chama
     "recursivo": uma funcao de parsing chama outra.

Se em algum passo o token nao bater com o esperado, e porque a
cadeia de tokens nao pertence a linguagem -> chama-se
erroSintatico() e o programa e encerrado, seguindo o formato exigido
pelo enunciado ("ERRO SINTATICO", linha e token).

"Preditivo" significa que o parser nunca precisa voltar atras ou
tentar mais de uma alternativa: basta olhar o token atual (1 unico
simbolo de lookahead, por isso a analise e "LL(1)") para saber qual
producao aplicar. Isso so funciona porque a gramatica da secao 3 e:
  - nao ambigua;
  - fatorada a esquerda (nenhum nao-terminal tem duas producoes
    comecando com o mesmo token);
  - sem recursao a esquerda.

4.2 Integracao entre lexico e sintatico (nextToken / obterToken)
---------------------------------------------------------------------
Existe uma variavel global tokenAtual, que e o "1 token de
lookahead" que o parser sempre enxerga. A funcao avancar() faz o
papel do nextToken() citado no enunciado: ela chama obterToken()
(funcao do analisador lexico, sem nenhuma alteracao na sua logica) e
guarda o resultado em tokenAtual, ja imprimindo o token -- por isso
a etapa 2 continua funcionando normalmente, so que agora quem manda
buscar cada token e o proprio parser, token por token, no momento em
que precisa dele.

4.3 Eliminacao de recursao a esquerda nas expressoes (uso de "while")
---------------------------------------------------------------------
A gramatica de expressoes ja vem sem recursao a esquerda (secao
3.6), no formato "expr_arit -> termo (('+'|'-') termo)*". Na
implementacao, cada "(...)*" da gramatica virou um laco "while" na
funcao correspondente (parseExprArit, parseTermo, parseExprE,
parseExpressao). Isso e equivalente ao truque classico de criar um
nao-terminal auxiliar (por exemplo E' -> +T E' | epsilon), so que
sem precisar criar esse nao-terminal extra -- o laco "while" ja
resolve.

A ordem das chamadas entre as funcoes de expressao (parseExpressao
chama parseExprE, que chama parseExprRel, que chama parseExprArit,
que chama parseTermo, que chama parseFator) e o que define a
precedencia dos operadores: o nao-terminal mais "interno" (fator) e
avaliado primeiro, entao * / \ MOD tem mais precedencia que + -, que
por sua vez tem mais precedencia que os operadores relacionais, que
tem mais precedencia que E, que tem mais precedencia que OU -- exatamente
a ordem definida na secao 3.6.

Em expr_rel usamos "if" em vez de "while", porque a gramatica so
permite no maximo um operador relacional por expressao (nao existe
"a < b < c" na linguagem).

4.4 O caso "id" no inicio de um comando (atribuicao vs. chamada)
---------------------------------------------------------------------
A regra "chamada -> id (...)?" e a regra "atribuicao -> variavel
'<-' expressao" comecam com o mesmo token (id). Para decidir qual
das duas se aplica sem violar o LL(1), a funcao
parseAtribuicaoOuChamada() consome o id e so entao olha o proximo
token (ainda apenas 1 token de lookahead) para escolher entre 4
finais possiveis:
    x <- expressao          -> atribuicao simples
    x[i] <- expressao       -> atribuicao em uma posicao de vetor
    f(a, b)                 -> chamada de procedimento/funcao com args
    nome_do_procedimento    -> chamada sem parametros (sem parenteses)

4.5 Bug corrigido no analisador lexico
------------------------------------------
O operador "<>" (usado por exemplo em nome <> "Joao", presente no
Anexo I do enunciado) ja era lido corretamente pelo analisador
lexico como um unico lexema de 2 caracteres (a logica de
obterToken() ja previa isso). Porem, a funcao classificarLexema()
nao tinha "<>" na lista de comparacao dos operadores relacionais, e
por isso ele caia na regra padrao e virava, por engano, um
identificador chamado "<>". Isso foi corrigido: "<>" foi adicionado
a comparacao de operadores relacionais e um novo atributo (OP_NE)
foi criado no enum OpRelAtributo.

4.6 Formato da mensagem de erro sintatico
----------------------------------------------
Seguindo o exigido no enunciado, toda vez que um token nao bate com
o que a gramatica esperava, e impressa a mensagem:
    ERRO SINTATICO | linha <numero> | token '<lexema>' | motivo: <descricao>
tanto na tela quanto no arquivo saida.txt, e o programa encerra com
exit(0) (o proprio enunciado pede retorno 0 mesmo em caso de erro,
para nao ser descontado por warning/retorno diferente de 0).


--------------------------------------------------------------------
5. LIMITACOES CONHECIDAS
--------------------------------------------------------------------

- O parser verifica apenas a SINTAXE (se a cadeia de tokens segue a
  gramatica). Nao ha analise semantica: nao se verifica, por
  exemplo, se uma variavel foi declarada antes de usada, se os tipos
  de uma atribuicao sao compativeis, ou se a quantidade de
  argumentos de uma chamada bate com os parametros declarados.
- Nao e gerada arvore de derivacao (nao foi exigido pelo enunciado
  da Etapa#3, apenas reconhecer a cadeia e emitir erro sintatico
  quando necessario).


--------------------------------------------------------------------
6. TESTES REALIZADOS
--------------------------------------------------------------------

Foram testados manualmente, entre outros:
- Varios exemplos do Anexo I do enunciado (variaveis, comentarios,
  operadores aritmeticos/relacionais/logicos, estrutura de controle,
  repeticao, vetores, procedimentos e funcoes) -- todos aceitos
  corretamente.
- Um programa com expressao incompleta ("x <- 5 +" seguido de
  fimalgoritmo), que corretamente gera ERRO SINTATICO na linha certa.
- Um programa usando o operador "<>", confirmando que o bug descrito
  em 4.5 foi corrigido (o operador e classificado como OP_REL /
  OP_NE, nao como identificador).
