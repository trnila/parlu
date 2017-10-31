#!/bin/bash

for i in examples/*.in; do
    if ! ./main txt < $i; then
            echo ERROR
            exit
    fi
done

for i in {1..100}; do
    if ! ./matrix_gen 1000 bin | ./main bin; then
        echo ERROR
        exit
    fi
done