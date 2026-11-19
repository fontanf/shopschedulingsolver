# Disjunctive MILP

`--algorithm milp-disjunctive`

Source: [`src/algorithms/milp_disjunctive.cpp`](../src/algorithms/milp_disjunctive.cpp)

## Supported problems

* Job shop $J_m$, flexible job shop $FJ_m$, open shop $O_m$ and flexible open shop $FO_m$.
* Objectives $C_{\max}$, $\sum w_j C_j$ and $\sum w_j T_j$.
* Constraints no-wait, blocking and mixed no-idle.

A flow shop is solved as a job shop. The permutation constraint is not modeled: use the [positional MILP](milp_positional.md) for the permutation flow shop. Release dates are ignored. With total tardiness, every job must have a due date.

## Optimality

Exact. The dual bound of the solver is reported through the callbacks and at the end, rounded up to the next integer. When the solver terminates before the time limit, the solution is optimal, the bound is equal to its value, and the output reports `"Optimal": true`.

## Algorithm

Mixed-integer linear programming model with big-M disjunctive constraints, for the job shop, the open shop and their flexible variants. The no-wait open shop part corresponds to model 3 of Naderi and Zandieh (2014), and the flexible part to model 5 of Roshanaei, Azab and ElMaraghy (2013).

The schedule is modeled by the completion times of the operations. For each pair of operations which can be processed on the same machine, a binary variable selects which one is first, and big-M constraints separate them accordingly. In the open shop, the operations of a job are separated in the same way.

## Model

**Input**:

* jobs $j = 1, \ldots, n$; for each job $j$: a weight $w_j$, a due date $d_j$ (total tardiness), and operations $o = 1, \ldots, \ell_j$, processed in this order in the job shop and in any order in the open shop
* for each operation $(j, o)$: alternatives $k$, each with a machine and a processing time $p_{jok}$; without flexibility, each operation has a single alternative, of processing time $p_{jo}$
* machines $i$; for each machine $i$: the set $A_i$ of the alternatives $(j, o, k)$ on machine $i$; each machine is no-idle or not
* the big-M constant $M = \sum_{j, o} \max_k p_{jok}$

**Variables**:

* $C_{jo} \in \mathbb{N}$: completion time of operation $(j, o)$; with blocking, the time at which it leaves its machine
* $y_{jok, j'o'k'} \in \{0, 1\}$, for each pair of alternatives of $A_i$: $y = 1$ iff $(j, o)$ is scheduled before $(j', o')$
* $x_{jok} \in \{0, 1\}$ (flexible): $x_{jok} = 1$ iff alternative $k$ is selected for $(j, o)$
* $P_{jo} \in \mathbb{N}$ (flexible or blocking): time during which $(j, o)$ occupies its machine. Otherwise, $P_{jo}$ denotes the constant $p_{jo}$ below.
* $C_{jok} \in \mathbb{N}$ (flexible): completion time of $(j, o)$ if alternative $k$ is selected, 0 otherwise
* $z_{j, oo'} \in \{0, 1\}$ (open shop): $z_{j, oo'} = 1$ iff operation $o$ of job $j$ is scheduled before operation $o'$
* $S_j \in \mathbb{N}$ and $P_j \in \mathbb{N}$ (open shop with no-wait or blocking): start of job $j$, and sum of the times during which its operations occupy their machines ($P_j$ is a constant without flexibility and blocking)
* $S^m_i \in \mathbb{N}$ (no-idle machine $i$): start of machine $i$
* $P^m_i \in \mathbb{N}$ (no-idle machine $i$, flexible): sum of the processing times of the operations on machine $i$
* $C_{\max} \in \mathbb{N}$ (makespan), $C_j \in \mathbb{N}$ (open shop with total flow time), $T_j \in \mathbb{N}$ (total tardiness)

The lower bound of $C_{jo}$ is the sum of the minimum processing times of operations $1, \ldots, o$ of job $j$ in the job shop, and the minimum processing time of $(j, o)$ in the open shop. The lower bound of $P_{jo}$ is the minimum processing time of $(j, o)$.

**Objective**: depending on the objective

$$\min C_{\max} \qquad \text{(makespan)}$$

$$\min \sum_j w_j \, C_{j \ell_j} \qquad \text{(total flow time, job shop)}$$

$$\min \sum_j w_j \, C_j \qquad \text{(total flow time, open shop)}$$

$$\min \sum_j w_j \, T_j \qquad \text{(total tardiness)}$$

**Constraints**:

* Alternatives (flexible): one alternative is selected for each operation; the time during which the operation occupies its machine is the processing time of the selected alternative (at least this time with blocking, the difference being the time during which the operation blocks the machine); the completion time of the operation is the one of its selected alternative

$$\forall j, o \qquad \sum_k x_{jok} = 1$$

$$\forall j, o \qquad P_{jo} = \sum_k p_{jok} \, x_{jok}$$

$$\forall j, o, k \qquad C_{jok} \le M \, x_{jok}$$

$$\forall j, o \qquad C_{jo} = \sum_k C_{jok} \qquad C_{jo} \ge P_{jo}$$

* Machine disjunctions: two operations whose selected alternatives are on the same machine don't overlap. The $x$ terms only appear in the flexible case. Without blocking, the constant $p_{jok}$ is used instead of $P_{jo}$, which is equal when the alternative is selected.

$$\forall i, \; \forall (j, o, k), (j', o', k') \in A_i \qquad C_{jo} \ge C_{j'o'} + P_{jo} - M \, y_{jok, j'o'k'} - M (2 - x_{jok} - x_{j'o'k'})$$

$$\forall i, \; \forall (j, o, k), (j', o', k') \in A_i \qquad C_{j'o'} \ge C_{jo} + P_{j'o'} - M (1 - y_{jok, j'o'k'}) - M (2 - x_{jok} - x_{j'o'k'})$$

* Job precedences (job shop): an operation starts after the end of the previous operation of its job. With no-wait, the equality replaces the inequality. With blocking too: since $P_{jo}$ includes the time during which the operation blocks its machine, the operation leaves its machine when the next one starts.

$$\forall j, \; \forall o < \ell_j \qquad C_{j,o+1} \ge C_{jo} + P_{j,o+1}$$

* Job disjunctions (open shop): two operations of the same job don't overlap

$$\forall j, \; \forall o' < o \qquad C_{jo} \ge C_{jo'} + P_{jo} - M \, z_{j, oo'}$$

$$\forall j, \; \forall o' < o \qquad C_{jo'} \ge C_{jo} + P_{jo'} - M (1 - z_{j, oo'})$$

* No-wait and blocking (open shop): all the operations of a job lie in a window whose length is the sum of their durations, so they are processed without interruption

$$\forall j \qquad P_j = \sum_o P_{jo}$$

$$\forall j, o \qquad S_j \le C_{jo} - P_{jo} \qquad C_{jo} - S_j \le P_j$$

* No-idle (without flexibility): all the operations of a no-idle machine lie in a window whose length is the sum of their processing times

$$\forall i \text{ no-idle}, \; \forall (j, o) \text{ on } i \qquad S^m_i \le C_{jo} - p_{jo} \qquad C_{jo} - S^m_i \le \sum_{(j', o') \text{ on } i} p_{j'o'}$$

* No-idle (flexible): the same, on the completion times of the alternatives, with a big-M term for the alternatives which are not selected, and with a variable sum of processing times. If alternative $k$ is not selected, $C_{jok} = 0$ and both constraints are satisfied.

$$\forall i \text{ no-idle} \qquad P^m_i = \sum_{(j, o, k) \in A_i} p_{jok} \, x_{jok}$$

$$\forall i \text{ no-idle}, \; \forall (j, o, k) \in A_i \qquad S^m_i \le C_{jok} - p_{jok} + M (1 - x_{jok}) \qquad S^m_i \ge C_{jok} - P^m_i$$

* Makespan, on the last operation of each job in the job shop, and on all the operations in the open shop

$$\forall j \qquad C_{\max} \ge C_{j \ell_j} \qquad \text{(job shop)}$$

$$\forall j, o \qquad C_{\max} \ge C_{jo} \qquad \text{(open shop)}$$

* Job completion times (open shop with total flow time)

$$\forall j, o \qquad C_j \ge C_{jo}$$

* Tardiness, on the last operation of each job in the job shop, and on all the operations in the open shop

$$\forall j \qquad T_j \ge C_{j \ell_j} - d_j \qquad \text{(job shop)}$$

$$\forall j, o \qquad T_j \ge C_{jo} - d_j \qquad \text{(open shop)}$$

The solution is rebuilt from the selected alternatives and the completion times. With blocking, an operation starts at $C_{jo} - P_{jo}$.

## Solver

HiGHS (default), CBC and Xpress, chosen with `--solver highs|cbc|xpress`, as for the [positional MILP](milp_positional.md).

The model can also be written to an MPS file without solving it, with the `write_mps` function or the `shopschedulingsolver_mps_writer` executable.

## Tests

* `test/algorithms/milp_disjunctive_test.cpp` with the instances of `test/algorithms/milp_disjunctive_test.txt`: job shop, flexible job shop, open shop and flexible open shop instances, with makespan, total (weighted) flow time and total (weighted) tardiness, each with no constraint, no-wait and blocking. The no-idle lists are not part of the test.
* `HandmadeAlgorithmTest`, in the same file: hand-made instances of `data/handmade/`, each with a reference optimal solution, for cases which the generated instances don't cover: a flexible job shop with no-idle machines.

## References

* B. Naderi and M. Zandieh (2014). Modeling and scheduling no-wait open shop problems. *International Journal of Production Economics*, 158, 256–266. [doi:10.1016/j.ijpe.2014.06.011](https://doi.org/10.1016/j.ijpe.2014.06.011)
* V. Roshanaei, A. Azab and H. ElMaraghy (2013). Mathematical modelling and a meta-heuristic for flexible job shop scheduling. *International Journal of Production Research*, 51(20), 6247–6274. [doi:10.1080/00207543.2013.827806](https://doi.org/10.1080/00207543.2013.827806)
