# TSP laboratory project

This project contains a modular C++ implementation for the Euclidean Travelling Salesman Problem used in the lab assignment. The program reads a TSPLIB-style `EUC_2D` instance, generates 1000 random Hamiltonian cycles, computes the two required aggregate statistics, finds a minimum spanning tree with Prim's algorithm, builds a TSP tour from DFS preorder on that tree, and saves the results to CSV for later plotting.

The README follows the requirements from the uploaded lab sheet: compute the average of minima for **100 groups of 10** random permutations, compute the average of minima for **20 groups of 50** random permutations, generate graphical output later from saved data, and for the extended part compute the MST weight plus a DFS-based TSP cycle from that tree. fileciteturn1file1

## Project layout

```text
.
├── Makefile
├── README.md
├── include/
│   ├── csv_writer.h
│   ├── distance.h
│   ├── mst.h
│   ├── parser.h
│   ├── random_experiment.h
│   └── types.h
└── src/
    ├── csv_writer.cpp
    ├── distance.cpp
    ├── main.cpp
    ├── mst.cpp
    ├── parser.cpp
    └── random_experiment.cpp
```

## What each module does

- `parser.*` reads a TSPLIB-like file and extracts the node coordinates.
- `distance.*` computes integer Euclidean distances using rounding to the nearest integer.
- `random_experiment.*` generates 1000 random permutations and computes the lab statistics.
- `mst.*` builds the minimum spanning tree with Prim's algorithm and the DFS-based TSP tour.
- `csv_writer.*` writes summary values, group minima, and plot-ready tours to CSV.
- `main.cpp` connects everything and handles the CLI.

## Build

Run:

```bash
make
```

This produces the executable:

```bash
./tsp_experiment
```

## Run

Directly:

```bash
./tsp_experiment <input.tsp> [output.csv] [seed]
```

With `make`:

```bash
make run INPUT=../dj38.tsp OUTPUT=../dj38_results.csv SEED=12345
```

If `output.csv` is omitted, the program uses `<input_basename>_results.csv`.

## Input format

The program expects a TSPLIB-like file containing a `NODE_COORD_SECTION`, for example:

```text
NAME: dj38
TYPE: TSP
DIMENSION: 38
EDGE_WEIGHT_TYPE: EUC_2D
NODE_COORD_SECTION
1 11003.611100 42102.500000
2 11108.611100 42373.888900
...
```

The lab sheet points to the Waterloo TSP data page and explicitly asks to pay attention to integer edge weights and rounding rules for the Euclidean instances. fileciteturn1file1

## Computed results

For each input instance the program computes:

1. `1000` random Hamiltonian cycles.
2. Average of the minimum from each of `100` consecutive groups of `10` tours.
3. Average of the minimum from each of `20` consecutive groups of `50` tours.
4. The overall minimum among all 1000 random tours.
5. The minimum spanning tree and its total weight.
6. A TSP cycle produced by listing vertices in the order of first visit during DFS on the MST.
7. The total length of that DFS-based tour. fileciteturn1file1

## CSV output

The CSV file contains three kinds of rows:

- `summary` for aggregate metrics
- `group_min` for minima of each 10-tour and 50-tour block
- `tour` for plot-ready ordered coordinates

Header:

```text
row_type,instance,algorithm,metric,value,group_index,step,node_id,x,y
```

Included tours:

- `random_best` — the best of the 1000 random tours
- `mst_dfs` — the cycle obtained from the MST DFS preorder

The first vertex is repeated at the end of each saved tour so the cycle is already closed for plotting.

## Typical workflow for the lab

1. Build the program.
2. Run it for each required dataset.
3. Save one CSV file per instance.
4. Plot the `tour` rows in Python.
5. Use the `summary` rows in your report.
6. Compare the random best tour with the MST-based approximation.

## Notes for the report

The lab asks for a short report describing the results for each dataset and suggests thinking about easy geometric improvements in Euclidean TSP, including whether edges may cross in an optimal cycle. 

This code does not perform such local improvements automatically; it generates the random-tour baseline and the MST-based 2-approximation style construction requested in the assignment. The lab sheet also notes that the DFS tour built from the MST should be no more than twice the MST cost. 

## Clean build files

```bash
make clean
```
