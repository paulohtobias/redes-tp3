#!/bin/bash

modo=$1
mensagem=$2
tamanho=$3
corrompimento=$4
descarte=$5
atraso=$6

# Abre o receptor
./main.out -t $tamanho -C $corrompimento -D $descarte -A $atraso -q &

# Abre o remetente
./main.out -$modo $mensagem -t $tamanho -C $corrompimento -D $descarte -A $atraso -q &

# Espera os processos terminarem
wait
