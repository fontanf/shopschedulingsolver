# Positional MILP

`--algorithm milp-positional`

Source: [`src/algorithms/milp_positional.cpp`](../src/algorithms/milp_positional.cpp)

## Algorithm

Mixed-integer linear programming model for the permutation flow shop, based on the positional model of Ta, Billaut and Bouquard (2018). The model of the paper minimizes the total tardiness. It is adapted here to the makespan and the total flow time, and to the no-idle, no-wait and blocking constraints.

Let $n$ be the number of jobs, $m$ the number of machines, and $k \in \{1, \dots, n\}$ the positions in the sequence. Variables:
* $x_{jk} \in \{0, 1\}$: $x_{jk} = 1$ iff job $j$ is in position $k$.
* $P_{ik} \ge 0$: processing time on machine $i$ of the job in position $k$.
* $C_{ik} \ge 0$: completion time on machine $i$ of the job in position $k$.
* $d_k \ge 0$, $T_k \ge 0$: due date and tardiness of the job in position $k$ (total tardiness only).
* $C_{\max}$ (makespan only).

Constraints:

$$\sum_k x_{jk} = 1 \quad \forall j \qquad \sum_j x_{jk} = 1 \quad \forall k$$

$$P_{ik} = \sum_j p_{ij} \, x_{jk} \qquad d_k = \sum_j d_j \, x_{jk}$$

$$C_{1,1} = P_{1,1} \qquad C_{ik} \ge C_{i-1,k} + P_{ik} \qquad C_{ik} \ge C_{i,k-1} + P_{ik}$$

$$C_{\max} \ge C_{ik} \qquad T_k \ge C_{mk} - d_k$$

Objectives:
* Makespan: $\min C_{\max}$.
* Total flow time: $\min \sum_k C_{mk}$.
* Total tardiness: $\min \sum_k T_k$.

Variants:
* No-idle machine $i$: $C_{ik} = C_{i,k-1} + P_{ik}$. The property can be set per machine (mixed no-idle).
* No-wait: $C_{ik} = C_{i-1,k} + P_{ik}$.
* Blocking: $P_{ik} \ge \sum_j p_{ij} x_{jk}$ instead of an equality, so that $P_{ik}$ is the time during which the job occupies machine $i$, including the time it blocks it, and $C_{ik} = C_{i-1,k} + P_{ik}$ is the time at which it leaves machine $i$. The job starts on machine $i$ when it leaves machine $i - 1$.

The solution is rebuilt from the values of $x$, $C$ and $P$: the job of position $k$ starts on machine $i$ at $C_{ik} - P_{ik}$.

Solvers: HiGHS (default), CBC and Xpress, chosen with `--solver highs|cbc|xpress`. The solver must be enabled when building (see the README). New solutions and bounds are retrieved through the callbacks of the solver, and the search is stopped through them when the time limit is reached. The log of the solver is written with `--solver-log`.

## Supported problems

Permutation flow shop: $F_m \mid \text{prmu} \mid \gamma$ with $\gamma \in \{C_{\max}, \sum C_j, \sum T_j\}$, with the mixed no-idle, no-wait or blocking constraints.

The algorithm doesn't check the type of the instance: it reads operation $i$ of each job as its operation on machine $i$. Weights and release dates are ignored, so the model minimizes $\sum C_j$ and $\sum T_j$ and not their weighted versions. With total tardiness, every job must have a due date.

## Optimality

Exact: when the solver terminates before the time limit, the solution is optimal and the bound is equal to its value.

## Proved optimality

The dual bound of the solver is reported through the callbacks and at the end, rounded up to the next integer. The output reports `"Optimal": true` when it is equal to the value of the solution.

## Tests

* `test/algorithms/milp_positional_test.cpp` with the instances of `test/algorithms/milp_positional_test.txt`: permutation flow shop instances with makespan, total flow time and total tardiness, each with no constraint, no-idle and blocking. The instances of the no-idle lists have no no-idle machine (`"no_idle": false` on every machine), so the no-idle constraints are not tested. The no-wait constraint is not tested either.

## References

* Q. C. Ta, J.-C. Billaut and J.-L. Bouquard (2018). Matheuristic algorithms for minimizing total tardiness in the m-machine flow-shop scheduling problem. *Journal of Intelligent Manufacturing*, 29(3), 617–628. [doi:10.1007/s10845-015-1046-4](https://doi.org/10.1007/s10845-015-1046-4)
