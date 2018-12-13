#!/bin/bash

modo=$1
mensagem=$2
tamanho=$3
corrompimento=$4
descarte=$5
atraso=$6

# Abre o receptor
#./main.out -t 5753 -C 70 -A 70 -D 70 &
./main.out -t $tamanho -C $corrompimento -D $descarte -A $atraso &

# Abre o remetente
#./main.out -f instancias/rfc-tcp.txt -t 5753 -C 70 -A 70 -D 70
./main.out -$modo $mensagem -t $tamanho -C $corrompimento -D $descarte -A $atraso &

# Espera os processos terminarem
wait
