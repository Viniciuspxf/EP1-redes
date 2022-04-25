#!/bin/bash
gcc     averages.c   -o averages -lm
cd ..
CASES=(0 100 1000)
rm results/finalResult.txt

for NUMBER_OF_CLIENTS in ${CASES[@]}; do
  ./scripts/averages $NUMBER_OF_CLIENTS < results/${NUMBER_OF_CLIENTS}/outputFiltered.txt
done