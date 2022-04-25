#!/bin/bash

CASES=(0 100 1000)
cd ..
rm -r results

mkdir results

for NUMBER_OF_CLIENTS in ${CASES[@]}; do
  mkdir results/${NUMBER_OF_CLIENTS}
  for _ in $(seq 1); do
    docker-compose up -d

    for COUNTER in $(seq $(expr $NUMBER_OF_CLIENTS / 2)); do
      mosquitto_sub -t "luminosity" -t "temperature" &
      mosquitto_sub -t "sound" &
    done

    ./scripts/collectStats.sh ${NUMBER_OF_CLIENTS} &
    for COUNTER in $(seq $(expr $NUMBER_OF_CLIENTS / 3)); do
      mosquitto_pub -t "luminosity" -m "${COUNTER}Lumen"
      mosquitto_pub -t "temperature" -m "${COUNTER}Celsius"
      mosquitto_pub -t "sound" -m "${COUNTER}dB"
    done
    kill -9 $(pgrep collectStats) collectStats 2> /dev/null
    kill -9 $(pgrep mosquitto_sub) 2> /dev/null
    docker-compose down
  done
done