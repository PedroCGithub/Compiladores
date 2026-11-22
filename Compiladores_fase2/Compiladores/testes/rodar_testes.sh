#!/bin/bash
# Roda todos os testes do compilador (Linux ou WSL).
# Uso, de dentro da pasta Compiladores:
#     bash testes/rodar_testes.sh
#
# 1) compila o compilador com o comando do enunciado;
# 2) testes/erros:    cada programa tem que dar ERRO e NAO gerar MEPA;
# 3) testes/validos:  cada programa tem que ser aceito nos DOIS modos
#    (aula e -completo), e o MEPA gerado e executado no InterprMEPA;
# 4) testes/completo: programas com recursao, so no modo -completo.
# Se existir <nome>.entrada, ela e o que o usuario digitaria no LEIT.
# Se existir <nome>.esperado, a saida dos IMPR e comparada com ela.

INTERP="../InterprMEPA-2.0/InterprMEPA-2.0/executaMepa"
falhas=0

# executa um programa: $1 = arquivo .txt, $2 = "" ou "-completo"
testar_valido() {
    local prog="$1" modo="$2" base="${1%.txt}" entrada="" obtido
    if ! ./compilador "$prog" $modo | grep -q "Programa aceito"; then
        echo "  FALHOU  $(basename "$prog") $modo (nao foi aceito)"
        falhas=$((falhas + 1))
        return
    fi
    if [ ! -x "$INTERP" ]; then
        echo "  ok      $(basename "$prog") $modo (aceito; sem InterprMEPA para executar)"
        return
    fi
    [ -f "$base.entrada" ] && entrada=$(cat "$base.entrada")
    # o "r" no comeco responde o primeiro prompt do interpretador
    # (modo interativo) e manda ele executar ate o fim (ver README 6.6)
    obtido=$(printf "r\n%s\n" "$entrada" | timeout 10 "$INTERP" -i saida_mepa_exec.txt |
             grep -a "saida: [-0-9]" | sed 's/.*saida: //' | tr -s ' \n' ' ' | sed 's/ *$//')
    if [ -f "$base.esperado" ] && [ "$obtido" != "$(cat "$base.esperado")" ]; then
        echo "  FALHOU  $(basename "$prog") $modo: saida [$obtido], esperado [$(cat "$base.esperado")]"
        falhas=$((falhas + 1))
    else
        echo "  ok      $(basename "$prog") $modo: saida [$obtido]"
    fi
}

echo "== Compilando o compilador"
gcc -g -Og -Wall compilador.c -o compilador || { echo "nao compilou"; exit 1; }

if [ ! -x "$INTERP" ]; then
    echo "(InterprMEPA nao encontrado em $INTERP: rode 'make' na pasta dele;"
    echo " por enquanto so confiro se os programas sao aceitos)"
fi

echo
echo "== Programas com erro (tem que aparecer ERRO e nao gerar MEPA)"
for prog in testes/erros/*.txt; do
    mensagem=$(./compilador "$prog" | grep "^ERRO")
    if [ -z "$mensagem" ] || [ -f saida_mepa.txt ]; then
        echo "  FALHOU  $prog"
        falhas=$((falhas + 1))
    else
        echo "  ok      $(basename "$prog"): $mensagem"
    fi
done

echo
echo "== Programas validos, modo aula (so instrucoes da aula)"
for prog in testes/validos/*.txt; do
    testar_valido "$prog" ""
done

echo
echo "== Programas validos, modo -completo (tem que dar a mesma saida)"
for prog in testes/validos/*.txt; do
    testar_valido "$prog" "-completo"
done

echo
echo "== Programas com recursao (so no modo -completo)"
for prog in testes/completo/*.txt; do
    testar_valido "$prog" "-completo"
done

rm -f saida_mepa.txt saida_mepa_exec.txt
echo
if [ $falhas -eq 0 ]; then
    echo "Todos os testes passaram."
else
    echo "$falhas teste(s) falharam."
fi
