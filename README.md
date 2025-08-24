# Shop scheduling solver

Research code for flow shop, job shop and open shop scheduling problems.

## Compilation

```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel && cmake --install build --config Release --prefix install
```

## Usage

Example:
```
./install/bin/shopschedulingsolver  --verbosity-level 1  --input ./data/vallada2015/Small/VFR10_10_1_Gap.txt --format flow-shop --objective makespan  --algorithm tree-search
```

## Implemented algorithms

$F_m \mid \text{prmu} \mid C_{\max}$
* Tree search `--algorithm tree-search`

$F_m \mid \text{prmu} \mid \sum C_j$
* Tree search `--algorithm tree-search`

$J_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \text{no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
