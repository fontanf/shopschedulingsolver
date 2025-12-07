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

### Permutation flow shop

#### Objective makespan

$F_m \mid \text{prmu} \mid C_{\max}$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`
* Tree search `--algorithm tree-search`

$F_m \mid \text{prmu}, \text{mixed no-idle} \mid C_{\max}$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$F_m \mid \text{prmu}, \text{blocking} \mid C_{\max}$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total flow time

$F_m \mid \text{prmu} \mid \sum C_j$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`
* Tree search `--algorithm tree-search`

$F_m \mid \text{prmu}, \text{mixed no-idle} \mid \sum C_j$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$F_m \mid \text{prmu}, \text{blocking} \mid \sum C_j$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total tardiness

$F_m \mid \text{prmu} \mid \sum T_j$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$F_m \mid \text{prmu}, \text{mixed no-idle} \mid \sum T_j$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$F_m \mid \text{prmu}, \text{blocking} \mid \sum T_j$
* Positional MILP `--algorithm milp-positional`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

### Job shop

#### Objective makespan

$J_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{mixed no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{blocking} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total flow time

$J_m \mid \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{no-wait} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{mixed no-idle} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{blocking} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total tardiness

$J_m \mid \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{no-wait} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{mixed no-idle} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$J_m \mid \text{blocking} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

### Flexible job shop

#### Objective makespan

$FJ_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{mixed no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{blocking} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total flow time

$FJ_m \mid \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{no-wait} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{mixed no-idle} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{blocking} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total tardiness

$FJ_m \mid \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{no-wait} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{mixed no-idle} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FJ_m \mid \text{blocking} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

### Open shop

#### Objective makespan

$O_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{mixed no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{blocking} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total flow time

$O_m \mid \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{no-wait} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{mixed no-idle} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{blocking} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total tardiness

$O_m \mid \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{no-wait} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{mixed no-idle} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$O_m \mid \text{blocking} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

### Flexible open shop

#### Objective makespan

$FO_m \mid \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{no-wait} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{mixed no-idle} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{blocking} \mid C_{\max}$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total flow time

$FO_m \mid \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{no-wait} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{mixed no-idle} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{blocking} \mid \sum w_j C_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

#### Objective total tardiness

$FO_m \mid \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{no-wait} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{mixed no-idle} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`

$FO_m \mid \text{blocking} \mid \sum w_j T_j$
* Disjunctive MILP `--algorithm milp-disjunctive`
* Constraint programming OptalCP `--algorithm constraint-programming-optalcp`


Generate test instances list for each algorithm:
```shell
python scripts/solve_test_data.py  --algorithm milp-positional  --output test/algorithms/milp_positional_test.txt  --instances \
        data/test_pfss_makespan.txt \
        data/test_pfss_makespan_no_idle.txt \
        data/test_pfss_makespan_blocking.txt \
        data/test_pfss_tft.txt \
        data/test_pfss_tft_no_idle.txt \
        data/test_pfss_tft_blocking.txt \
        data/test_pfss_tt.txt \
        data/test_pfss_tt_no_idle.txt \
        data/test_pfss_tt_blocking.txt
python scripts/solve_test_data.py  --algorithm milp-disjunctive  --output test/algorithms/milp_disjunctive_test.txt  --instances \
        data/test_jss_makespan.txt \
        data/test_jss_makespan_no_wait.txt \
        data/test_jss_makespan_blocking.txt \
        data/test_jss_tft.txt \
        data/test_jss_tft_no_wait.txt \
        data/test_jss_tft_blocking.txt \
        data/test_jss_twft.txt \
        data/test_jss_twft_no_wait.txt \
        data/test_jss_twft_blocking.txt \
        data/test_jss_tt.txt \
        data/test_jss_tt_no_wait.txt \
        data/test_jss_tt_blocking.txt \
        data/test_jss_twt.txt \
        data/test_jss_twt_no_wait.txt \
        data/test_jss_twt_blocking.txt \
        data/test_fjss_makespan.txt \
        data/test_fjss_makespan_no_wait.txt \
        data/test_fjss_makespan_blocking.txt \
        data/test_fjss_tft.txt \
        data/test_fjss_tft_no_wait.txt \
        data/test_fjss_tft_blocking.txt \
        data/test_fjss_twft.txt \
        data/test_fjss_twft_no_wait.txt \
        data/test_fjss_twft_blocking.txt \
        data/test_fjss_tt.txt \
        data/test_fjss_tt_no_wait.txt \
        data/test_fjss_tt_blocking.txt \
        data/test_fjss_twt.txt \
        data/test_fjss_twt_no_wait.txt \
        data/test_fjss_twt_blocking.txt \
        data/test_oss_makespan.txt \
        data/test_oss_makespan_no_wait.txt \
        data/test_oss_makespan_blocking.txt \
        data/test_oss_tft.txt \
        data/test_oss_tft_no_wait.txt \
        data/test_oss_tft_blocking.txt \
        data/test_oss_twft.txt \
        data/test_oss_twft_no_wait.txt \
        data/test_oss_twft_blocking.txt \
        data/test_oss_tt.txt \
        data/test_oss_tt_no_wait.txt \
        data/test_oss_tt_blocking.txt \
        data/test_oss_twt.txt \
        data/test_oss_twt_no_wait.txt \
        data/test_oss_twt_blocking.txt \
        data/test_foss_makespan.txt \
        data/test_foss_makespan_no_wait.txt \
        data/test_foss_makespan_blocking.txt \
        data/test_foss_tft.txt \
        data/test_foss_tft_no_wait.txt \
        data/test_foss_tft_blocking.txt \
        data/test_foss_twft.txt \
        data/test_foss_twft_no_wait.txt \
        data/test_foss_twft_blocking.txt \
        data/test_foss_tt.txt \
        data/test_foss_tt_no_wait.txt \
        data/test_foss_tt_blocking.txt \
        data/test_foss_twt.txt \
        data/test_foss_twt_no_wait.txt \
        data/test_foss_twt_blocking.txt
python scripts/solve_test_data.py  --algorithm constraint-programming-optalcp  --output test/algorithms/constraint_programming_optalcp_test.txt  --instances \
        data/test_pfss_makespan.txt \
        data/test_pfss_makespan_no_idle.txt \
        data/test_pfss_makespan_blocking.txt \
        data/test_pfss_tft.txt \
        data/test_pfss_tft_no_idle.txt \
        data/test_pfss_tft_blocking.txt \
        data/test_pfss_tt.txt \
        data/test_pfss_tt_no_idle.txt \
        data/test_pfss_tt_blocking.txt \
        data/test_jss_makespan.txt \
        data/test_jss_makespan_no_wait.txt \
        data/test_jss_makespan_blocking.txt \
        data/test_jss_tft.txt \
        data/test_jss_tft_no_wait.txt \
        data/test_jss_tft_blocking.txt \
        data/test_jss_twft.txt \
        data/test_jss_twft_no_wait.txt \
        data/test_jss_twft_blocking.txt \
        data/test_jss_tt.txt \
        data/test_jss_tt_no_wait.txt \
        data/test_jss_tt_blocking.txt \
        data/test_jss_twt.txt \
        data/test_jss_twt_no_wait.txt \
        data/test_jss_twt_blocking.txt \
        data/test_fjss_makespan.txt \
        data/test_fjss_makespan_no_wait.txt \
        data/test_fjss_makespan_blocking.txt \
        data/test_fjss_tft.txt \
        data/test_fjss_tft_no_wait.txt \
        data/test_fjss_tft_blocking.txt \
        data/test_fjss_twft.txt \
        data/test_fjss_twft_no_wait.txt \
        data/test_fjss_twft_blocking.txt \
        data/test_fjss_tt.txt \
        data/test_fjss_tt_no_wait.txt \
        data/test_fjss_tt_blocking.txt \
        data/test_fjss_twt.txt \
        data/test_fjss_twt_no_wait.txt \
        data/test_fjss_twt_blocking.txt \
        data/test_oss_makespan.txt \
        data/test_oss_makespan_no_wait.txt \
        data/test_oss_makespan_blocking.txt \
        data/test_oss_tft.txt \
        data/test_oss_tft_no_wait.txt \
        data/test_oss_tft_blocking.txt \
        data/test_oss_twft.txt \
        data/test_oss_twft_no_wait.txt \
        data/test_oss_twft_blocking.txt \
        data/test_oss_tt.txt \
        data/test_oss_tt_no_wait.txt \
        data/test_oss_tt_blocking.txt \
        data/test_oss_twt.txt \
        data/test_oss_twt_no_wait.txt \
        data/test_oss_twt_blocking.txt \
        data/test_foss_makespan.txt \
        data/test_foss_makespan_no_wait.txt \
        data/test_foss_makespan_blocking.txt \
        data/test_foss_tft.txt \
        data/test_foss_tft_no_wait.txt \
        data/test_foss_tft_blocking.txt \
        data/test_foss_twft.txt \
        data/test_foss_twft_no_wait.txt \
        data/test_foss_twft_blocking.txt \
        data/test_foss_tt.txt \
        data/test_foss_tt_no_wait.txt \
        data/test_foss_tt_blocking.txt \
        data/test_foss_twt.txt \
        data/test_foss_twt_no_wait.txt \
        data/test_foss_twt_blocking.txt
python scripts/solve_test_data.py --algorithm tree-search-pfss-makespan  --output test/algorithms/tree_search_pfss_makespan_test.txt  --instances \
        data/test_pfss_makespan.txt
python scripts/solve_test_data.py --algorithm tree-search-pfss-tft  --output test/algorithms/tree_search_pfss_tft_test.txt  --instances \
        data/test_pfss_tft.txt
cmake --build build --config Release --parallel  &&  cmake --install build --config Release --prefix install
ctest --parallel --output-on-failure  --test-dir build/test
```
