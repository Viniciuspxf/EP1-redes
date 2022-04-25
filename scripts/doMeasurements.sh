#!/bin/bash
make filter

CASES=(0 100 1000)
cd ..

rm -r results

mkdir results

for NUMBER_OF_CLIENTS in ${CASES[@]}; do
  mkdir results/${NUMBER_OF_CLIENTS}
  for _ in $(seq 30); do
    docker-compose up -d

    ./scripts/collectStats.sh ${NUMBER_OF_CLIENTS} &
    for COUNTER in $(seq $(expr $NUMBER_OF_CLIENTS / 2)); do
      mosquitto_sub -p 8000 -t "luminosity" -t "temperature" &
      mosquitto_sub -p 8000 -t "sound" &
    done

    for COUNTER in $(seq $(expr $NUMBER_OF_CLIENTS / 3)); do
      mosquitto_pub -p 8000 -t "luminosity" -m "${COUNTER}Lumen"
      mosquitto_pub -p 8000 -t "temperature" -m "${COUNTER}Celsius"
      mosquitto_pub -p 8000 -t "sound" -m "${COUNTER}dB"
    done
    sleep 3
    kill -9 $(pgrep collect) 2> /dev/null
    sleep 3
    ./scripts/filter < results/${NUMBER_OF_CLIENTS}/output.txt >> results/${NUMBER_OF_CLIENTS}/outputFiltered.txt
    kill -9 $(pgrep mosquitto_sub) 2> /dev/null
    docker-compose down
  done
done