# Tree search, permutation flow shop, makespan

`--algorithm tree-search-pfss-makespan`

Source: [`src/algorithms/tree_search_pfss_makespan.cpp`](../src/algorithms/tree_search_pfss_makespan.cpp)

## Algorithm

Iterative beam search with bidirectional branching, from Libralesso et al. (2022).

**Search.** Iterative beam search (`iterative_beam_search_2` of [treesearchsolver](https://github.com/fontanf/treesearchsolver)) runs beam searches of increasing width: 1, 2, 4, ..., up to $10^8$ (growth factor 2). A node is pruned when its bound is greater than or equal to the makespan of the best solution found. If a beam search ends without discarding any node because of the beam width, the search tree has been fully explored and the best solution is optimal.

**Branching.** A node is a partial solution made of a prefix (jobs scheduled forward, from the start) and a suffix (jobs scheduled backward, from the end). Each child adds an unscheduled job either at the end of the prefix or at the start of the suffix, depending on the direction of the node. For each machine $i$, the node stores the completion time $f_i$ of the prefix on machine $i$, the time $b_i$ during which the suffix occupies machine $i$ (computed on the reversed instance), the remaining processing time $R_i$ of the unscheduled jobs on machine $i$, and the idle times.

The direction of a node is chosen when its children are generated. For both directions, the algorithm counts the children which would not be pruned and sums their bounds. It chooses the direction with fewer children; on ties, the direction with the larger sum of bounds; on ties again, the direction opposite to the one of the parent.

**Bound.** The bound of a node is $\max_i \left( f_i + R_i + b_i \right)$: each machine must process the prefix, then the remaining jobs, then the suffix. With blocking, $f_i$ and $b_i$ are departure times, computed with the blocking recurrence (a job leaves machine $i$ when machine $i + 1$ is free).

**Guide.** The nodes of a beam are sorted by the guide below (lowest first). With $\alpha = k / n$, $k$ the number of scheduled jobs:

$$\text{guide} = \alpha \cdot \text{bound} + (1 - \alpha) \cdot \text{WI} \cdot \text{bound}$$

where WI is the weighted idle time: the sum over machines of the idle time added by each job, divided by the current completion time on that machine, so that idle time at the start of the schedule weighs more. Other guides are implemented (bound only, idle time only, ...) but not exposed on the command line.

**Solution.** The prefix followed by the suffix is turned back into a schedule, each operation starting as early as possible (or as early as the blocking constraints allow).

## Supported problems

$F_m \mid \text{prmu} \mid C_{\max}$ and $F_m \mid \text{prmu}, \text{blocking} \mid C_{\max}$.

The algorithm doesn't check the type of the instance. It is not valid with no-wait or no-idle constraints, release dates, or other objectives.

## Optimality

Exact: if the algorithm terminates before the time limit, the solution is optimal. On large instances, it is used as a heuristic: each beam search gives a solution, and the solutions improve as the width increases.

## Proved optimality

When the search tree has been fully explored, the algorithm sets the makespan bound to the value of its solution, and the output reports `"Optimal": true`. No bound is reported otherwise.

## Tests

* `test/algorithms/tree_search_pfss_makespan_test.cpp` with the instances of `test/algorithms/tree_search_pfss_makespan_test.txt`: permutation flow shop instances with makespan, without constraint and with blocking.

## References

* L. Libralesso, P. A. Focke, A. Secardin and V. Jost (2022). Iterative beam search algorithms for the permutation flowshop. *European Journal of Operational Research*, 301(1), 217–234. [doi:10.1016/j.ejor.2021.10.015](https://doi.org/10.1016/j.ejor.2021.10.015)
