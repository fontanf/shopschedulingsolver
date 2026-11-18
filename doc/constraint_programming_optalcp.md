# Constraint programming (OptalCP)

`--algorithm constraint-programming-optalcp`

Source: [`src/algorithms/constraint_programming_optalcp.mts`](../src/algorithms/constraint_programming_optalcp.mts) (model) and [`src/algorithms/constraint_programming_optalcp.cpp`](../src/algorithms/constraint_programming_optalcp.cpp) (launcher)

Only built with `-DSHOPSCHEDULINGSOLVER_USE_OPTALCP=ON`. See the README for the requirements.

## Algorithm

Constraint programming model solved with OptalCP.

OptalCP has a JavaScript/TypeScript API, so the model is written in TypeScript and compiled to `constraint_programming_optalcp.mjs` when building. The C++ launcher runs it with Node.js and communicates with it through one JSON object per line:
* The launcher writes the instance and the parameters on the standard input of the script, then `stop` when the time limit is reached. The search also stops if the standard input is closed.
* The script writes `{"type": "solution", ...}` for each new solution, `{"type": "bound", ...}` for each new bound, and `{"type": "end"}` at the end.

The environment variable `SHOPSCHEDULINGSOLVER_OPTALCP_SCRIPT` sets the path of the script to use instead of the one of the build directory.

Model:
* One interval variable per operation, of length its processing time. In the flexible case, one optional interval variable per alternative, linked to the interval variable of the operation by an `alternative` constraint.
* For each machine, a sequence variable on the intervals of its alternatives, and a `noOverlap` constraint.
* Job shop: `endBeforeStart` between consecutive operations of a job; `endAtStart` with no-wait or blocking.
* Open shop: for each job, a sequence variable on its operations and a `noOverlap` constraint.
* Blocking: the length of an operation (or of each of its alternatives) is in $[p, H]$ instead of $p$, the extra length being the time during which it blocks the machine. In the job shop, the last operation of a job has a fixed length, and the other operations end when the next one starts (`endAtStart`). In the open shop, all operations have a variable length. In the flexible case, $H = \sum_{j, o} \max_k p_{jok}$ (processing the operations one after the other ends before $H$); otherwise, the length is not bounded.
* Open shop with no-wait or blocking: for each job, an interval variable whose start is the minimum start of its operations, whose end is the maximum end, and whose length is the sum of their lengths, so that the operations are processed without interruption.
* No-idle machine: for each such machine, an interval variable whose start is the minimum start of its operations, whose end is the maximum end, and whose length is the sum of their lengths.
* Permutation: one integer variable per job, equal to the position of the job in the sequence variable of each machine.

Objectives: $\min C_{\max}$, $\min \sum_j w_j C_j$ or $\min \sum_j w_j T_j$, with $C_j$ the maximum end of the operations of job $j$.

Search parameters: four workers (`FDS`, `FDS` with the `Split` lower bound strategy, and two `LNS`), propagation levels 4, 3 and 2 for no-overlap, cumulative and reservoir constraints in the FDS workers, `usePrecedenceEnergy = 1`, `packPropagationLevel = 2`, and a relative gap tolerance of 0 so that the search only stops when optimality is proved.

## Supported problems

* Permutation flow shop, job shop, flexible job shop, open shop and flexible open shop.
* Objectives $C_{\max}$, $\sum w_j C_j$ and $\sum w_j T_j$.
* Constraints no-wait, blocking and mixed no-idle.

Release dates are ignored. With total tardiness, every job must have a due date. The preview edition of OptalCP is not supported since it doesn't return solutions.

## Optimality

Exact: when OptalCP proves optimality before the time limit, the solution is optimal.

## Proved optimality

The lower bounds found during the search are reported. At the end, if OptalCP has proved optimality, the bound is set to the value of the solution and the output reports `"Optimal": true`.

## Tests

* `test/algorithms/constraint_programming_optalcp_test.cpp` with the instances of `test/algorithms/constraint_programming_optalcp_test.txt`: permutation flow shop (makespan, total flow time and total tardiness, each with no constraint, no-idle and blocking), and job shop, flexible job shop, open shop and flexible open shop (makespan, total (weighted) flow time and total (weighted) tardiness, each with no constraint, no-wait and blocking).
* The instances of the no-idle lists of `data-tests-v2` have no no-idle machine (`"no_idle": false` on every machine, because of a bug of the generator), so these lists don't test the no-idle constraints.
* `HandmadeAlgorithmTest`, in the same file: hand-made instances of `data/handmade/`, each with a reference optimal solution, for cases which the generated instances don't cover: a permutation flow shop with no-idle machines and more jobs than machines, and a flexible open shop with blocking in which the optimal schedule blocks a machine.

## References

* D. O. Fernández Pons and P. Vilím (2024). OptalCP, un nouveau moteur d'ordonnancement. *Journées Francophones de Programmation par Contraintes (JFPC 2024)*.
* OptalCP: [optalcp.com](https://optalcp.com)
