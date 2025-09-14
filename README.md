# ShopSchedulingSolver

Research code for flow shop, job shop and open shop scheduling problems.

![scheduleexample](img/schedule.png?raw=true "Schedule example")

## Usage

Compile the code:
```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel && cmake --install build --config Release --prefix install
```

Setup Python environment to use the Python scripts:
```shell
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Download data files:
```shell
python scripts/download_data.py
```

Run:
```
./install/bin/shopschedulingsolver  --verbosity-level 1  --input ./data/vallada2015/Small/VFR10_10_1_Gap.txt --format flow-shop --objective makespan  --algorithm tree-search  --certificate certificate.json
```

Visualize solution:
```
python scripts/visualize.py certificate.json
```

## Implemented algorithms

### Flow shop

$F_m \mid \text{prmu} \mid C_{\max}$
* Tree search `--algorithm tree-search`

$F_m \mid \text{prmu} \mid \sum C_j$
* Tree search `--algorithm tree-search`

### Job shop

$J_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \text{no-wait} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \text{no-idle} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \text{no-wait} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$J_m \mid \text{no-idle} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

### Open shop

$O_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-wait} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-idle} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-wait} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`

$O_m \mid \text{no-idle} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
