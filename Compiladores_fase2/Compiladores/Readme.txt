
COMPILADORES - PROJETO Fase 2 - MiniVisualg
README - Fase 1 (Etapas 1, 2 e 3) e Fase 2 (semantica e MEPA)


Integrantes:
  - Henrique Ferreira Marciano - RA 10439797
  - Pedro Casas Pequeno Junior - RA 10437031
  - Pedro Gabriel Guimarães Fernandes - RA 10437465


1. COMO COMPILAR E EXECUTAR

Compilar (comando do enunciado da Fase 2, compila sem warning):
    gcc -g -Og -Wall compilador.c -o compilador

Executar:
    compilador <arquivo_fonte> [arquivo_mepa.txt] [-completo]
    (os dois opcionais estao explicados na secao 6.8)

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
    - (Fase 2) Se houver erro semantico, a mensagem "ERRO SEMANTICO"
      com a linha, onde foi e o motivo, e o programa encerra com
      exit(0). Ver secao 6.4.
    - (Fase 2) Se o programa nao tiver nenhum erro, os arquivos
      saida_mepa.txt (codigo MEPA no estilo da aula, com os nomes das
      variaveis) e saida_mepa_exec.txt (o mesmo codigo no formato do
      InterprMEPA, para executar). Ver secao 6.5; o passo a passo
      para rodar e testar esta na secao 6.6.



2. O QUE FOI IMPLEMENTADO


Etapa#1 - Gramatica livre de contexto: concluida (ver secao 3).
Etapa#2 - Analisador lexico: concluido.
Etapa#3 - Analisador sintatico: concluido, usando o metodo
          descendente recursivo preditivo.
Fase 2  - Analisador semantico: concluido (todas as regras do
          enunciado, secao 6).
Fase 2  - Geracao de codigo MEPA: concluida para todos os comandos,
          procedimentos e funcoes, usando so as instrucoes da aula
          de MEPA (secao 6.5), testada no InterprMEPA 2.0.

Os analisadores lexico e sintatico funcionam em conjunto: o parser
chama a funcao avancar(), que e o "nextToken()" citado no enunciado,
e essa funcao chama obterToken() (a funcao do analisador lexico) para
buscar o proximo token sob demanda. Ou seja, o lexico so roda quando
o sintatico pede o proximo token, exatamente como no diagrama do
enunciado (analisador lexico <-> analisador sintatico).



3. GRAMATICA UTILIZADA (Etapa#1)


Esta e a mesma gramatica entregue na Etapa#1, usada sem alteracoes
como base do parser da Etapa#3. Na Fase 2 so mudou o passo do
"para", que passou a ser uma constante (ver 6.2.8).

3.1 Tokens (expressoes regulares)

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

programa    -> algoritmo cadeia declaracao* inicio comando* fimalgoritmo
declaracao  -> decl_var | decl_proc | decl_func

3.3 Declaracao de variaveis

decl_var   -> var decl_lista+
decl_lista -> id_lista ':' tipo
id_lista   -> id (',' id)*
tipo       -> tipo_base | vetor '[' num_int '..' num_int ']' de tipo_base
tipo_base  -> inteiro | real | caractere | logico

3.4 Procedimentos e funcoes

decl_proc  -> procedimento id ('(' parametros ')')? inicio comando* fimprocedimento
decl_func  -> funcao id '(' parametros? ')' ':' tipo_base inicio comando* fimfuncao
parametros -> parametro (',' parametro)*
parametro  -> id ':' tipo_base

Procedimentos sem parametro nao usam parenteses -- nem na
declaracao, nem na chamada (ex: "linha_decorativa" no Anexo I).

3.5 Comandos

comando  -> atribuicao | leitura | escrita | condicional
          | repeticao_para | repeticao_enquanto | chamada | retorno

atribuicao        -> variavel '<-' expressao
variavel          -> id ('[' expressao ']')?
leitura           -> leia '(' variavel ')'
escrita           -> (escreva | escreval) '(' expressao (',' expressao)* ')'
condicional       -> se '(' expressao ')' entao comando* (senao comando*)? fimse
repeticao_para    -> para id de expressao ate expressao (passo '-'? num_int)? faca comando* fimpara
repeticao_enquanto -> enquanto '(' expressao ')' faca comando* fimenquanto
chamada           -> id ('(' (expressao (',' expressao)*)? ')')?
retorno           -> retorne expressao

3.6 Expressoes

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



4. IMPLEMENTACAO DO PARSER (Etapa#3)


4.1 Por que analisador descendente recursivo preditivo

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

Existe uma variavel global tokenAtual, que e o "1 token de
lookahead" que o parser sempre enxerga. A funcao avancar() faz o
papel do nextToken() citado no enunciado: ela chama obterToken()
(funcao do analisador lexico, sem nenhuma alteracao na sua logica) e
guarda o resultado em tokenAtual, ja imprimindo o token -- por isso
a etapa 2 continua funcionando normalmente, so que agora quem manda
buscar cada token e o proprio parser, token por token, no momento em
que precisa dele.

4.3 Eliminacao de recursao a esquerda nas expressoes (uso de "while")

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

Seguindo o exigido no enunciado, toda vez que um token nao bate com
o que a gramatica esperava, e impressa a mensagem:
    ERRO SINTATICO | linha <numero> | token '<lexema>' | motivo: <descricao>
tanto na tela quanto no arquivo saida.txt, e o programa encerra com
exit(0) (o proprio enunciado pede retorno 0 mesmo em caso de erro,
para nao ser descontado por warning/retorno diferente de 0).


5. TESTES REALIZADOS (Fase 1)


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



6. FASE 2 - ANALISE SEMANTICA E GERACAO DE CODIGO (MEPA)


Nesta fase o compilador passou a fazer duas coisas novas. A primeira
e a analise semantica: alem de saber se o programa esta "bem escrito"
(sintatico), agora ele confere se o programa faz sentido (variavel
declarada, tipos que combinam, chamada com o numero certo de
argumentos...). A segunda e a geracao de codigo: quando o programa
nao tem nenhum erro, o compilador escreve um arquivo com instrucoes
da maquina MEPA, que pode ser executado no interpretador da MEPA.

Uma coisa que a gente so entendeu fazendo: a analise semantica e a
geracao de codigo nao sao "passadas" separadas. Elas acontecem DENTRO
das mesmas funcoes parse*() da Fase 1. Por exemplo, parseFator() ja
reconhecia um numero; agora ela tambem diz que o tipo dele e inteiro
e escreve "CRCT 5". Isso se chama traducao dirigida pela sintaxe.

Compilacao exigida pelo enunciado da Fase 2:
    gcc -g -Og -Wall compilador.c -o compilador


6.1 Decisao de projeto: tabela de simbolos

O enunciado da Fase 2 pede uma minitabela de simbolos em lista
encadeada (struct TNo). Com autorizacao da professora (conversa em
[DATA]), o grupo NAO usou a TNo: a tabela de simbolos ja existente
desde a Fase 1 (a matriz tabelaSimbolos[][], preenchida pelo
analisador lexico) foi mantida como tabela unica e estendida com
vetores paralelos, indexados pela mesma posicao do identificador.

Por que: o analisador lexico ja insere cada identificador na matriz
e devolve a posicao dele em token.attribute.table_index. Usar essa
mesma posicao no parser evita manter duas tabelas com o mesmo nome
e evita mexer na estrutura do token, que o enunciado pede para
manter igual a da Fase 1.

Como a matriz recebe TODO identificador que aparece no texto (inclusive
os nao declarados), "estar na tabela" nao significa "estar
declarado". Por isso existe a flag declarada[] (ver 6.2.1).

Exemplo de como a tabela fica (programa com uma funcao "dobro(n)" e
as variaveis x, y e notas: vetor[1..3] de real):

    i  nome    declarada categoria  tipo     endereco
    0  dobro   1         CAT_FUNC   inteiro  -1   (rotina nao ocupa memoria)
    1  n       0         CAT_PARAM  inteiro  (posicao escondida, ver 6.5.5;
                                                  saiu de escopo no fimfuncao)
    2  x       1         CAT_VAR    inteiro  0
    3  y       1         CAT_VAR    inteiro  1
    4  notas   1         CAT_VETOR  real     2    (ocupa 2, 3 e 4)


6.2 Registro das mudancas

Cada mudanca segue o modelo "O que mudou / Onde / Por que". Os
numeros (6.2.x) sao citados nos comentarios do codigo.

6.2.1 Tabela de simbolos estendida e funcoes do semantico
    O que mudou: criados os enums Categoria (CAT_VAR, CAT_VETOR,
                 CAT_PARAM, CAT_PROC, CAT_FUNC) e TipoBase (T_INT,
                 T_REAL, T_CHAR, T_LOG, T_NENHUM); criados os vetores
                 paralelos a tabelaSimbolos[] (declarada, categoria,
                 tipoBase, endereco, linhaDecl, atribuida, usada,
                 limInf, limSup, qtdParams, tipoParam, rotuloRotina)
                 e as globais proxEndereco e rotinaAtual; criadas as
                 funcoes declarar(), busca_tabela_simbolos(),
                 proximo_rotulo(), erroSemantico() e
                 erroSemanticoNaLinha().
    Onde:        definicoes de tipos e variaveis globais no inicio do
                 compilador.c; as funcoes no bloco "Analisador
                 semantico (Fase 2)", perto do fim do arquivo.
    Por que:     a matriz da Fase 1 so guarda o nome. Os vetores
                 paralelos guardam, na mesma posicao i, tudo o que o
                 semantico precisa saber sobre o nome tabelaSimbolos[i]
                 (ver 6.1). declarar() e o unico ponto que registra
                 uma declaracao e ja barra nomes repetidos.
                 busca_tabela_simbolos() e proximo_rotulo() seguem o
                 que o enunciado recomenda. erroSemantico() segue o
                 mesmo formato de mensagem dos erros lexico e
                 sintatico e encerra com exit(0).

6.2.2 Declaracao de variaveis na tabela de simbolos
    O que mudou: parseListaIds() passou a declarar cada nome assim que
                 o le (com tipo provisorio T_NENHUM) e a devolver os
                 indices em ids[]; parseDeclaracao() completa o tipo,
                 a categoria (CAT_VAR ou CAT_VETOR), os limites do
                 vetor e os enderecos depois de ler o tipo; parseTipo()
                 passou a devolver o TipoBase e os limites do vetor, e
                 foi criada parseTipoBase() para os tipos simples;
                 parseListaDeclaracoes() passou a entrar tambem quando
                 o nome e uma palavra reservada (exceto "inicio").
    Onde:        parseListaDeclaracoes(), parseDeclaracao(),
                 parseListaIds(), parseTipo(), parseTipoBase() (nova);
                 parseFuncao() e parseParametro() passaram a chamar
                 parseTipoBase().
    Por que:     na linguagem os nomes vem antes do tipo
                 ("a, b : inteiro"), entao o tipo so e conhecido no fim
                 da linha. Declarar cada nome no momento em que e lido
                 faz o erro de nome repetido apontar a linha certa.
                 Cada vetor ocupa (limSup - limInf + 1) posicoes, entao
                 os enderecos sao recalculados depois de conhecer o
                 tipo. O laco de parseListaDeclaracoes() precisou
                 aceitar palavras reservadas para que "inteiro: real"
                 gere ERRO SEMANTICO (exigido no enunciado) em vez de
                 ERRO SINTATICO. Funcoes e parametros so aceitam tipo
                 base (gramatica, secao 3.4), por isso usam
                 parseTipoBase(); pelo mesmo motivo "vetor de vetor"
                 passou a ser erro sintatico, como ja dizia a gramatica
                 da secao 3.3.

6.2.3 Uso de identificadores, rotinas e parametros na tabela
    O que mudou: todo lugar onde um nome e USADO (atribuicao, leia,
                 para, expressao, chamada) passou a conferir se ele foi
                 declarado e se e do tipo certo de coisa. Para isso
                 foram criadas usarComoVariavel() e usarComoRotina().
                 Procedimentos e funcoes passaram a ser declarados na
                 tabela (iniciarRotina()) logo depois do nome, e cada
                 parametro e declarado como CAT_PARAM. No fim da rotina,
                 encerrarRotina() tira os parametros de escopo.
                 parseComandoLeia() ganhou a auxiliar
                 parseVariavelLeia().
    Onde:        parseAtribuicaoOuChamada(), parseVariavelLeia(),
                 parseComandoPara(), parseIdentificadorEmExpressao(),
                 parseProcedimento(), parseFuncao(), parseParametro();
                 usarComoVariavel(), usarComoRotina(), iniciarRotina()
                 e encerrarRotina() no bloco do semantico.
    Por que:     sem registrar as rotinas e os parametros, "retorne
                 a + b" ou "somar(10, 5)" dariam "nao declarado", e os
                 exemplos do Anexo I parariam de funcionar. A rotina e
                 declarada ANTES do corpo (na analise, fat chamando fat
                 e aceito; a geracao de codigo e que recusa, ver
                 6.5.5). Os parametros saem de escopo no fim da
                 rotina para que duas funcoes possam ter um parametro
                 "n" cada uma. As duas funcoes auxiliares evitam repetir
                 as mesmas 4 checagens em 5 lugares diferentes.
                 Um detalhe que deu trabalho: em "x <- v" o parser so
                 descobre que o "v" esta errado DEPOIS de ja ter lido o
                 proximo token, que pode estar na linha de baixo. Por
                 isso guardamos a linha do identificador e usamos
                 erroSemanticoNaLinha(), senao a mensagem apontava a
                 linha errada.

6.2.4 Tipos nas expressoes
    O que mudou: parseExpressao(), parseExprE(), parseExprRel(),
                 parseExprArit(), parseTermo() e parseFator() deixaram
                 de ser void e passaram a devolver o TipoBase da
                 expressao. Em cada operador o tipo dos dois lados e
                 conferido (regras na secao 6.3). Atribuicao, se,
                 enquanto, para e indice de vetor conferem o tipo que
                 receberam. Foram criadas ehNumerico(), podeAtribuir(),
                 tipoAritmetico() e exigirTipo().
    Onde:        funcoes de expressao (fim do analisador sintatico) e
                 bloco "Regras de tipos" do semantico.
    Por que:     o enunciado pede que "variaveis utilizadas em uma
                 mesma expressao" tenham tipos compativeis. Como a
                 expressao e uma arvore (a + b * c), o jeito natural e
                 cada funcao devolver o tipo do pedaco que ela
                 reconheceu para a funcao de cima decidir. A gente
                 percebeu que isso e o mesmo caminho da precedencia da
                 Fase 1 (secao 4.3), so que "subindo" com o tipo.

6.2.5 Regras de chamada e de retorno de rotinas
    O que mudou: criada parseArgumentos(), que confere quantidade e
                 tipo de cada argumento contra qtdParams/tipoParam;
                 funcao usada como comando agora e erro; parseComandoRetorne()
                 confere se esta dentro de uma funcao e se o tipo da
                 expressao combina com o tipo de retorno; funcao sem
                 nenhum retorne tambem virou erro.
    Onde:        parseArgumentos() (nova), parseAtribuicaoOuChamada(),
                 parseIdentificadorEmExpressao(), parseComandoRetorne(),
                 parseFuncao().
    Por que:     sao quatro regras do enunciado: tipo e numero dos
                 argumentos; procedimento nao pode estar em expressao;
                 funcao nao pode ser usada como procedimento; retorno
                 compativel com a declaracao. A regra "funcao sem
                 retorne" nao esta no enunciado, mas sem ela a funcao
                 devolveria lixo de memoria na MEPA.

6.2.6 Variavel atribuida e nunca usada
    O que mudou: as marcas atribuida[] (<-, leia e para) e usada[]
                 (quando o valor e lido numa expressao) sao preenchidas
                 durante a analise, e no fim do programa
                 verificarVariaveisNaoUsadas() procura variavel com
                 atribuida = 1 e usada = 0.
    Onde:        verificarVariaveisNaoUsadas(), chamada no fim de
                 parsePrograma().
    Por que:     regra do enunciado ("toda variavel declarada e com
                 valor atribuido a ela deve ser usada"). So da para
                 saber isso no fim, porque a variavel pode ser usada
                 em qualquer linha depois da atribuicao. A mensagem
                 aponta a linha da DECLARACAO (linhaDecl[]), que e onde
                 a pessoa precisa olhar.
    Obs.:        variavel declarada e nunca atribuida nem usada NAO e
                 erro (o enunciado so fala da atribuida).

6.2.7 Geracao de codigo MEPA
    O que mudou: criado o bloco "Geracao de codigo MEPA": o codigo e
                 montado em memoria (struct Codigo, com a versao da
                 aula e a versao exec), as funcoes emitir*() e
                 gravarCodigoMepa(). Cada funcao parse*() passou a
                 emitir as instrucoes do seu comando (padroes na secao
                 6.5).
    Onde:        bloco "Geracao de codigo MEPA" no fim do arquivo;
                 chamadas emitir*() espalhadas pelas funcoes parse*().
    Por que:     o enunciado pede a geracao de codigo para todos os
                 comandos e rotinas, num arquivo TXT executavel na
                 maquina MEPA. Os motivos dos buffers e dos dois
                 arquivos estao em 6.5.

6.2.8 Passo do "para" como constante
    O que mudou: na gramatica da Fase 1 o passo era uma expressao
                 qualquer; agora ele precisa ser uma constante inteira
                 (com ou sem sinal de menos), diferente de zero.
    Onde:        parsePasso() (nova), chamada por parseComandoPara().
    Por que:     o teste de parada do laco depende do sinal do passo:
                 com passo positivo o laco continua enquanto
                 i <= fim (CMEG); com passo negativo, enquanto
                 i >= fim (CMAG). Com uma expressao qualquer o
                 compilador nao sabe o sinal na hora de gerar o codigo.
                 Todos os exemplos do Anexo I usam passo constante
                 (ex.: "passo -2" na ContagemRegressiva).

6.2.9 Geracao restrita as instrucoes da aula
    O que mudou: a primeira versao da geracao usava instrucoes do
                 livro que nao estao na aula (CHPR/ENPR/RTPR para
                 rotinas, CREN/CRVI/ARMI para vetores, DMEM) e
                 enderecos "nivel, deslocamento". Ela foi refeita para
                 usar so as instrucoes da aula: rotulos "L1", variaveis
                 pelo nome, DIVF para "/", rotinas copiadas no lugar da
                 chamada (expandirRotina) e vetores acessados posicao
                 por posicao (emitirLeituraVetor/emitirEscritaVetor).
    Onde:        bloco "Geracao de codigo MEPA"; parseProcedimento(),
                 parseFuncao(), parseParametro(), iniciarRotina(),
                 encerrarRotina(), parseIndiceVetor() (antes
                 parseEnderecoVetor).
    Por que:     o codigo gerado tem que seguir o que foi ensinado em
                 aula. Detalhes e limitacoes em 6.5.5 a 6.5.7; a
                 principal e que recursao deixou de ser possivel.

6.2.10 Modo completo e nome do arquivo de saida
    O que mudou: o compilador ganhou a opcao -completo (rotinas com
                 CHPR/ENPR/RTPR e vetores com CREN/CRVI/ARMI, que
                 permitem recursao) e aceita um 2o argumento com o nome
                 do arquivo MEPA. Sem opcao nenhuma, continua tudo no
                 estilo da aula.
    Onde:        lerArgumentos() (nova), parsePrograma(),
                 parseProcedimento(), parseFuncao(), parseParametro(),
                 parseIndiceVetor(), iniciarRotina(), e as funcoes
                 emitirVariavel(), emitirLeituraVetor(),
                 emitirEscritaVetor(), emitirRetornoFuncao() e
                 expandirRotina(), que olham a variavel modoCompleto.
    Por que:     juntar a versao dos colegas (recursao de verdade) com
                 a nossa (so instrucoes da aula) sem perder nenhuma das
                 duas. Detalhes na secao 6.8.

6.2.11 Avisos e temporarias so quando precisa
    O que mudou: o compilador passou a mostrar linhas "AVISO" (sem parar
                 a compilacao) quando o programa usa algo que a MEPA nao
                 representa exatamente; e as duas posicoes temporarias
                 (_tmp0 e _tmp1) so sao reservadas no AMEM se algum
                 vetor ou MOD realmente usar.
    Onde:        aviso() (nova), parseComandoEscreva(), parseTermo(),
                 parseFator(); emitirTemporaria() marca usouTemporaria,
                 e gravarBuffer()/gravarCodigoMepa() descontam as
                 temporarias quando nao foram usadas.
    Por que:     ideias da conversa de outro integrante (6.8). O aviso
                 deixa claro na hora da compilacao o que vai mudar na
                 execucao (ver 6.5.7), e o AMEM fica so com o necessario.

6.2.12 Organizacao do codigo
    O que mudou: cabecalho atualizado para a Fase 2 (comando de
                 compilacao do enunciado e arquivos de saida);
                 prototipos separados por modulo (lexico, semantico,
                 geracao de codigo, sintatico); comentarios em cada
                 comando mostrando o codigo MEPA que ele gera; linhas
                 longas quebradas.
    Por que:     clareza e organizacao contam na nota, e esses
                 comentarios ajudaram a gente mesmo a conferir o
                 codigo gerado.


6.3 Regras semanticas adotadas

Tipos e compatibilidade:

    Operacao               Operandos aceitos           Resultado
    ---------------------  --------------------------  ---------
    +  -  *                inteiro/real (misturados)   real se algum for real,
                                                       senao inteiro
    /                      inteiro/real                real (como no VisuAlg)
    \  MOD                 inteiro                     inteiro
    - (unario)             inteiro/real                o mesmo tipo
    =  <>                  numero com numero, caract.  logico
                           com caract., logico com
                           logico
    <  <=  >  >=           numero com numero, caract.  logico
                           com caractere
    E  OU                  logico                      logico

    Atribuicao, argumento e retorne: o tipo tem que ser o mesmo, com
    uma excecao: inteiro pode ir para real ("soma <- 0" com soma real,
    como no MediaNotas do Anexo I). Real para inteiro e erro, porque
    perderia a parte fracionaria.

    se / enquanto: a condicao tem que ser logica.
    para: variavel de controle, valor inicial e valor final inteiros;
          passo constante inteira diferente de zero (6.2.8).
    indice de vetor: inteiro.

Escopo:
    - Os procedimentos e funcoes vem ANTES da secao var na gramatica.
      Por isso, dentro de uma rotina, so existem os parametros dela
      (as variaveis globais ainda nem foram declaradas). Usar uma
      global dentro de uma rotina da "identificador nao declarado".
    - Parametros saem de escopo no fim da rotina: duas rotinas podem
      ter parametros com o mesmo nome, e uma global pode ter o nome
      de um parametro.
    - Variaveis, rotinas e parametros dividem o mesmo espaco de nomes:
      uma funcao "f" e uma variavel "f" e erro.
    - Uma rotina pode chamar qualquer rotina declarada antes dela.
      Chamar a si mesma (recursao) passa na analise semantica, mas a
      geracao de codigo da erro (ver 6.5.5).


6.4 Mensagens de erro semantico

Formato (igual ao dos erros lexico e sintatico):
    ERRO SEMANTICO | linha <n> | sequencia '<onde>' | <motivo>

Motivos que o compilador pode mostrar:
    - identificador nao declarado / rotina nao declarada
    - identificador declarado mais de uma vez
    - palavra reservada nao pode ser usada como nome de variavel
      (ou de parametro)
    - vetor com limite inferior maior que o limite superior
    - vetor usado sem indice / indice usado em variavel que nao e vetor
    - rotina usada como se fosse variavel / variavel usada como se
      fosse rotina
    - operacao aritmetica com operando que nao e numero
    - comparacao entre tipos incompativeis
    - "<contexto>: esperado <tipo>, encontrado <tipo>" (condicao do
      se/enquanto, operandos de E/OU/MOD/\, para, indice de vetor)
    - tipo da expressao incompativel com o tipo da variavel/do vetor
    - chamada com argumentos demais / de menos
    - tipo de argumento incompativel com o parametro
    - procedimento nao pode ser usado em expressao
    - funcao nao pode ser usada como procedimento
    - retorne so pode ser usado dentro de funcao
    - tipo do retorno incompativel com o declarado na funcao
    - funcao sem nenhum comando retorne
    - variavel recebe valor mas nunca e usada
    - o passo do para deve ser uma constante inteira / nao pode ser zero

Alem dos erros semanticos, existe um erro de geracao de codigo:
    ERRO DE GERACAO DE CODIGO | linha <n> | sequencia '<rotina>' |
    recursao nao e suportada: a MEPA da aula nao tem instrucao de
    chamada de rotina

Em todos os casos o compilador para no primeiro erro, com exit(0), e
os arquivos MEPA nao sao gerados.


6.5 Geracao de codigo MEPA

6.5.1 So o que foi visto na aula

O codigo gerado usa apenas as instrucoes da aula de MEPA (Profa.
Daniela Cunha):

    CRCT k   CRVL n   ARMZ n                     carregar / guardar
    SOMA  SUBT  MULT  DIVI  DIVF  INVR           aritmetica
    CONJ  DISJ                                   E / OU
    CMME  CMMA  CMIG  CMDG  CMEG  CMAG           comparacoes
    DSVS p  DSVF p  NADA                         desvios e rotulos
    INPP  AMEM m  LEIT  IMPR  PARA               programa, memoria, E/S

e os mesmos padroes de traducao da aula para if/else, while e for,
com rotulos no formato "L1: NADA". As variaveis sao referenciadas
pelos nomes, como o enunciado pede ("CRVL a", "ARMZ s", igual aos
exemplos da aula).

A MEPA e uma maquina de pilha: as contas sao feitas empilhando os
operandos e depois aplicando o operador. Por exemplo, "c <- a + b * 2"
vira:

    CRVL a        empilha a
    CRVL b        empilha b
    CRCT 2        empilha 2
    MULT          troca b e 2 por b*2
    SOMA          troca a e b*2 por a+b*2
    ARMZ c        tira o resultado da pilha e guarda em c

Isso e exatamente o "1o passo: converter para posfixa" da aula. O que
a gente achou legal: o compilador nao precisa converter nada
separado, porque a ordem em que as funcoes de expressao terminam de
reconhecer cada pedaco ja e a ordem posfixa (o operador so e emitido
depois dos dois operandos).

Algumas coisas da linguagem nao tem instrucao propria na aula. Elas
foram montadas so com as instrucoes acima (ver 6.5.5 e 6.5.6):
procedimentos/funcoes, acesso a vetor com indice variavel e MOD.

6.5.2 Os dois arquivos gerados

    saida_mepa.txt       o codigo no estilo da aula: "CRVL soma"
    saida_mepa_exec.txt  o MESMO codigo, so que no formato que o
                         InterprMEPA 2.0 (pasta InterprMEPA-2.0) le

Por que o segundo: o InterprMEPA nao aceita nome de variavel (o
leMepa.l dele so reconhece instrucoes, rotulos e numeros) e exige o
endereco no formato "nivel, deslocamento". Ele tambem nao tem a
instrucao DIVF, porque so trabalha com numeros inteiros. Entao a
versao exec troca:

    CRVL soma  ->  CRVL 0, 2     (2 = endereco de soma, ordem de declaracao)
    DIVF       ->  DIVI

e nada mais: as instrucoes, a ordem e os rotulos sao os mesmos. Os
arquivos do InterprMEPA nao foram alterados.

6.5.3 Por que o codigo vai para um buffer antes do arquivo

O programa tem que comecar com "AMEM m" reservando a memoria das
variaveis. So que na nossa gramatica a secao var vem DEPOIS dos
procedimentos e funcoes, entao enquanto o compilador analisa as
rotinas ele ainda nao sabe quanto vale m. A solucao foi guardar o
codigo num texto em memoria (struct Buffer, que cresce com realloc) e
so escrever os arquivos no fim, ja com o cabecalho certo. Isso tambem
garante que, se tiver erro, nenhum arquivo MEPA e criado (os de uma
compilacao anterior sao apagados no inicio, em iniciarGeracao()).

Estrutura do arquivo gerado (igual ao exemplo de programa da aula):

         INPP
         AMEM m          (m = variaveis do programa + posicoes escondidas;
                          as 2 temporarias so entram se forem usadas)
         ... comandos do programa principal ...
         PARA

6.5.4 Padroes de codigo por comando

    x <- e          <e> ; ARMZ x
    leia(x)         LEIT ; ARMZ x
    escreva(e)      <e> ; IMPR             (um IMPR por expressao)
    se/senao        <cond> ; DSVF L1 ; <entao> ; DSVS L2 ;
                    L1: NADA ; <senao> ; L2: NADA
    enquanto        L1: NADA ; <cond> ; DSVF L2 ; <corpo> ;
                    DSVS L1 ; L2: NADA
    para i de a     <a> ; ARMZ i ; L1: NADA ; CRVL i ; <b> ;
      ate b         CMEG (CMAG se passo < 0) ; DSVF L2 ; <corpo> ;
      passo k       CRVL i ; CRCT k ; SOMA ; ARMZ i ; DSVS L1 ;
                    L2: NADA
    numero / logico CRCT valor (verdadeiro = 1, falso = 0)
    variavel        CRVL x
    operadores      + SOMA, - SUBT, * MULT, / DIVF, \ DIVI, E CONJ,
                    OU DISJ, menos unario INVR, < CMME, > CMMA,
                    = CMIG, <> CMDG, <= CMEG, >= CMAG

Rotulos: cada comando pega os seus rotulos com proximo_rotulo() ANTES
de analisar os comandos de dentro (variaveis locais rotuloSenao,
rotuloFim...). O enunciado avisa disso: se o rotulo fosse pedido
depois da chamada recursiva, um "se" dentro de outro "se" pegaria os
mesmos numeros e os desvios iriam para o lugar errado.

6.5.5 Procedimentos e funcoes (expansao no lugar da chamada)

A aula nao tem nenhuma instrucao para chamar uma rotina e voltar
(nao tem CHPR, ENPR nem RTPR). A saida que o grupo encontrou foi
copiar o corpo da rotina em cada lugar onde ela e chamada:

  - Quando o compilador analisa a rotina, o codigo do corpo nao vai
    para o programa principal: vai para um "molde" (codigoRotina[]).
  - Cada parametro e o resultado da funcao ganham uma posicao de
    memoria so deles, depois das variaveis do programa. Na versao da
    aula eles aparecem como "rotina_parametro" e com o nome da funcao:

        funcao somar(a: inteiro, b: inteiro): inteiro
            retorne a + b
        ...
        resultado <- somar(10, 5)

    vira

        CRCT 10                (argumentos empilhados na ordem)
        CRCT 5
        ARMZ somar_b           (tira da pilha do ultimo para o primeiro)
        ARMZ somar_a
        CRVL somar_a           (corpo copiado do molde)
        CRVL somar_b
        SOMA
        ARMZ somar             (retorne: guarda o resultado...)
        DSVS L2                (...e pula para o fim do corpo)
    L2: NADA
        CRVL somar             (o resultado vai para a pilha)
        ARMZ resultado

  - Os rotulos dentro do molde ficam marcados como "L#n". Cada copia
    troca essas marcas por rotulos novos; senao duas chamadas da
    mesma funcao teriam o mesmo "L3" e o desvio iria para a copia
    errada.
  - Uma rotina pode chamar outra declarada antes dela (a copia da
    outra entra dentro do molde dela).
  - Recursao (fat chamando fat) NAO e possivel desse jeito: o molde
    ainda nao esta pronto quando a rotina chama a si mesma, e copiar
    o corpo dentro dele mesmo nunca terminaria. Nesse caso o
    compilador mostra "ERRO DE GERACAO DE CODIGO". Nenhum exemplo do
    Anexo I usa recursao.

6.5.6 Vetores e MOD (as duas posicoes temporarias)

A aula so tem CRVL n e ARMZ n com endereco fixo; nao tem instrucao
para ler uma posicao cujo endereco e calculado durante a execucao.
Com indice constante isso nao seria problema, mas em "notas[i]" o i
so e conhecido na hora de rodar. A solucao foi testar o indice
posicao por posicao, usando so CMIG e desvios:

    leitura de notas[i], com notas: vetor[1..4]
        <i>
        ARMZ _tmp0             guarda o indice
        CRVL _tmp0
        CRCT 1
        CMIG                   i = 1 ?
        DSVF L2
        CRVL notas[1]          se sim, empilha notas[1]...
        DSVS L1                ...e sai
    L2: NADA
        ... (o mesmo para 2, 3 e 4) ...
        CRCT 0                 indice fora dos limites
    L1: NADA

A gravacao (notas[i] <- e, leia(notas[i])) e igual, so que guarda
tambem o valor em _tmp1 e no lugar do "CRVL notas[k]" faz
"CRVL _tmp1 ; ARMZ notas[k]". O codigo fica maior (cresce com o
tamanho do vetor), mas usa so instrucoes da aula. Na versao exec,
"notas[3]" vira o endereco do vetor + (3 - limite inferior).

A MEPA tambem nao tem MOD. Usamos a MOD b = a - (a \ b) * b, e as
mesmas temporarias guardam a e b para poder empilha-los de novo.

As temporarias so sao usadas em sequencias curtas, depois que tudo
o que estava dentro (indice, valor, operandos) ja foi calculado,
entao uma expressao dentro da outra nao estraga o valor guardado.

6.5.7 Limitacoes

    - A MEPA da aula so carrega constante inteira (CRCT k). Um literal
      real como 1.60 vira CRCT 1 (a parte fracionaria se perde). A
      checagem de tipos continua tratando real como real.
    - IMPR so imprime numero, entao uma cadeia sozinha dentro do
      escreva ("Ola") e aceita mas nao gera codigo. escreval nao gera
      a quebra de linha.
    - Um valor caractere vira o codigo ASCII do primeiro caractere
      ("Ana" -> 65); leia de caractere le um numero.
    - Recursao nao e suportada (6.5.5).
    - Indice fora dos limites do vetor: a leitura da 0 e a gravacao
      nao faz nada (nao ha mensagem de erro na execucao).

Nos casos de real, "/", cadeia no escreva e cadeia em expressao, o
compilador avisa durante a compilacao, sem parar:
    AVISO | linha 9 | numero real truncado para inteiro (a MEPA so trabalha com inteiros)
    AVISO | linha 7 | cadeia no escreva nao e impressa (IMPR da MEPA so imprime numero)


6.6 Como rodar e testar

Passo 1 - Compilar o compilador (Windows ou Linux/WSL), na pasta
Compiladores:
    gcc -g -Og -Wall compilador.c -o compilador

Passo 2 - Compilar um programa MiniVisualg:
    ./compilador testes/validos/ok_soma.txt
    (no Windows: compilador testes\validos\ok_soma.txt)
  Se der certo, aparecem saida_mepa.txt (estilo da aula, para ler e
  entregar) e saida_mepa_exec.txt (para executar).
  Para recursao: ./compilador testes/completo/fat_recursivo.txt -completo
  Para escolher o nome: ./compilador programa.txt resultado.txt
  (gera resultado.txt e resultado_exec.txt).

Passo 3 - Montar o InterprMEPA (uma vez so). Ele usa flex e bison,
entao precisa de Linux ou WSL:
    sudo apt-get install flex bison libfl-dev libbison-dev
    cd ../InterprMEPA-2.0/InterprMEPA-2.0
    make
    cd ../../Compiladores

Passo 4 - Executar o codigo gerado:
    ../InterprMEPA-2.0/InterprMEPA-2.0/executaMepa -i saida_mepa_exec.txt
  Ele abre no modo passo a passo e mostra "=>". Digite r e Enter
  para ele rodar ate o fim. Cada LEIT pede um numero ("(entrada):")
  e cada IMPR aparece como "saida: <valor>".

  Cuidado: a opcao -r (rodar direto, sem passo a passo) pode parar
  logo na primeira instrucao sem mostrar nada. Descobrimos que no
  executaMepa.c a variavel "cmd" nao e inicializada no modo -r, entao
  o resultado depende do lixo de memoria. Por isso usamos sempre -i e
  o comando r. (Nao alteramos o interpretador.)

Passo 5 - Rodar todos os testes de uma vez (Linux/WSL), na pasta
Compiladores:
    bash testes/rodar_testes.sh
  O script compila o compilador, confere que cada programa de
  testes/erros da ERRO e nao gera MEPA, e executa cada programa de
  testes/validos no InterprMEPA nos dois modos (aula e -completo),
  usando <nome>.entrada como o que seria digitado e comparando a
  saida com <nome>.esperado. Depois roda testes/completo (recursao)
  no modo -completo. No fim mostra "Todos os testes passaram." ou
  quantos falharam (hoje sao 79 verificacoes).


6.7 Testes da Fase 2

Todos rodados com o compilador compilado com "gcc -g -Og -Wall" (sem
nenhum warning); todos terminam com retorno 0.

testes/validos (todos aceitos e executados no InterprMEPA, nos dois
modos, com a mesma saida; a saida esperada esta no arquivo .esperado):
    - linguagem_teste.txt: o teste que ja existia     -> 1 2 3 4 5
    - anexo_*.txt: todos os exemplos do Anexo I do enunciado da Fase 1
      (o de procedimentos foi juntado num programa so, porque no PDF
      ele esta com dois "algoritmo"). Exemplos de saida:
         ContagemRegressiva -> 10 8 6 4 2 0
         CalculadoraBasica  -> 13 30 3 3
         FuncaoEhPar        -> 14 1
         RotinasComRetorno  -> 15
         MediaNotas (7 8 9 10) -> 1 2 3 4 8
    - ok_soma.txt      entrada 3 e 4      -> 11
    - ok_func.txt      somar, eh_par, fat (com laco), procedimentos,
                       MOD e \             -> 15 1 0 120 2 3
    - ok_laco.txt      vetor, para, passo -1, enquanto
                       entrada 7 8 9 10   -> 8 10 9 8 7 3 70
    - ok_aninhado.txt  funcao chamando funcao, procedimento chamando
                       funcao, dobro(dobro(5)), v[v[1]], vetor[0..2],
                       MOD com negativo    -> 26 -7 22 8 4 -1

testes/erros (um arquivo para cada erro; nenhum gera MEPA):
    01 identificador nao declarado      14 retorne fora de funcao
    02 palavra reservada como nome      15 funcao sem retorne
    03 nome declarado duas vezes        16 vetor usado sem indice
    04 atribuida e nunca usada          17 indice real
    05 inteiro + caractere              18 para com variavel real
    06 real atribuido a inteiro         19 passo zero
    07 se com condicao inteira          20 E com operando inteiro
    08 argumentos demais                21 MOD com real
    09 argumentos de menos              22 caractere < numero
    10 argumento de tipo errado         23 lida com leia e nunca usada
    11 procedimento em expressao        24 erro lexico (continua igual)
    12 funcao usada como procedimento   25 erro sintatico (continua igual)
    13 retorno de tipo errado           26 recursao (erro de geracao
                                           no modo aula)

testes/completo (recursao, so com -completo):
    - fat_recursivo.txt   fat(5) recursiva e as rotinas do ok_func
                          -> 15 1 0 120 2 3
    - fib_recursivo.txt   fib(10) recursiva, rotinas aninhadas, vetor
                          -> 26 -7 55 1 -1
    - recursao_vetor.txt  fat e fib recursivas, procedimento chamando
                          funcao recursiva, vetor preenchido com fat
                          -> 120 55 106 2 4 24


6.8 Modo completo (-completo): juntando com o trabalho dos colegas

Outra parte do grupo fez uma versao da geracao de codigo seguindo o
livro do Kowaltowski e o exemplo MEPA7.2 que veio com o interpretador,
com chamadas de rotina de verdade (e por isso com recursao). Em vez de
escolher uma das duas, juntamos: o compilador tem os dois modos.

    ./compilador programa.txt                  modo aula (padrao)
    ./compilador programa.txt -completo        modo completo
    ./compilador programa.txt minha_saida.txt  escolhe o nome do arquivo
                                               (gera tambem minha_saida_exec.txt)

O modo aula e o que entregamos como principal, porque usa so o que foi
visto em aula (6.5). O modo completo usa 7 instrucoes a mais, que nao
estao nos slides mas existem na MEPA e no InterprMEPA:

    CHPR p, k   chama a rotina que comeca no rotulo p
    ENPR k      entra na rotina (salva a base D[k] antiga)
    RTPR k, n   sai da rotina e tira os n argumentos da pilha
    DMEM m      libera a memoria no fim do programa
    CREN n      carrega o ENDERECO de n (e nao o valor)
    CRVI n      carrega o valor que esta no endereco guardado em n
    ARMI n      guarda no endereco guardado em n

O que muda no codigo gerado:

  - Rotinas: o corpo e gerado uma vez so, no comeco do programa, e as
    chamadas pulam para ele. Isso permite recursao (fat(n - 1)).

        funcao somar(a, b)            AMEM 3
            retorne a + b             DSVS L1        pula as rotinas
        resultado <- somar(10, 5)  L2: ENPR 1         comeco de somar
                                      CRVL somar_a   (exec: CRVL 1, -5)
                                      CRVL somar_b   (exec: CRVL 1, -4)
                                      SOMA
                                      ARMZ somar     (exec: ARMZ 1, -6)
                                      DSVS L3
                                   L3: NADA
                                      RTPR 1, 2      volta e tira 2 argumentos
                                   L1: NADA           comeco do programa
                                      AMEM 1         espaco do resultado
                                      CRCT 10
                                      CRCT 5
                                      CHPR L2, 0     chama somar
                                      ARMZ resultado

    O deslocamento negativo dos parametros demorou para fazer sentido:
    quando a rotina comeca, os argumentos ja estao na pilha ANTES do
    registro de ativacao (endereco de retorno, nivel e o D[1] antigo,
    as 3 posicoes do "+3"). Entao o parametro i de N fica em
    -(N + 3) + i, e o resultado da funcao (o AMEM 1 que quem chamou
    fez) fica uma posicao abaixo: -(N + 4).

  - Vetores: em vez de testar posicao por posicao, calcula o endereco
    e acessa direto:
        CREN v ; <i> ; CRCT limInf ; SUBT ; SOMA ; ARMZ _tmp0 ; CRVI _tmp0

  - O resto (expressoes, se, enquanto, para, leia, escreva) e igual
    nos dois modos.

Os dois modos dao exatamente a mesma saida em todos os programas de
testes/validos (o script rodar_testes.sh confere isso). Os programas
de testes/completo usam recursao e so funcionam no modo completo.

Juntamos tambem outras ideias das conversas dos colegas (Henrique e
Pedro) com a IA, que chegaram em caminhos parecidos com o nosso:
    - o 2o argumento com o nome do arquivo MEPA;
    - o teste linguagem_teste.txt (saida 1 2 3 4 5);
    - as linhas AVISO e as temporarias so quando precisa (6.2.11);
    - nao deixar arquivo MEPA quando ha erro (aqui ele e apagado no
      inicio e so e escrito no fim, se nao houve erro);
    - a limpeza da entrega (sem teste.c, executaveis e saida.txt
      antigos).
As duas conversas leram "variaveis referenciadas pelos seus nomes"
como "procurar o nome na tabela e escrever o endereco" (CRVL 0, 2).
Por isso o compilador gera as duas formas: com o nome
(saida_mepa.txt) e com o endereco (saida_mepa_exec.txt).

Links que os colegas usaram como referencia de MEPA:
    https://www.inf.ufpr.br/bmuller/assets/ci1211/Recursos/InterprMEPA-2.0.tar.bz2
    https://www.brunoribas.com.br/compiladores/2024-1/ribas-mepa.gz
    https://www.ic.unicamp.br/~tomasz/ilp/   (livro do Kowaltowski)


6.9 Uso de Inteligencia Artificial

Conforme exigido no enunciado, o grupo declara que usou o assistente
Claude (Anthropic) na Fase 2:
    - Passos 1 a 3 (tabela de simbolos estendida, declaracoes e uso
      de identificadores): a IA explicou cada passo, indicou onde
      mexer no codigo e forneceu trechos de codigo, que o grupo
      aplicou e testou; a IA tambem revisou o que foi aplicado.
    - Passos 4 em diante (tipos nas expressoes, regras de rotinas,
      variavel nao usada, geracao de codigo MEPA, organizacao final):
      o codigo foi gerado pela IA e revisado pelo grupo. A adaptacao
      da geracao para usar so as instrucoes da aula (6.2.9) tambem
      foi feita pela IA, a pedido do grupo.
    - O modo completo e os avisos (6.8 e 6.2.11) juntam ideias das
      conversas de outros integrantes com a IA (rotinas com
      CHPR/ENPR/RTPR, nome do arquivo de saida, avisos, temporarias
      sob demanda); o codigo dessas partes foi escrito pela IA.
    - Este README (secao 6) foi escrito com ajuda da IA, a partir das
      decisoes tomadas junto com o grupo.
    - Os testes da pasta testes/ e o script rodar_testes.sh foram
      montados com ajuda da IA e rodados no compilador e no
      InterprMEPA.
