#!/bin/bash

set -e

g++ -O2 -std=c++17 ga_tsp.cpp -o ga_tsp

INSTANCES=("qa194" "ca4663" "eg7146" "ar9152" "it16862")

for INST in "${INSTANCES[@]}"; do
  if [ "$INST" = "it16862" ]; then
    POP=300
  else
    POP=250
  fi

  if [ "$INST" = "qa194" ]; then
    POP=200
    GEN=2000
    NOIMP=500
  else
    GEN=3000
    NOIMP=700
  fi

  for CROSS in ox pmx; do
    echo "Running $INST with $CROSS"

    ./ga_tsp data/${INST}.tsp \
      --population $POP \
      --generations $GEN \
      --no-improve $NOIMP \
      --crossover-rate 0.9 \
      --mutation-rate 0.15 \
      --elite 3 \
      --tournament 5 \
      --crossover $CROSS \
      --runs 10 \
      --seed 123 \
      --runs-file runs_ga_${INST}_${CROSS}.csv \
      --history history_ga_${INST}_${CROSS}.csv \
      --route best_route_ga_${INST}_${CROSS}.csv \
      --points points_ga_${INST}.csv

    python plot_ga_results.py \
      --history history_ga_${INST}_${CROSS}.csv \
      --runs runs_ga_${INST}_${CROSS}.csv \
      --route best_route_ga_${INST}_${CROSS}.csv \
      --name ${INST}_${CROSS}
  done

  python compare_ga_crossovers.py \
    --ox runs_ga_${INST}_ox.csv \
    --pmx runs_ga_${INST}_pmx.csv \
    --name ${INST}
done
