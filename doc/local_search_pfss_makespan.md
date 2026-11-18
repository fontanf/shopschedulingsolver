# Local search, permutation flow shop, makespan

`--algorithm local-search-pfss-makespan`

Source: [`src/algorithms/local_search_pfss_makespan.cpp`](../src/algorithms/local_search_pfss_makespan.cpp)

## Algorithm

Population-based iterated local search, based on the hybrid population-based ruin-and-recreate algorithm of Teixeira, Subramanian and Kramer (2025) for the blocking flow shop.

**Population.** The population (`Population` of [localsearchsolver](https://github.com/fontanf/localsearchsolver)) keeps between 20 and 40 solutions. When it exceeds 40 solutions, it removes the solutions with the worst biased fitness until 20 remain. The biased fitness combines the rank of the solution by makespan and its rank by diversity contribution (mean distance to its 3 closest solutions), with 10 elite solutions. The distance between two sequences is the number of jobs whose predecessor or successor differs.

**Initial solutions.** 20 solutions built with PF-NEH (Pan and Wang, 2012), each improved by the local search:
1. Start with a random job.
2. Profile fitting: repeatedly append the unscheduled job which minimizes the sum of the departure times on all machines.
3. Remove the last $a$ jobs, with $a = 25$ if $n > 20$ and $a = 21$ otherwise (the value of the code of Teixeira et al.; the paper states 25 and 20), and reinsert them in a random order, each at the position which minimizes the makespan.

**Iterations.** At each iteration:
1. Select a solution of the population by binary tournament.
2. Apply a perturbation, chosen at random with weights $2 + s$, $s$ being the number of times the perturbation has improved the solution it was applied to:
   * Random shifts: remove 2 to 4 random jobs and reinsert each at its best position, other than its original one.
   * Ruin and recreate: remove 4 random jobs, apply the local search to the partial sequence, then reinsert each removed job at its best position.
3. Apply the local search, and add the solution to the population.

Insertions at the best position break ties at random.

**Local search.** Variable neighborhood descent: the neighborhoods are explored in order, and after an improvement, the descent restarts from the first one. The neighborhoods are:
* Shift: move a job to another position. For each position, the best new position is found and applied if it improves the makespan.
* Block shift: same with blocks of 2, 3 and 4 consecutive jobs.
* Swap: exchange two jobs.

When $n \ge 256$, only the shift neighborhood is used.

**Accelerations.**
* Makespans of the shift moves are evaluated in $O(m)$ each with Taillard's heads and tails (Taillard, 1990), adapted to blocking with departure times.
* The critical path of the current solution is stored, and a lower bound of the makespan after a move is computed from it in $O(1)$: the makespan of the current solution, minus the contribution of the moved job (or of the job before it when it is removed from an anti-block), plus its processing time on the machine where the critical path crosses the insertion position (Ding et al., 2016). The makespan of a move is only computed if this bound is lower than the best makespan found.
* With blocking, inserting a job in the block it belongs to can't improve the makespan (Grabowski and Pempera, 2007), so these positions are skipped.
* For swaps, the move is skipped unless the contributions of the two jobs to the critical path decrease.

## Supported problems

$F_m \mid \text{prmu} \mid C_{\max}$ and $F_m \mid \text{prmu}, \text{blocking} \mid C_{\max}$.

The algorithm throws `std::invalid_argument` with another objective. It doesn't check the other properties of the instance and is not valid with no-wait or no-idle constraints.

## Optimality

Heuristic: no optimality guarantee, and no bound is computed.

The algorithm stops at the time limit. It also has maximum numbers of iterations and of iterations without improvement, which are parameters of the C++ function (`LocalSearchParameters`) and not of the command line, so **`--time-limit` must be set**. The random generator is seeded with `--seed` (0 by default).

## Proved optimality

None.

## Tests

None.

## References

* E. Teixeira, A. Subramanian and H. H. Kramer (2025). A hybrid population-based ruin-and-recreate algorithm for the blocking flow shop scheduling problem. *European Journal of Operational Research*. [doi:10.1016/j.ejor.2025.08.002](https://doi.org/10.1016/j.ejor.2025.08.002)
* Q.-K. Pan and L. Wang (2012). Effective heuristics for the blocking flowshop scheduling problem with makespan minimization. *Omega*, 40(2), 218–229. [doi:10.1016/j.omega.2011.06.002](https://doi.org/10.1016/j.omega.2011.06.002)
* M. Nawaz, E. E. Enscore and I. Ham (1983). A heuristic algorithm for the m-machine, n-job flow-shop sequencing problem. *Omega*, 11(1), 91–95. [doi:10.1016/0305-0483(83)90088-9](https://doi.org/10.1016/0305-0483(83)90088-9)
* E. Taillard (1990). Some efficient heuristic methods for the flow shop sequencing problem. *European Journal of Operational Research*, 47(1), 65–74. [doi:10.1016/0377-2217(90)90090-X](https://doi.org/10.1016/0377-2217(90)90090-X)
* J. Grabowski and J. Pempera (2007). The permutation flow shop problem with blocking. A tabu search approach. *Omega*, 35(3), 302–311. [doi:10.1016/j.omega.2005.07.004](https://doi.org/10.1016/j.omega.2005.07.004)
* J.-Y. Ding, S. Song, J. N. D. Gupta, C. Wang, R. Zhang and C. Wu (2016). New block properties for flowshop scheduling with blocking and their application in an iterated greedy algorithm. *International Journal of Production Research*, 54(16), 4759–4772. [doi:10.1080/00207543.2015.1076941](https://doi.org/10.1080/00207543.2015.1076941)
