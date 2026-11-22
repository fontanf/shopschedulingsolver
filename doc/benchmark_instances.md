# Benchmark instances

The benchmark instances are not stored in the repository. They are in the `benchmarks` archive of the release [`data-benchmarks-v1`](https://github.com/fontanf/shopschedulingsolver/releases/tag/data-benchmarks-v1), pinned in `data/data.json` and downloaded in `data/` by:
```shell
python3 scripts/download_data.py --archives benchmarks
```

Each directory of `data/` holds one set of instances from the literature. It is named after the first author and the year of the publication that introduced the set; a `doi.url` or `link.url` file in some directories points to that publication. The files are in the format of the original authors, except for the flexible job shop instances, which are in the format of the collection of Behnke and Geiger (2012).

The instances used by the unit tests are described at the [end of this page](#test-instances).

## Overview

| Directory | Problem | Instances | Jobs × machines | `--format` |
|---|---|---:|---|---|
| [`taillard1993`](#taillard1993) | Permutation flow shop | 120 | 20×5 to 500×20 | `flow-shop` |
| [`vallada2015`](#vallada2015) | Permutation flow shop | 480 | 10×5 to 800×60 | `flow-shop` |
| [`vallada2008`](#vallada2008) | Permutation flow shop, total tardiness | 540 | 50×10 to 350×50 | `flow-shop-vallada2008` |
| [`ruiz2008`](#ruiz2008) | Permutation flow shop, sequence-dependent setup times | 480 | 20×5 to 500×20 | not supported |
| [`ruiz2009`](#ruiz2009) | No-idle permutation flow shop | 250 + 15 | 50×10 to 500×50 | not supported |
| [`naderi2010`](#naderi2010) | Distributed permutation flow shop | 1140 + 84 | 4×2 to 500×20 | not supported |
| [`ruiz2007`](#ruiz2007) | No-wait flow shop, separate setup times, maximum lateness | 6000 + 60 | 20×3 to 100×20 | not supported |
| [`muth1963`](#job-shop) | Job shop | 3 | 6×6 to 20×5 | `job-shop` |
| [`lawrence1984`](#job-shop) | Job shop | 40 | 10×5 to 30×10 | `job-shop` |
| [`adams1988`](#job-shop) | Job shop | 5 | 10×10 to 20×15 | `job-shop` |
| [`applegate1991`](#job-shop) | Job shop | 10 | 10×10 | `job-shop` |
| [`storer1992`](#job-shop) | Job shop | 20 | 20×10 to 50×10 | `job-shop` |
| [`yamada1992`](#job-shop) | Job shop | 4 | 20×20 | `job-shop` |
| [`taillard1993_jss`](#job-shop) | Job shop | 80 | 15×15 to 100×20 | `job-shop` |
| [`brandimarte1993`](#brandimarte1993) | Flexible job shop | 15 | 10×6 to 30×15 | `flexible-job-shop` |
| [`hurink1994`](#hurink1994) | Flexible job shop (multi-purpose machines) | 4 × 66 | 6×6 to 30×10 | `flexible-job-shop` |
| [`dauzere1994`](#dauzere1994) | Flexible job shop (multiprocessor job shop) | 18 | 10×5 to 20×10 | `flexible-job-shop` |
| [`barnes1995`](#barnes1995) | Flexible job shop | 21 | 10×11 to 15×18 | `flexible-job-shop` |
| [`kacem2002`](#kacem2002) | Flexible job shop | 4 | 4×5 to 15×10 | `flexible-job-shop` |
| [`fattahi2007`](#fattahi2007) | Flexible job shop | 20 | 2×2 to 12×8 | `flexible-job-shop` |
| [`behnke2012`](#behnke2012) | Flexible job shop with work centers | 60 | 10×20 to 100×60 | `flexible-job-shop` |
| [`taillard1993_oss`](#open-shop) | Open shop | 60 | 4×4 to 20×20 | not supported |
| [`brucker1997`](#open-shop) | Open shop | 52 | 3×3 to 8×8 | not supported |
| [`gueret1998`](#open-shop) | Open shop | 80 | 3×3 to 10×10 | not supported |
| [`abreu2023`](#open-shop) | Open shop | 30 | 25×25 to 40×40 | not supported |

In the "Instances" column, the second number is the number of instances of the separate calibration sets (the `*_TEST` directories), generated like the others and meant to tune the parameters of an algorithm without using the instances on which it is evaluated.

"Not supported" means that `InstanceBuilder` has no reader for the format: the instances are in the archive, but the solver can't read them yet. Most of them are also for problems the solver doesn't model (setup times, distributed flow shop, maximum lateness).

All the instances define a single objective in the literature (given in the sections below), but the readers only read the data, and the objective is set with `--objective`. The same instances are therefore commonly used for other objectives: for example the instances of Taillard (1993) for the total flow time and for the blocking flow shop.

## Benchmark files

The files `data/data_*.csv` list the instances of a benchmark, with the best known solution value and bound of each instance and the results reported in the literature. They are read by `scripts/run_benchmarks.py`:

| File | Benchmark | Instances | Time limit | Columns of results from the literature |
|---|---|---|---|---|
| `data_pfss_makespan.csv` | $F_m \mid \text{prmu} \mid C_{\max}$ | `vallada2015` | $n \cdot m \cdot 45$ ms | `kizilay2019 (VIBH)` |
| `data_pfss_blocking_makespan.csv` | $F_m \mid \text{prmu}, \text{blocking} \mid C_{\max}$ | `taillard1993` | $n \cdot m \cdot 100$ ms | `teixeira2025 (HyPRR)`, `shao2019 (DIWO)`, `ribas2011 (IG)` |
| `data_pfss_tct.csv` | $F_m \mid \text{prmu} \mid \sum C_j$ | `taillard1993` | $n \cdot m \cdot 45$ ms | |
| `data_pfss_tt.csv` | $F_m \mid \text{prmu} \mid \sum T_j$ | `vallada2008` | $n \cdot m \cdot 120$ ms | `karabulut2016`, `fernandez2018` (8 algorithms, 3 stopping criteria) |

## Permutation flow shop

### taillard1993

Taillard (1993), "Benchmarks for basic scheduling problems". Designed for $F_m \mid \text{prmu} \mid C_{\max}$.

* 120 instances: 10 instances for each size $n \times m$ in $\{20, 50, 100\} \times \{5, 10, 20\}$, $200 \times \{10, 20\}$ and $500 \times 20$. Files `tai<n>_<m>_<k>.txt`, $k \in \{0, \dots, 9\}$.
* Processing times uniform in $[1, 99]$.
* Taillard generated many instances of each size, solved them roughly with a tabu search, and kept the 10 that seemed the hardest: the ones whose best makespan was far from a lower bound, and for which several runs of the tabu search from different initial solutions found different solutions.
* Each instance is defined by the seed of a given random generator, so the paper describes the instances completely and they can be regenerated.
* Format: the numbers of jobs and machines, then one line per machine with the processing times of the jobs.

The set has become the standard benchmark of the permutation flow shop for other objectives and constraints. In this repository, it is used for the total flow time and for the blocking flow shop. Taillard (1993) also proposed the job shop and open shop sets below.

### vallada2015

Vallada, Ruiz and Framinan (2015), "New hard benchmark for flowshop scheduling problems minimising makespan". Designed for $F_m \mid \text{prmu} \mid C_{\max}$, as a harder benchmark than the instances of Taillard (1993), with more discriminating power when comparing algorithms.

* `Small/`: 240 instances, 10 for each size $n \times m$ with $n \in \{10, 20, 30, 40, 50, 60\}$ and $m \in \{5, 10, 15, 20\}$.
* `Large/`: 240 instances, 10 for each size with $n \in \{100, 200, \dots, 800\}$ and $m \in \{20, 40, 60\}$.
* Processing times uniform in $[1, 99]$. The authors generated thousands of candidate instances and kept those with the largest gap between very good upper and lower bounds (the `_Gap` suffix of the file names).
* Files `VFR<n>_<m>_<k>_Gap.txt`. Same format as `taillard1993`.

### vallada2008

Vallada, Ruiz and Minella (2008), "Minimising total tardiness in the m-machine flowshop problem: A review and evaluation of heuristics and metaheuristics". Designed for $F_m \mid \text{prmu} \mid \sum T_j$.

* 540 instances: 5 instances for each combination of $n \in \{50, 150, 250, 350\}$, $m \in \{10, 30, 50\}$, a tardiness factor $T \in \{0.2, 0.4, 0.6\}$ and a due date range $R \in \{0.2, 0.6, 1\}$. Files `I_<T>_<R>_<n>_<m>_<k>.txt`, with a decimal comma.
* Processing times uniform in $[1, 99]$.
* Due dates uniform in $[P(1 - T - R/2), P(1 - T + R/2)]$ (method of Potts and Van Wassenhove, 1982), with $P$ the lower bound of the makespan of Taillard (1993). Negative due dates are replaced by 0.
* Format: one line per job with pairs (machine, processing time), then a section `Reldue` with one line per job `-1 <due date> -1 <weight>`, and a line `LBCmax: <P>`. The reader `flow-shop-vallada2008` reads the processing times and the due dates; it ignores the weights.

### ruiz2008

Ruiz and Stützle (2008), "An Iterated Greedy heuristic for the sequence dependent setup times flowshop problem with makespan and weighted tardiness objectives". Designed for the permutation flow shop with sequence-dependent setup times, with objectives $C_{\max}$ and $\sum w_j T_j$.

* 480 instances: the 120 instances of Taillard (1993), with sequence-dependent setup times added on each machine. Directories `ssd10`, `ssd50`, `ssd100` and `ssd125`: the setup times are uniform in $[1, 9]$, $[1, 49]$, $[1, 99]$ and $[1, 124]$, so at most 10%, 50%, 100% and 125% of the maximum processing time.
* The files `DD_SDST<s>_ta<k>.txt` also have the weights and the due dates of the weighted tardiness version (the sets "DD_SDST" of the paper). The weights are uniform in $[1, 10]$, and the due date of job $j$ is $d_j = (P_j + S_j)(1 + 3 u)$, where $P_j$ is the sum of its processing times, $S_j$ the sum over the machines of its average setup time to the other jobs, and $u$ uniform in $[0, 1]$. The due dates are tight.
* Format: the processing times as in `vallada2008`, then a section `SSD` with one $n \times n$ matrix of setup times per machine (`M0`, `M1`, ...), then a section `Reldue` as in `vallada2008`.

### ruiz2009

Ruiz, Vallada and Fernández-Martínez (2009), "Scheduling in flowshops with no-idle machines". Designed for the no-idle permutation flow shop with objective $C_{\max}$.

* `PFSP_NoIdle/`: 250 instances, 5 for each combination of $n \in \{50, 100, \dots, 500\}$ and $m \in \{10, 20, 30, 40, 50\}$. Processing times uniform in $[1, 99]$.
* `PFSP_NoIdle_TEST/`: 15 calibration instances.
* The files are those of the mixed no-idle benchmark of Pan and Ruiz (2014). That benchmark has 7 groups of 250 instances, with the same processing times and different sets of no-idle machines: the first half of the machines, the second half, every other machine, a random 25%, 50% or 75% of the machines, and all the machines. Only group 7, in which all the machines are no-idle and which is the original set of Ruiz et al. (2009), is in the archive (files `I_7_<n>_<m>_<k>.txt`).
* Format: the processing times as in `vallada2008`, then a section `NOIDLE` with a line `1` and a line with one 0/1 flag per machine.

### naderi2010

Naderi and Ruiz (2010), "The distributed permutation flowshop scheduling problem". Designed for the distributed permutation flow shop with objective $C_{\max}$: the jobs are assigned to $F$ identical factories, each being a permutation flow shop, and the makespan is the largest makespan of the factories.

* `DPFSP_Small/<F>/`: 420 instances, 5 for each combination of $n \in \{4, 6, \dots, 16\}$, $m \in \{2, 3, 4, 5\}$ and $F \in \{2, 3, 4\}$. Processing times uniform in $[1, 99]$. Small enough to be solved by the MILP models of the paper.
* `DPFSP_Large/<F>/`: 720 instances, the 120 instances of Taillard (1993) with $F \in \{2, \dots, 7\}$. Files `Ta<k>_<F>.txt`.
* `DPFSP_Small_TEST/` and `DPFSP_Large_TEST/`: 34 and 50 calibration instances.
* Format: the numbers of jobs and machines, then $F$, then the processing times as in `vallada2008`.

The same instances are used for the total flow time (Pan et al., 2019).

### ruiz2007

Ruiz and Allahverdi (2007), "No-wait flowshop with separate setup times to minimize maximum lateness". Designed for the no-wait flow shop with sequence-independent setup times separated from the processing times (the setup of a machine can be done before the job arrives), with objective $L_{\max}$.

* 6000 instances: 10 instances for each combination of $n \in \{20, 40, 60, 80, 100\}$, $m \in \{3, 5, 10, 15, 20\}$, 4 distributions of the processing times, 3 distributions of the setup times and 2 due date settings.
* The directories `Ali No-Wait SISD Lateness <d>` correspond to the distributions of the processing times: uniform in $[1, 5]$, $[1, 10]$ or $[1, 100]$, or normal with mean 100 and standard deviation 2. Each one is split into `m=3` and `mgt3` ($m > 3$).
* The setup times are uniform with a maximum equal to 50%, 100% or 150% of the maximum processing time.
* The due dates are uniform, generated from a lower bound of the makespan with $(T, R) = (0.3, 0.9)$ (looser and more spread) or $(0.5, 0.5)$ (tight and clustered).
* Files `I_<T>_<R>_<s>_<n>_<m>_<k>.txt`, with $s \in \{0.5, 1, 1.5\}$ the ratio of the setup times.
* `Ali No-Wait SISD Lateness TEST/`: 60 calibration instances.
* Format: the processing times as in `vallada2008`, then a section `SIST` with the setup times of each machine (`M0`, `M1`, ...), then a section `Reldue` and a line `LBCmax:` as in `vallada2008`.

## Job shop

The classical job shop instances, designed for $J_m \parallel C_{\max}$. Each job has exactly one operation per machine, and the processing times are integers. Except for `taillard1993_jss`, the files come from the OR-Library (Beasley, 1990), with a header in comments giving the origin of the instance.

| Directory | Instances | Sizes | Reference |
|---|---|---|---|
| `muth1963` | ft06, ft10, ft20 | 6×6, 10×10, 20×5 | Fisher and Thompson (1963). ft10 stayed unsolved for 26 years. Also called mt06, mt10, mt20. |
| `lawrence1984` | la01–la40 | 10×5 to 30×10, 15×15 | Lawrence (1984) |
| `adams1988` | abz5–abz9 | 10×10, 20×15 | Adams, Balas and Zawack (1988) |
| `applegate1991` | orb01–orb10 | 10×10 | Applegate and Cook (1991). The instances were proposed by several people; the header of each file gives its author. |
| `storer1992` | swv01–swv20 | 20×10, 20×15, 50×10 | Storer, Wu and Vaccari (1992). swv01–swv15 are labeled hard, swv16–swv20 easy. |
| `yamada1992` | yn1–yn4 | 20×20 | Yamada and Nakano (1992) |
| `taillard1993_jss` | ta01–ta80 | 15×15 to 100×20 | Taillard (1993). 10 instances for each size; processing times uniform in $[1, 99]$, each job visiting the machines in a random order; selected as the hardest of many generated instances, as for the flow shop. |

Format: the numbers of jobs and machines, then one line per job with the pairs (machine, processing time) of its operations in order, the machines being numbered from 0. The lines starting with `#` are comments.

## Flexible job shop

Designed for $FJ_m \parallel C_{\max}$: each operation can be processed by one machine of a set of eligible machines, with a processing time that depends on the machine.

The files come from the collection of Behnke and Geiger (2012), which gathers the instances most used in the literature, gives their lower and upper bounds, and adds a new set (`behnke2012`). Format: a line with the number of jobs, the number of machines and the average number of machines per operation, then one line per job: its number of operations and, for each operation, its number of eligible machines $k$ followed by $k$ pairs (machine, processing time), the machines being numbered from 1.

### brandimarte1993

Brandimarte (1993), "Routing and scheduling in a flexible job shop by tabu search". The first instances of the general flexible job shop.

* 15 instances, Mk1–Mk15, with 10 to 30 jobs, 4 to 15 machines and 3 to 15 operations per job.
* The number of operations of each job, the eligible machines and the processing times are uniform in ranges which depend on the instance. Depending on the instance, an operation has at most 2 to 6 eligible machines (medium flexibility).
* Mk11–Mk15 (30 jobs, processing times in $[10, 30]$) are less used than Mk1–Mk10.

### hurink1994

Hurink, Jurisch and Thole (1994), "Tabu search for the job-shop scheduling problem with multi-purpose machines". Designed for the job shop with multi-purpose machines: the processing time of an operation doesn't depend on the machine.

* 66 classical job shop instances: ft06, ft10, ft20, la01–la40, abz5–abz9, car1–car8 and orb01–orb10.
* 4 sets, one per directory, built from them by enlarging at random the set of eligible machines of each operation:
  * `sdata`: the original job shop instances (one machine per operation).
  * `edata`: few operations have more than one eligible machine (1.15 eligible machines per operation on average).
  * `rdata`: most operations have a few eligible machines (1.97 on average).
  * `vdata`: all operations have several eligible machines (4.13 on average).

### dauzere1994

Dauzère-Pérès and Paulli (1994), "Solving the general multiprocessor job-shop scheduling problem" (technical report; the instances are also used in Dauzère-Pérès and Paulli, 1997).

* 18 instances (`DPpaulli<k>a.fjs`): 10 jobs and 5 machines (1–6), 15 jobs and 8 machines (7–12), 20 jobs and 10 machines (13–18).
* 15 to 25 operations per job, processing times in $[10, 100]$.
* Each machine is eligible for an operation with probability 0.1, 0.3 or 0.5 (if no machine is selected, one is chosen at random), so the instances have a high flexibility. Within each group, the instances differ by this probability and by how much the processing times of an operation vary across its eligible machines.

### barnes1995

Chambers and Barnes (1996), "Flexible job shop scheduling by tabu search" (technical report). The `doi.url` file points to Barnes and Chambers (1995), the job shop tabu search which this work extends.

* 21 instances (`ChambersBarnes<k>.fjs`), built from three hard job shop instances: mt10 (ft10, 10×10), la24 (setb4, 15×10) and la40 (seti5, 15×15).
* The flexible instances are obtained by replicating machines: the machines with the largest total processing time, or with the largest number of operations on the critical path of the best known solutions, are duplicated, giving 11 to 18 machines.

### kacem2002

Kacem, Hammadi and Borne (2002), "Pareto-optimality approach for flexible job-shop scheduling problems: hybridization of evolutionary algorithms and fuzzy logic".

* 4 instances: 4×5, 10×7, 10×10 and 15×10.
* Total flexibility: in the files, every machine is eligible for every operation.
* The paper also gives release dates, which are not in the files.

### fattahi2007

Fattahi, Saidi Mehrabad and Jolai (2007), "Mathematical modeling and heuristic approaches to flexible job shop scheduling problems". Designed to evaluate MILP models.

* 20 instances: 10 small (SFJS1–SFJS10, `Fattahi1`–`Fattahi10`, 2 to 4 jobs and 2 to 5 machines) and 10 medium (MFJS1–MFJS10, `Fattahi11`–`Fattahi20`, 5 to 12 jobs and 6 to 8 machines).
* 2 to 4 operations per job, each with 1 to 3 eligible machines.

### behnke2012

Behnke and Geiger (2012), "Test instances for the flexible job shop scheduling problem with work centers". Designed for the flexible job shop with work centers, a variant in which similar machines are pooled.

* 60 instances: 5 for each combination of $n \in \{10, 20, 50, 100\}$ and 5, 10 or 15 work centers of 4 machines each ($m \in \{20, 40, 60\}$).
* Each job has exactly 5 operations. The first operation of each job is processed in the first work center, the last one in the last work center; each other operation can be processed in a random number of work centers.
* If an operation can be processed in a work center, it can be processed on all its machines, with processing times uniform in $[10, 30]$ which depend on the machine.

## Open shop

Designed for $O_m \parallel C_{\max}$. The files only contain the matrix of the processing times (there is no machine order in an open shop).

| Directory | Instances | Sizes | Reference |
|---|---|---|---|
| `taillard1993_oss` | 60 | $n = m \in \{4, 5, 7, 10, 15, 20\}$ | Taillard (1993). 10 instances per size, processing times uniform in $[1, 99]$, selected as the hardest of many generated instances. |
| `brucker1997` | 52 | $n = m \in \{3, \dots, 8\}$ | Brucker, Hurink, Jurisch and Wöstmann (1997). Files `j<n>-per<p>-<k>.txt`: in the instances `per0`, every job and every machine has a total processing time of 1000, so the trivial lower bound is the same for all of them; with `per10` and `per20`, the total processing times are up to 10% and 20% lower. These instances are harder than those of Taillard (1993), whose optimal makespan is often equal to the lower bound. |
| `gueret1998` | 80 | $n = m \in \{3, \dots, 10\}$ | Guéret and Prins (1999), "A new lower bound for the open-shop problem" (the directory name follows Abreu et al. (2023), who cite Guéret and Prins (1998)). 10 instances per size. Every job and every machine has a total processing time of 1000, the processing times being in $[1, 1000]$. |
| `abreu2023` | 30 | $n = m \in \{25, 30, 40\}$ | Abreu, Nagano and Prata (2023), "A new two-stage constraint programming approach for open shop scheduling problem with machine blocking". Generated for the open shop with blocking, as larger instances than the three sets above. 10 instances per size, generated like those of Guéret and Prins: every job and every machine has a total processing time of 1000. |

Abreu et al. (2023) use the four sets for the open shop with blocking.

## Test instances

The unit tests don't use the benchmark instances but small generated instances, in the `tests` archive (release `data-tests-v2`), downloaded in `data/tests/`:
```shell
python3 scripts/download_data.py --archives tests
```

They are generated by `scripts/generate_data.py` with `shopschedulingsolver_generator` (`src/generator.cpp`), for each shop type (permutation flow shop, flow shop, flexible flow shop, job shop, flexible job shop, open shop and flexible open shop), each objective (makespan, total (weighted) flow time, total (weighted) tardiness and throughput) and each constraint (none, no-wait, no-idle and blocking):
* 1 to 8 jobs, 1 to 3 machine groups of 1 machine (or 2 for the flexible variants), and 2 seeds. Files `<type>_<objective>[_<constraint>]_n<n>_m<groups>x<machines per group>_s<seed>.json`.
* Each job has one operation per machine group. The operations visit the groups in order in the flow shop and in a random order in the job shop and the open shop. In the flexible variants, each operation can be processed on every machine of its group, with a processing time that depends on the machine.
* Processing times uniform in $[1, 100]$; weights uniform in $[1, 100]$ for the weighted objectives and the throughput, 1 otherwise.
* Due date of job $j$: $d_j = \lfloor f \sum_o p_{jo} \rfloor$ (Eilon and Hodgson, 1967), with $f = 1$ and, for a flexible operation, the mean of its processing times. The factor is lower than the ones used in the literature for larger instances, so that most instances have a positive optimal tardiness.

The lists `data/test_*.txt` give the instances of each combination. The expected solutions (`*_solution.json`) are not in the archive: each test computes them or reads them from its own list in `test/algorithms/` (see the README).

In the instances of `data-tests-v2`, no machine is no-idle (`"no_idle": false` on every machine, because of a bug of the generator, fixed since then), so the no-idle lists don't test the no-idle constraints.

`data/handmade/` contains a few hand-made instances, each with a reference optimal solution, stored in the repository. They cover cases which the generated instances don't: a permutation flow shop with no-idle machines, a flexible job shop with no-idle machines, and a flexible open shop with blocking in which the optimal schedule blocks a machine.

## References

* E. Taillard (1993). Benchmarks for basic scheduling problems. *European Journal of Operational Research*, 64(2), 278–285. [doi:10.1016/0377-2217(93)90182-M](https://doi.org/10.1016/0377-2217(93)90182-M)
* E. Vallada, R. Ruiz and J. M. Framinan (2015). New hard benchmark for flowshop scheduling problems minimising makespan. *European Journal of Operational Research*, 240(3), 666–677. [doi:10.1016/j.ejor.2014.07.033](https://doi.org/10.1016/j.ejor.2014.07.033)
* E. Vallada, R. Ruiz and G. Minella (2008). Minimising total tardiness in the m-machine flowshop problem: A review and evaluation of heuristics and metaheuristics. *Computers & Operations Research*, 35(4), 1350–1373. [doi:10.1016/j.cor.2006.08.016](https://doi.org/10.1016/j.cor.2006.08.016)
* C. N. Potts and L. N. Van Wassenhove (1982). A decomposition algorithm for the single machine total tardiness problem. *Operations Research Letters*, 1(5), 177–181. [doi:10.1016/0167-6377(82)90035-9](https://doi.org/10.1016/0167-6377(82)90035-9)
* R. Ruiz and T. Stützle (2008). An Iterated Greedy heuristic for the sequence dependent setup times flowshop problem with makespan and weighted tardiness objectives. *European Journal of Operational Research*, 187(3), 1143–1159. [doi:10.1016/j.ejor.2006.07.029](https://doi.org/10.1016/j.ejor.2006.07.029)
* R. Ruiz, E. Vallada and C. Fernández-Martínez (2009). Scheduling in flowshops with no-idle machines. In U. K. Chakraborty (ed.), *Computational Intelligence in Flow Shop and Job Shop Scheduling*, Springer, 21–51. [doi:10.1007/978-3-642-02836-6_2](https://doi.org/10.1007/978-3-642-02836-6_2)
* Q.-K. Pan and R. Ruiz (2014). An effective iterated greedy algorithm for the mixed no-idle permutation flowshop scheduling problem. *Omega*, 44, 41–50. [doi:10.1016/j.omega.2013.10.002](https://doi.org/10.1016/j.omega.2013.10.002)
* B. Naderi and R. Ruiz (2010). The distributed permutation flowshop scheduling problem. *Computers & Operations Research*, 37(4), 754–768. [doi:10.1016/j.cor.2009.06.019](https://doi.org/10.1016/j.cor.2009.06.019)
* Q.-K. Pan, L. Gao, L. Wang, J. Liang and X.-Y. Li (2019). Effective heuristics and metaheuristics to minimize total flowtime for the distributed permutation flowshop problem. *Expert Systems with Applications*, 124, 309–324. [doi:10.1016/j.eswa.2019.01.062](https://doi.org/10.1016/j.eswa.2019.01.062)
* R. Ruiz and A. Allahverdi (2007). No-wait flowshop with separate setup times to minimize maximum lateness. *The International Journal of Advanced Manufacturing Technology*, 35(5), 551–565. [doi:10.1007/s00170-006-0726-9](https://doi.org/10.1007/s00170-006-0726-9)
* J. E. Beasley (1990). OR-Library: distributing test problems by electronic mail. *Journal of the Operational Research Society*, 41(11), 1069–1072. [doi:10.1057/jors.1990.166](https://doi.org/10.1057/jors.1990.166)
* H. Fisher and G. L. Thompson (1963). Probabilistic learning combinations of local job-shop scheduling rules. In J. F. Muth and G. L. Thompson (eds.), *Industrial Scheduling*, Prentice-Hall, 225–251.
* S. Lawrence (1984). Resource constrained project scheduling: an experimental investigation of heuristic scheduling techniques (Supplement). Graduate School of Industrial Administration, Carnegie-Mellon University.
* J. Adams, E. Balas and D. Zawack (1988). The shifting bottleneck procedure for job shop scheduling. *Management Science*, 34(3), 391–401. [doi:10.1287/mnsc.34.3.391](https://doi.org/10.1287/mnsc.34.3.391)
* D. Applegate and W. Cook (1991). A computational study of the job-shop scheduling problem. *ORSA Journal on Computing*, 3(2), 149–156. [doi:10.1287/ijoc.3.2.149](https://doi.org/10.1287/ijoc.3.2.149)
* R. H. Storer, S. D. Wu and R. Vaccari (1992). New search spaces for sequencing problems with application to job shop scheduling. *Management Science*, 38(10), 1495–1509. [doi:10.1287/mnsc.38.10.1495](https://doi.org/10.1287/mnsc.38.10.1495)
* T. Yamada and R. Nakano (1992). A genetic algorithm applicable to large-scale job-shop problems. In *Parallel Problem Solving from Nature 2*, Elsevier, 281–290.
* P. Brandimarte (1993). Routing and scheduling in a flexible job shop by tabu search. *Annals of Operations Research*, 41(3), 157–183. [doi:10.1007/BF02023073](https://doi.org/10.1007/BF02023073)
* J. Hurink, B. Jurisch and M. Thole (1994). Tabu search for the job-shop scheduling problem with multi-purpose machines. *OR Spektrum*, 15(4), 205–215. [doi:10.1007/BF01719451](https://doi.org/10.1007/BF01719451)
* S. Dauzère-Pérès and J. Paulli (1994). Solving the general multiprocessor job-shop scheduling problem. Management Report Series 182, Rotterdam School of Management, Erasmus Universiteit Rotterdam. [pure.eur.nl](https://pure.eur.nl/en/publications/solving-the-general-multiprocessor-job-shop-scheduling-problem)
* S. Dauzère-Pérès and J. Paulli (1997). An integrated approach for modeling and solving the general multiprocessor job-shop scheduling problem using tabu search. *Annals of Operations Research*, 70, 281–306. [doi:10.1023/A:1018930406487](https://doi.org/10.1023/A:1018930406487)
* J. W. Barnes and J. B. Chambers (1995). Solving the job shop scheduling problem with tabu search. *IIE Transactions*, 27(2), 257–263. [doi:10.1080/07408179508936739](https://doi.org/10.1080/07408179508936739)
* J. B. Chambers and J. W. Barnes (1996). Flexible job shop scheduling by tabu search. Technical Report ORP96-09, Graduate Program in Operations Research and Industrial Engineering, The University of Texas at Austin.
* I. Kacem, S. Hammadi and P. Borne (2002). Pareto-optimality approach for flexible job-shop scheduling problems: hybridization of evolutionary algorithms and fuzzy logic. *Mathematics and Computers in Simulation*, 60(3–5), 245–276. [doi:10.1016/S0378-4754(02)00019-8](https://doi.org/10.1016/S0378-4754(02)00019-8)
* P. Fattahi, M. Saidi Mehrabad and F. Jolai (2007). Mathematical modeling and heuristic approaches to flexible job shop scheduling problems. *Journal of Intelligent Manufacturing*, 18(3), 331–342. [doi:10.1007/s10845-007-0026-8](https://doi.org/10.1007/s10845-007-0026-8)
* D. Behnke and M. J. Geiger (2012). Test instances for the flexible job shop scheduling problem with work centers. Research Report RR-12-01-01, Helmut-Schmidt-Universität, Hamburg. [doi:10.24405/436](https://doi.org/10.24405/436)
* P. Brucker, J. Hurink, B. Jurisch and B. Wöstmann (1997). A branch & bound algorithm for the open-shop problem. *Discrete Applied Mathematics*, 76(1–3), 43–59. [doi:10.1016/S0166-218X(96)00116-3](https://doi.org/10.1016/S0166-218X(96)00116-3)
* C. Guéret and C. Prins (1998). Classical and new heuristics for the open-shop problem: A computational evaluation. *European Journal of Operational Research*, 107(2), 306–314. [doi:10.1016/S0377-2217(97)00332-9](https://doi.org/10.1016/S0377-2217(97)00332-9)
* C. Guéret and C. Prins (1999). A new lower bound for the open-shop problem. *Annals of Operations Research*, 92, 165–183. [doi:10.1023/A:1018930613891](https://doi.org/10.1023/A:1018930613891)
* L. R. Abreu, M. S. Nagano and B. A. Prata (2023). A new two-stage constraint programming approach for open shop scheduling problem with machine blocking. *International Journal of Production Research*, 61(24), 8560–8579. [doi:10.1080/00207543.2022.2154404](https://doi.org/10.1080/00207543.2022.2154404)
* S. Eilon and R. M. Hodgson (1967). Job shops scheduling with due dates. *International Journal of Production Research*, 6(1), 1–13. [doi:10.1080/00207546708929764](https://doi.org/10.1080/00207546708929764)
