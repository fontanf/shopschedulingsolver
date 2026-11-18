# Local search, permutation flow shop, total flow time and total tardiness

`--algorithm local-search-pfss`

Source: [`src/algorithms/local_search_pfss.cpp`](../src/algorithms/local_search_pfss.cpp)

## Supported problems

$F_m \mid \text{prmu} \mid \gamma$ and $F_m \mid \text{prmu}, \text{blocking} \mid \gamma$ with $\gamma \in \{\sum C_j, \sum T_j\}$.

The algorithm throws `std::invalid_argument` with another objective or with no-wait or no-idle constraints. It ignores weights and release dates.

## Optimality

Heuristic: no optimality guarantee, and no bound is computed, so the output never reports `"Optimal": true`.

The algorithm stops at the time limit. It also has maximum numbers of iterations and of iterations without improvement, which are parameters of the C++ function (`LocalSearchPfssParameters`) and not of the command line, so **`--time-limit` must be set**. The random generator is seeded with `--seed` (0 by default).

## Algorithm

Population-based iterated local search, with the same structure as the [makespan local search](local_search_pfss_makespan.md).

**Population.** Same as for the makespan: between 20 and 40 solutions, biased fitness from the objective and the diversity contribution (3 closest neighbors, 10 elite solutions), distance between sequences counting the jobs whose predecessor or successor differs.

**Initial solution.** One solution built by the [tree search](tree_search_pfss.md) with a beam width of 1024 (a single beam search), then improved by the local search.

**Iterations.** At each iteration:
1. Select the best solution of the population during the first 63 iterations, then a solution chosen by binary tournament.
2. Apply a perturbation, chosen at random with weights $2 + s$, $s$ being the number of times the perturbation has improved the solution it was applied to:
   * Random shifts: remove 2 to 4 random jobs and reinsert each at its best position, other than its original one.
   * Ruin and recreate: remove 4 random jobs, apply the local search to the partial sequence, then reinsert each removed job at its best position.
3. Apply the local search, and add the solution to the population.

Insertions at the best position break ties at random.

**Local search.** Variable neighborhood descent on the neighborhoods below, shuffled before each pass; the pass stops at the first neighborhood which improves the solution, and a new pass starts until no neighborhood improves it:
* Shift: move a block of 1, 2 or 3 consecutive jobs to another position. For each position, the best new position is found and applied if it improves the objective.
* Swap: exchange two jobs.

**Accelerations.** The completion times of each prefix of the current sequence and the objective of each prefix are stored, so a move is evaluated from the first modified position only. The evaluation of a move stops as soon as its partial objective reaches the best objective found. When the partial objective plus the objective of the unchanged suffix in the current solution reaches the best objective, and the completion times on all machines before the suffix are not earlier than in the current solution, the suffix can't complete earlier, so the move is discarded. The blocking case is selected at compile time, so the inner loops have no branch on it.

## Tests

None.

## References

* E. Teixeira, A. Subramanian and H. H. Kramer (2025). A hybrid population-based ruin-and-recreate algorithm for the blocking flow shop scheduling problem. *European Journal of Operational Research*. [doi:10.1016/j.ejor.2025.08.002](https://doi.org/10.1016/j.ejor.2025.08.002)
* V. Fernandez-Viagas, J. M. S. Valente and J. M. Framinan (2018). Iterated-greedy-based algorithms with beam search initialization for the permutation flowshop to minimise total tardiness. *Expert Systems with Applications*, 94, 58–69. [doi:10.1016/j.eswa.2017.10.050](https://doi.org/10.1016/j.eswa.2017.10.050)
