# CP-SAT (OR-Tools)

`--algorithm cp-sat-ortools`

Source: [`src/algorithms/cp_sat_ortools.cpp`](../src/algorithms/cp_sat_ortools.cpp)

Only built with `-DSHOPSCHEDULINGSOLVER_USE_ORTOOLS=ON`. See the README for the requirements.

## Algorithm

Constraint programming model solved with the CP-SAT solver of OR-Tools.

Model:
* For each operation $(j, o)$, integer variables $S_{jo}$ and $C_{jo}$ in $[r_j, H]$ and an interval variable of length $p_{jo}$ between them.
* For each machine, a `NoOverlap` constraint on the intervals of its operations.
* For consecutive operations of a job, $S_{j,o+1} \ge C_{jo}$, or $S_{j,o+1} = C_{jo}$ with no-wait.

The horizon $H = \max_j r_j + \sum_{j, o} p_{jo}$ is the end of the schedule which processes the operations one after the other from $\max_j r_j$, so the model always has a solution.

Objectives, with $C_j$ the completion time of the last operation of job $j$:
* Makespan: $\min C_{\max}$, with $C_{\max} = \max_j C_j$.
* Total flow time: $\min \sum_j w_j (C_j - r_j)$.
* Total tardiness: $\min \sum_j w_j T_j$, with $T_j \ge C_j - d_j$, $T_j \ge 0$. Jobs without a due date are not counted.

CP-SAT runs with its default parameters, apart from the time limit. Each new solution and bound is reported through the callbacks of the solver, and the search is stopped through them when the timer ends. The log of the solver is written with `--solver-log`.

## Supported problems

Job shop $J_m \mid r_j \mid \gamma$ and $J_m \mid r_j, \text{no-wait} \mid \gamma$ with $\gamma \in \{C_{\max}, \sum w_j C_j, \sum w_j T_j\}$. A flow shop is solved as a job shop.

The algorithm throws `std::invalid_argument` on the throughput objective, and on flexible, open shop, blocking, no-idle, mixed no-idle and permutation instances.

This is the only algorithm of the repository which takes release dates into account.

## Optimality

Exact: when CP-SAT terminates before the time limit, the solution is optimal.

## Proved optimality

The best bound of CP-SAT is reported through the callbacks and at the end. The output reports `"Optimal": true` when it is equal to the value of the solution.

## Tests

* `test/algorithms/cp_sat_ortools_test.cpp` with the instances of `test/algorithms/cp_sat_ortools_test.txt`: job shop instances with makespan, total (weighted) flow time and total (weighted) tardiness, each with no constraint and no-wait. The test instances have no release dates.

## References

* OR-Tools CP-SAT solver: [developers.google.com/optimization/cp/cp_solver](https://developers.google.com/optimization/cp/cp_solver)
