# Positional MILP

`--algorithm milp-positional`

Source: [`src/algorithms/milp_positional.cpp`](../src/algorithms/milp_positional.cpp)

## Supported problems

Permutation flow shop: $F_m \mid \text{prmu} \mid \gamma$ with $\gamma \in \{C_{\max}, \sum C_j, \sum T_j\}$, with the mixed no-idle, no-wait or blocking constraints.

The algorithm doesn't check the type of the instance: it reads operation $i$ of each job as its operation on machine $i$. Weights and release dates are ignored, so the model minimizes $\sum C_j$ and $\sum T_j$ and not their weighted versions. With total tardiness, every job must have a due date.

## Optimality

Exact. The dual bound of the solver is reported through the callbacks and at the end, rounded up to the next integer. When the solver terminates before the time limit, the solution is optimal, the bound is equal to its value, and the output reports `"Optimal": true`.

## Algorithm

Mixed-integer linear programming model for the permutation flow shop, based on the positional model of Ta, Billaut and Bouquard (2018). The model of the paper minimizes the total tardiness. It is adapted here to the makespan and the total flow time, and to the no-idle, no-wait and blocking constraints.

The sequence of the jobs is modeled by assigning each job to a position. The completion times are then defined per position and per machine, so that the constraints between consecutive jobs are linear.

## Model

**Input**:

* jobs $j = 1, \ldots, n$; for each job $j$: a processing time $p_{ij}$ on each machine $i$ and, for the total tardiness, a due date $d_j$
* machines $i = 1, \ldots, m$, visited in this order by all the jobs; each machine is no-idle or not
* positions $k = 1, \ldots, n$ in the sequence

**Variables**:

* $x_{jk} \in \{0, 1\}$: $x_{jk} = 1$ iff job $j$ is in position $k$
* $P_{ik} \in \mathbb{N}$: time during which the job in position $k$ occupies machine $i$
* $C_{ik} \in \mathbb{N}$: completion time on machine $i$ of the job in position $k$; with blocking, the time at which it leaves machine $i$
* $d_k \in \mathbb{N}$ and $T_k \in \mathbb{N}$ (total tardiness): due date and tardiness of the job in position $k$
* $C_{\max} \in \mathbb{N}$ (makespan)

**Objective**: depending on the objective

$$\min C_{\max} \qquad \text{(makespan)}$$

$$\min \sum_k C_{mk} \qquad \text{(total flow time)}$$

$$\min \sum_k T_k \qquad \text{(total tardiness)}$$

**Constraints**:

* Assignment: each job is in one position, and each position contains one job

$$\forall j \qquad \sum_k x_{jk} = 1$$

$$\forall k \qquad \sum_j x_{jk} = 1$$

* Processing times of the positions. With blocking, the inequality $\ge$ replaces the equality: the difference is the time during which the job blocks the machine.

$$\forall i, k \qquad P_{ik} = \sum_j p_{ij} \, x_{jk}$$

* Due dates of the positions (total tardiness)

$$\forall k \qquad d_k = \sum_j d_j \, x_{jk}$$

* Job: the job of a position starts on a machine after it completes on the previous machine. With no-wait, the equality replaces the inequality: the job starts on a machine when it completes on the previous one. With blocking too: it starts on a machine when it leaves the previous one.

$$C_{1,1} = P_{1,1}$$

$$\forall k, \; \forall i \ge 2 \qquad C_{ik} \ge C_{i-1,k} + P_{ik}$$

* Machine: the job of a position starts on a machine after the job of the previous position. On a no-idle machine, the equality replaces the inequality. The property can be set per machine (mixed no-idle).

$$\forall i, \; \forall k \ge 2 \qquad C_{ik} \ge C_{i,k-1} + P_{ik}$$

* Makespan

$$\forall i \qquad C_{\max} \ge C_{in}$$

* Tardiness

$$\forall k \qquad T_k \ge C_{mk} - d_k$$

The solution is rebuilt from the values of $x$, $C$ and $P$: the job of position $k$ starts on machine $i$ at $C_{ik} - P_{ik}$.

## Solver

HiGHS (default), CBC and Xpress, chosen with `--solver highs|cbc|xpress`. The solver must be enabled when building (see the README). New solutions and bounds are retrieved through the callbacks of the solver, and the search is stopped through them when the time limit is reached. The log of the solver is written with `--solver-log`.

## Tests

* `test/algorithms/milp_positional_test.cpp` with the instances of `test/algorithms/milp_positional_test.txt`: permutation flow shop instances with makespan, total flow time and total tardiness, each with no constraint, no-idle and blocking. The instances of the no-idle lists of `data-tests-v2` have no no-idle machine (`"no_idle": false` on every machine, because of a bug of the generator), so the no-idle constraints are not tested. The no-wait constraint is not tested either.

## References

* Q. C. Ta, J.-C. Billaut and J.-L. Bouquard (2018). Matheuristic algorithms for minimizing total tardiness in the m-machine flow-shop scheduling problem. *Journal of Intelligent Manufacturing*, 29(3), 617–628. [doi:10.1007/s10845-015-1046-4](https://doi.org/10.1007/s10845-015-1046-4)
