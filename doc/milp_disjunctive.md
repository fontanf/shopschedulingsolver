# Disjunctive MILP

`--algorithm milp-disjunctive`

Source: [`src/algorithms/milp_disjunctive.cpp`](../src/algorithms/milp_disjunctive.cpp)

## Algorithm

Mixed-integer linear programming model with big-M disjunctive constraints, for the job shop, the open shop and their flexible variants. The no-wait open shop part corresponds to model 3 of Naderi and Zandieh (2014), and the flexible part to model 5 of Roshanaei, Azab and ElMaraghy (2013).

Let $(j, o)$ be operation $o$ of job $j$, and $k$ one of its alternatives (a machine and a processing time $p_{jok}$). Variables:
* $C_{jo} \ge 0$: completion time of operation $(j, o)$.
* $y_{jo, j'o'} \in \{0, 1\}$, for each pair of machine operations on the same machine: $y = 1$ iff $(j, o)$ is scheduled before $(j', o')$.
* $x_{jok} \in \{0, 1\}$ (flexible only): $x_{jok} = 1$ iff alternative $k$ is selected for $(j, o)$, with $\sum_k x_{jok} = 1$.
* $P_{jo}$ (flexible or blocking only): time during which $(j, o)$ occupies its machine. In the flexible case, $P_{jo} = \sum_k p_{jok} x_{jok}$; with blocking, $P_{jo} \ge$ this value, the difference being the time during which the operation blocks the machine.
* $C_{jok}$ (flexible only): completion time of $(j, o)$ if alternative $k$ is selected, 0 otherwise, with $C_{jok} \le M x_{jok}$ and $C_{jo} = \sum_k C_{jok}$.
* $z_{j, oo'} \in \{0, 1\}$ (open shop only): $z = 1$ iff operation $o$ of job $j$ is scheduled before operation $o'$.
* $C_{\max}$, $C_j$ (open shop with total flow time), $T_j$ (total tardiness).

The big-M constant is $M = \sum_{j, o} \max_k p_{jok}$.

Constraints:
* Machine disjunctions, for each pair of operations on the same machine (the $x$ terms only appear in the flexible case):
  $$C_{jo} \ge C_{j'o'} + P_{jo} - M y_{jo, j'o'} - M (2 - x_{jok} - x_{j'o'k'})$$
  $$C_{j'o'} \ge C_{jo} + P_{j'o'} - M (1 - y_{jo, j'o'}) - M (2 - x_{jok} - x_{j'o'k'})$$
* Job shop, precedences between consecutive operations: $C_{j,o+1} \ge C_{jo} + P_{j,o+1}$, with an equality with no-wait or blocking. With blocking, $P_{jo}$ is the occupation time, so the equality means that the operation leaves its machine when the next operation starts.
* Open shop, job disjunctions with the $z$ variables, in the same way as the machine disjunctions.
* Open shop with no-wait or blocking: with $S_j$ the start of job $j$, $S_j \le C_{jo} - P_{jo}$ and $C_{jo} - S_j \le \sum_{o'} P_{jo'}$. All operations of the job lie in a window whose length is the sum of their durations, so they are processed without interruption.
* No-idle machine $i$: with $S_i$ the start of the machine, $S_i \le C_{jo} - p_{jo}$ and $C_{jo} - S_i \le \sum_{(j', o') \text{ on } i} p_{j'o'}$ for each operation on the machine. In the flexible case, the same constraints are written on $C_{jok}$ with big-M terms, and the sum of processing times of the machine is a variable.
* $C_{\max} \ge C_{jo}$; $T_j \ge C_{jo} - d_j$ for the last operation (job shop) or all operations (open shop) of job $j$.

Objectives:
* Makespan: $\min C_{\max}$.
* Total flow time: $\min \sum_j w_j C_j$, with $C_j$ the completion time of the last operation in the job shop.
* Total tardiness: $\min \sum_j w_j T_j$.

The solution is rebuilt from the selected alternatives and the completion times. With blocking, an operation starts at $C_{jo} - P_{jo}$.

Solvers: HiGHS (default), CBC and Xpress, chosen with `--solver highs|cbc|xpress`, as for the [positional MILP](milp_positional.md).

The model can also be written to an MPS file without solving it, with the `write_mps` function or the `shopschedulingsolver_mps_writer` executable.

## Supported problems

* Job shop $J_m$, flexible job shop $FJ_m$, open shop $O_m$ and flexible open shop $FO_m$.
* Objectives $C_{\max}$, $\sum w_j C_j$ and $\sum w_j T_j$.
* Constraints no-wait, blocking and mixed no-idle.

A flow shop is solved as a job shop. The permutation constraint is not modeled: use the [positional MILP](milp_positional.md) for the permutation flow shop. Release dates are ignored. With total tardiness, every job must have a due date.

## Optimality

Exact: when the solver terminates before the time limit, the solution is optimal and the bound is equal to its value.

## Proved optimality

The dual bound of the solver is reported through the callbacks and at the end, rounded up to the next integer. The output reports `"Optimal": true` when it is equal to the value of the solution.

## Tests

* `test/algorithms/milp_disjunctive_test.cpp` with the instances of `test/algorithms/milp_disjunctive_test.txt`: job shop, flexible job shop, open shop and flexible open shop instances, with makespan, total (weighted) flow time and total (weighted) tardiness, each with no constraint, no-wait and blocking. The no-idle lists are not part of the test.
* `HandmadeAlgorithmTest`, in the same file: hand-made instances of `data/handmade/`, each with a reference optimal solution, for cases which the generated instances don't cover: a flexible job shop with no-idle machines.

## References

* B. Naderi and M. Zandieh (2014). Modeling and scheduling no-wait open shop problems. *International Journal of Production Economics*, 158, 256–266. [doi:10.1016/j.ijpe.2014.06.011](https://doi.org/10.1016/j.ijpe.2014.06.011)
* V. Roshanaei, A. Azab and H. ElMaraghy (2013). Mathematical modelling and a meta-heuristic for flexible job shop scheduling. *International Journal of Production Research*, 51(20), 6247–6274. [doi:10.1080/00207543.2013.827806](https://doi.org/10.1080/00207543.2013.827806)
