# Tree search, permutation flow shop, total flow time and total tardiness

`--algorithm tree-search-pfss`

Source: [`src/algorithms/tree_search_pfss.cpp`](../src/algorithms/tree_search_pfss.cpp)

## Algorithm

Iterative beam search with forward branching.

**Search.** As in the [makespan tree search](tree_search_pfss_makespan.md): beam searches of width 1, 2, 4, ..., up to $10^8$ (growth factor 2). The minimum and maximum widths are parameters of the C++ function (`TreeSearchPfssParameters`), not of the command line. A node is pruned when its bound is greater than or equal to the value of the best solution found. If a beam search ends without discarding any node because of the beam width, the best solution is optimal.

**Branching.** A node is a prefix of the sequence. Each child appends an unscheduled job. The node stores, for each machine, the completion time of the prefix on it (or the departure time with blocking) and its idle time.

**Bound.** Let $k$ be the number of jobs of the prefix, $C$ its completion time on the last machine, and $U$ the set of unscheduled jobs.
* Total flow time: $\sum_{\text{scheduled}} C_j + \sum_{j \in U} (C + p_{mj})$. Each unscheduled job completes after the prefix and after its own operation on the last machine.
* Total tardiness: $\sum_{\text{scheduled}} T_j + \sum_{j \in U, d_j < C} (C - d_j)$. Each unscheduled job completes after $C$. The jobs of $U$ with $d_j < C$ are maintained incrementally from the jobs sorted by due date.

**Guide.** With $\alpha = k / n$:
* Total flow time (guide 2): $\alpha \cdot \sum_{\text{scheduled}} C_j + (1 - \alpha) \cdot I \cdot k / m$, with $I$ the idle time of the prefix.
* Total tardiness (guide 4, Fernandez-Viagas, Valente and Framinan, 2018): $w_{TI} \cdot \text{TI} + w_{TE} \cdot \text{TE} + w_{TT} \cdot \text{TT}$, with TI the weighted idle time, TE the total earliness and TT the bound above, and the weights $w_{TT} = \frac{k + n - 1}{2n}$, $w_{TE} = \frac{2n - k - 1}{2n}$, $w_{TI} = \frac{n - k - 1}{n}$.

Other guides are implemented but not exposed on the command line. A dominance rule (same set of scheduled jobs, smaller objective and completion times) is implemented but disabled.

## Supported problems

$F_m \mid \text{prmu} \mid \gamma$ and $F_m \mid \text{prmu}, \text{blocking} \mid \gamma$ with $\gamma \in \{\sum C_j, \sum T_j\}$.

The algorithm doesn't check the type of the instance. It ignores weights and release dates, and is not valid with no-wait or no-idle constraints. With total tardiness, jobs without a due date are never tardy.

## Optimality

Exact: if the algorithm terminates before the time limit, the solution is optimal. On large instances, it is used as a heuristic, for example to build the initial solution of the [local search](local_search_pfss.md).

## Proved optimality

When the search tree has been fully explored, the algorithm sets the bound to the value of its solution, and the output reports `"Optimal": true`. No bound is reported otherwise.

## Tests

* `test/algorithms/tree_search_pfss_test.cpp` with the instances of `test/algorithms/tree_search_pfss_test.txt`: permutation flow shop instances with total flow time and total tardiness, without constraint and with blocking.

## References

* L. Libralesso, P. A. Focke, A. Secardin and V. Jost (2022). Iterative beam search algorithms for the permutation flowshop. *European Journal of Operational Research*, 301(1), 217–234. [doi:10.1016/j.ejor.2021.10.015](https://doi.org/10.1016/j.ejor.2021.10.015)
* V. Fernandez-Viagas, J. M. S. Valente and J. M. Framinan (2018). Iterated-greedy-based algorithms with beam search initialization for the permutation flowshop to minimise total tardiness. *Expert Systems with Applications*, 94, 58–69. [doi:10.1016/j.eswa.2017.10.050](https://doi.org/10.1016/j.eswa.2017.10.050)
