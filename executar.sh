#!/bin/bash

# Abre o receptor
./main.out -t 5753 -C 70 -A 70 -D 70 &

# Abre o remetente
#./main.out -f instancias/rfc-tcp.txt -t 5753 -C 70 -A 70 -D 70
./main.out -s a -C 70 -t 1

# Espera os processos terminarem
wait
