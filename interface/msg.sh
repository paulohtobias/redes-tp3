#!/bin/bash
../main.out -q -t $1 -s "$2" -C $3 -D $4 -A $5 -R $6 2>&1 > /dev/null & ../main.out -q -t $1 2>&1 > /dev/null
