#!/bin/bash

set -e

g++ -O3 -march=native -flto -std=c++17 ga_tsp_better.cpp -o ga_tsp_better -pthread

INSTANCES=("qa194" "ca4663" "eg7146" "ar9152" "it16862")
CROSSOVERS=("ox" "pmx")

THREADS=8
RUNS=100
SEED=123

for INST in "${INSTANCES[@]}"; do
  for CROSS in "${CROSSOVERS[@]}"; do
    echo "Running ${INST} with ${CROSS}"

    ./ga_tsp_better data/${INST}.tsp \
      --auto-config 1 \
      --crossover ${CROSS} \
      --runs ${RUNS} \
      --threads ${THREADS} \
      --seed ${SEED} \
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
