#include "shopschedulingsolver/algorithms/local_search_pfss.hpp"

#include "shopschedulingsolver/algorithm_formatter.hpp"
#include "shopschedulingsolver/solution_builder.hpp"
#include "shopschedulingsolver/algorithms/tree_search_pfss.hpp"

#include "localsearchsolver/population.hpp"

#include "optimizationtools/utils/common.hpp"

using namespace shopschedulingsolver;

namespace
{

struct LocalSearchSolution
{
    std::vector<JobId> jobs;
    std::vector<JobId> jobs_positions;
    Time objective = 0;
};

Solution build_solution(
        const Instance& instance,
        LocalSearchSolution& ls_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    solution_builder.from_permutation(ls_solution.jobs);
    Solution solution = solution_builder.build();
    if (solution.objective_value() != ls_solution.objective) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": wrong objective; "
                "solution: " + std::to_string(solution.objective_value()) + "; "
                "ls_solution: " + std::to_string(ls_solution.objective) + ".");
    }
    return solution;
}

struct LocalSearchData
{
    LocalSearchSolution solution;

    // completion_times[pos][machine_id]: completion time of the pos-th job on
    // machine machine_id in the current solution. Index 0 is the all-zero
    // initial state. Never modified inside local_search().
    std::vector<std::vector<Time>> completion_times;

    // prefix_objective[pos]: cumulative TFT/TT of the first pos jobs in the
    // current solution. Recomputed by update_prefix_objective() after every
    // mutation.
    std::vector<Time> prefix_objective;

    // Scratch CT vector used during move evaluation (block + suffix pass).
    std::vector<Time> completion_times_tmp_1;

    // Scratch CT vector used in Loop 2 of local_search() as a rolling
    // skip-sequence CT (updated in-place each iteration).
    std::vector<Time> completion_times_tmp_2;

    // Accumulates all moves that share the best objective found in the current
    // neighborhood exploration pass; one is picked at random at the end.
    std::vector<std::pair<JobId, JobId>> best_moves;
};

// Return the objective contribution of one job given its completion time.
inline Time job_contribution(
        const Instance& instance,
        JobId job_id,
        Time completion_time)
{
    if (instance.objective() == Objective::TotalFlowTime) {
        return completion_time;
    } else {
        return std::max(Time(0), completion_time - instance.job(job_id).due_date);
    }
}

// Advance a 1-D completion-time vector by one job in-place.
//
// Standard:  ct[m] = max(ct[m], ct[m-1]) + p[m]
// Blocking:  ct[0]   = max(ct[0]+p[0], ct[1])
//            ct[m]   = max(ct[m-1]+p[m], ct[m+1])  for 0 < m < M-1
//            ct[M-1] = ct[M-2]+p[M-1]
//
// In-place is safe for both cases:
//   Standard: ct[m] (old) read before being overwritten; ct[m-1] (new) used.
//   Blocking: ct[m+1] (old) read before being overwritten by the m+1 step.
//
// Blocking is a compile-time constant: no runtime branch in the inner loops.
template <bool Blocking>
inline void advance_ct(
        const Instance& instance,
        std::vector<Time>& ct,
        const Job& job)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    if (Blocking) {
        Time p0 = job.operations[0].alternatives[0].processing_time;
        if (last_machine_id > 0) {
            ct[0] = std::max(ct[0] + p0, ct[1]);
        } else {
            ct[0] += p0;
        }
        for (MachineId machine_id = 1; machine_id < last_machine_id; ++machine_id) {
            Time p = job.operations[machine_id].alternatives[0].processing_time;
            ct[machine_id] = std::max(ct[machine_id - 1] + p, ct[machine_id + 1]);
        }
        if (last_machine_id > 0) {
            Time p = job.operations[last_machine_id].alternatives[0].processing_time;
            ct[last_machine_id] = ct[last_machine_id - 1] + p;
        }
    } else {
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            Time proc = job.operations[machine_id].alternatives[0].processing_time;
            Time prev_machine = (machine_id > 0)? ct[machine_id - 1]: 0;
            ct[machine_id] = std::max(ct[machine_id], prev_machine) + proc;
        }
    }
}

// Recompute completion_times[p+1..n] using the current solution jobs.
// completion_times[p] must already hold the correct starting state.
template <bool Blocking>
void update_completion_times(
        const Instance& instance,
        LocalSearchData& data,
        JobId p)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    if (Blocking) {
        for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
            const Job& job = instance.job(data.solution.jobs[pos - 1]);
            Time p0 = job.operations[0].alternatives[0].processing_time;
            if (last_machine_id > 0) {
                data.completion_times[pos][0] = std::max(
                        data.completion_times[pos - 1][0] + p0,
                        data.completion_times[pos - 1][1]);
            } else {
                data.completion_times[pos][0] = data.completion_times[pos - 1][0] + p0;
            }
            for (MachineId machine_id = 1; machine_id < last_machine_id; ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                data.completion_times[pos][machine_id] = std::max(
                        data.completion_times[pos][machine_id - 1] + p,
                        data.completion_times[pos - 1][machine_id + 1]);
            }
            if (last_machine_id > 0) {
                Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                data.completion_times[pos][last_machine_id] = data.completion_times[pos][last_machine_id - 1] + p;
            }
        }
    } else {
        for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
            const Job& job = instance.job(data.solution.jobs[pos - 1]);
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                Time proc = job.operations[machine_id].alternatives[0].processing_time;
                Time prev_machine = (machine_id > 0)? data.completion_times[pos][machine_id - 1]: 0;
                data.completion_times[pos][machine_id] =
                    std::max(data.completion_times[pos - 1][machine_id], prev_machine) + proc;
            }
        }
    }
}

// Recompute prefix_objective[1..n] from completion_times (current solution).
void update_prefix_objective(
        const Instance& instance,
        LocalSearchData& data)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    data.prefix_objective[0] = 0;
    for (JobId pos = 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
        data.prefix_objective[pos] =
            data.prefix_objective[pos - 1]
            + job_contribution(
                    instance,
                    data.solution.jobs[pos - 1],
                    data.completion_times[pos][last_machine_id]);
    }
    data.solution.objective = data.prefix_objective[data.solution.jobs.size()];
}

template <bool Blocking>
void load_solution(
        LocalSearchData& data,
        const Instance& instance,
        const Solution& solution)
{
    data.solution.jobs.clear();
    std::fill(data.solution.jobs_positions.begin(), data.solution.jobs_positions.end(), -1);
    for (SolutionOperationId id = 0; id < solution.number_of_operations(); ++id) {
        const Solution::Operation& op = solution.operation(id);
        if (op.machine_id != 0)
            continue;
        data.solution.jobs.push_back(op.job_id);
        data.solution.jobs_positions[op.job_id] = op.machine_position;
    }
    update_completion_times<Blocking>(instance, data, 0);
    update_prefix_objective(instance, data);
}

// ---- Sequence-mutation helpers ----

template <bool Blocking>
void add_job(
        const Instance& instance,
        LocalSearchData& data,
        JobId job_id,
        JobId pos_new)
{
    data.solution.jobs.insert(data.solution.jobs.begin() + pos_new, job_id);
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos)
        data.solution.jobs_positions[data.solution.jobs[pos]] = pos;
    update_completion_times<Blocking>(instance, data, pos_new);
    update_prefix_objective(instance, data);
}

template <bool Blocking>
void add_block(
        const Instance& instance,
        LocalSearchData& data,
        const std::vector<JobId>& job_ids,
        JobId pos_new)
{
    data.solution.jobs.insert(
            data.solution.jobs.begin() + pos_new,
            job_ids.begin(),
            job_ids.end());
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos)
        data.solution.jobs_positions[data.solution.jobs[pos]] = pos;
    update_completion_times<Blocking>(instance, data, pos_new);
    update_prefix_objective(instance, data);
}

template <bool Blocking>
void remove_job(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos)
{
    data.solution.jobs.erase(data.solution.jobs.begin() + pos);
    for (JobId k = pos; k < (JobId)data.solution.jobs.size(); ++k)
        data.solution.jobs_positions[data.solution.jobs[k]] = k;
    update_completion_times<Blocking>(instance, data, pos);
    update_prefix_objective(instance, data);
}

template <bool Blocking>
void remove_block(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos,
        JobId size)
{
    data.solution.jobs.erase(
            data.solution.jobs.begin() + pos,
            data.solution.jobs.begin() + pos + size);
    for (JobId k = pos; k < (JobId)data.solution.jobs.size(); ++k)
        data.solution.jobs_positions[data.solution.jobs[k]] = k;
    update_completion_times<Blocking>(instance, data, pos);
    update_prefix_objective(instance, data);
}

template <bool Blocking>
void shift_jobs(
        const Instance& instance,
        LocalSearchData& data,
        JobId size,
        JobId pos_old,
        JobId pos_new)
{
    if (pos_new > pos_old) {
        std::rotate(
                data.solution.jobs.begin() + pos_old,
                data.solution.jobs.begin() + pos_old + size,
                data.solution.jobs.begin() + pos_new + size);
        for (JobId k = pos_old; k < pos_new + size; ++k)
            data.solution.jobs_positions[data.solution.jobs[k]] = k;
    } else {
        std::rotate(
                data.solution.jobs.begin() + pos_new,
                data.solution.jobs.begin() + pos_old,
                data.solution.jobs.begin() + pos_old + size);
        for (JobId k = pos_new; k < pos_old + size; ++k)
            data.solution.jobs_positions[data.solution.jobs[k]] = k;
    }
    update_completion_times<Blocking>(instance, data, 0);
    update_prefix_objective(instance, data);
}

template <bool Blocking>
void swap_jobs(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos_1,
        JobId size_1,
        JobId pos_2,
        JobId size_2)
{
    if (pos_2 < pos_1) {
        swap_jobs<Blocking>(instance, data, pos_2, size_2, pos_1, size_1);
        return;
    }
    // Transforms [B1 M B2] → [B2 M B1] using two std::rotate calls.
    // Requires pos_1 + size_1 <= pos_2 (non-overlapping, B1 before B2).
    //
    // Step 1: rotate at B1's end → [M B2 B1]
    // Step 2: rotate the [M B2] prefix at M's end → [B2 M]
    // Combined result: [B2 M B1]
    JobId gap = pos_2 - pos_1 - size_1;
    std::rotate(
            data.solution.jobs.begin() + pos_1,
            data.solution.jobs.begin() + pos_1 + size_1,
            data.solution.jobs.begin() + pos_2 + size_2);
    std::rotate(
            data.solution.jobs.begin() + pos_1,
            data.solution.jobs.begin() + pos_1 + gap,
            data.solution.jobs.begin() + pos_1 + gap + size_2);
    for (JobId pos = pos_1; pos < pos_2 + size_2; ++pos)
        data.solution.jobs_positions[data.solution.jobs[pos]] = pos;
    // completion_times[pos_1] = CT after the first pos_1 jobs (unchanged).
    update_completion_times<Blocking>(instance, data, pos_1);
    update_prefix_objective(instance, data);
}

// ---- Main local search (VND with block-insertion) ----

enum class Neighborhood
{
    Shift,
    Swap,
};

template <bool Blocking>
bool explore_shift_neighborhood(
        const Instance& instance,
        LocalSearchData& data,
        std::mt19937_64& generator,
        JobId size)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId n = (JobId)data.solution.jobs.size();

    bool improved = false;

    for (JobId pos_old = 0; pos_old + size <= n; ) {
        JobId pos_new_best = -1;
        Time objective_best = data.solution.objective;

        // Loop 1: block moves left (pos_new < pos_old).
        Time suffix_lb_left = data.prefix_objective[n] - data.prefix_objective[pos_old + size];
        for (JobId pos_new = 0; pos_new < pos_old; ++pos_new) {
            if (data.prefix_objective[pos_new] >= objective_best)
                break;

            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
                data.completion_times_tmp_1[machine_id] = data.completion_times[pos_new][machine_id];

            Time objective = data.prefix_objective[pos_new];

            // Block jobs.
            for (JobId b = pos_old; b < pos_old + size; ++b) {
                JobId job_id = data.solution.jobs[b];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= objective_best)
                    goto next_pos_new_left;
            }

            // Displaced: original jobs[pos_new..pos_old-1].
            for (JobId pos = pos_new; pos < pos_old; ++pos) {
                JobId job_id = data.solution.jobs[pos];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= objective_best)
                    goto next_pos_new_left;
            }

            // CT-dominance pruning.
            if (objective + suffix_lb_left >= objective_best) {
                bool dominated = true;
                for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                    if (data.completion_times_tmp_1[machine_id] < data.completion_times[pos_old + size][machine_id]) {
                        dominated = false;
                        break;
                    }
                }
                if (dominated)
                    goto next_pos_new_left;
            }

            // Suffix.
            for (JobId pos = pos_old + 1; pos <= n - size; ++pos) {
                JobId job_id = data.solution.jobs[pos - 1 + size];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= objective_best)
                    goto next_pos_new_left;
            }

            objective_best = objective;
            pos_new_best = pos_new;
            next_pos_new_left:;
        }

        // Loop 2: block moves right (pos_new > pos_old).
        if (data.prefix_objective[pos_old] < objective_best) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
                data.completion_times_tmp_2[machine_id] = data.completion_times[pos_old][machine_id];

            Time skip_prefix_obj = data.prefix_objective[pos_old];
            for (JobId pos_new = pos_old + 1; pos_new <= n - size; ++pos_new) {
                JobId skip_job_id = data.solution.jobs[pos_new - 1 + size];
                advance_ct<Blocking>(instance, data.completion_times_tmp_2, instance.job(skip_job_id));
                skip_prefix_obj += job_contribution(instance, skip_job_id, data.completion_times_tmp_2[last_machine_id]);
                if (skip_prefix_obj >= objective_best)
                    break;

                for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
                    data.completion_times_tmp_1[machine_id] = data.completion_times_tmp_2[machine_id];

                Time objective = skip_prefix_obj;

                // Block jobs.
                for (JobId b = pos_old; b < pos_old + size; ++b) {
                    JobId job_id = data.solution.jobs[b];
                    advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                    objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                    if (objective >= objective_best)
                        goto next_pos_new_right;
                }

                // CT-dominance pruning.
                if (objective + data.prefix_objective[n] - data.prefix_objective[pos_new + size] >= objective_best) {
                    bool dominated = true;
                    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                        if (data.completion_times_tmp_1[machine_id] < data.completion_times[pos_new + size][machine_id]) {
                            dominated = false;
                            break;
                        }
                    }
                    if (dominated)
                        goto next_pos_new_right;
                }

                // Suffix.
                for (JobId pos = pos_new + 1; pos <= n - size; ++pos) {
                    JobId job_id = data.solution.jobs[pos - 1 + size];
                    advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                    objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                    if (objective >= objective_best)
                        goto next_pos_new_right;
                }

                objective_best = objective;
                pos_new_best = pos_new;
                next_pos_new_right:;
            }
        }

        if (pos_new_best != -1) {
            shift_jobs<Blocking>(instance, data, size, pos_old, pos_new_best);
            if (data.solution.objective != objective_best) {
                throw std::runtime_error(
                        FUNC_SIGNATURE + ": wrong objective after move; "
                        "data.solution.objective: " + std::to_string(data.solution.objective) + "; "
                        "objective_best: " + std::to_string(objective_best) + ".");
            }
            improved = true;
        } else {
            ++pos_old;
        }
    }

    return improved;
}

template <bool Blocking>
bool explore_swap_neighborhood(
        const Instance& instance,
        LocalSearchData& data,
        std::mt19937_64& generator,
        JobId size_1,
        JobId size_2)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId n = data.solution.jobs.size();

    bool improved = false;

    for (JobId pos_1 = 0; pos_1 < n; ) {
        if (data.prefix_objective[pos_1] >= data.solution.objective)
            break;

        bool applied = false;

        for (JobId pos_2 = pos_1 + size_1; pos_2 + size_2 <= n; ++pos_2) {
            for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
                data.completion_times_tmp_1[machine_id] = data.completion_times[pos_1][machine_id];

            Time objective = data.prefix_objective[pos_1];

            // B2: jobs[pos_2..pos_2+size_2-1].
            for (JobId b = pos_2; b < pos_2 + size_2; ++b) {
                JobId job_id = data.solution.jobs[b];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= data.solution.objective)
                    goto next_pair;
            }

            // M: jobs[pos_1+size_1..pos_2-1].
            for (JobId pos = pos_1 + size_1; pos < pos_2; ++pos) {
                JobId job_id = data.solution.jobs[pos];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= data.solution.objective)
                    goto next_pair;
            }

            // B1: jobs[pos_1..pos_1+size_1-1].
            for (JobId b = pos_1; b < pos_1 + size_1; ++b) {
                JobId job_id = data.solution.jobs[b];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= data.solution.objective)
                    goto next_pair;
            }

            // CT-dominance pruning (ref_pos = pos_2 + size_2).
            if (objective + data.prefix_objective[n] - data.prefix_objective[pos_2 + size_2] >= data.solution.objective) {
                bool dominated = true;
                for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
                    if (data.completion_times_tmp_1[machine_id] < data.completion_times[pos_2 + size_2][machine_id]) {
                        dominated = false;
                        break;
                    }
                }
                if (dominated)
                    goto next_pair;
            }

            // Suffix: jobs[pos_2+size_2..n-1].
            for (JobId pos = pos_2 + size_2; pos < n; ++pos) {
                JobId job_id = data.solution.jobs[pos];
                advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(job_id));
                objective += job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);
                if (objective >= data.solution.objective)
                    goto next_pair;
            }

            if (data.solution.objective > objective) {
                swap_jobs<Blocking>(instance, data, pos_1, size_1, pos_2, size_2);
                if (data.solution.objective != objective) {
                    throw std::runtime_error(
                            FUNC_SIGNATURE + ": wrong objective after swap; "
                            "data.solution.objective: " + std::to_string(data.solution.objective) + "; "
                            "objective: " + std::to_string(objective) + ".");
                }
                applied = true;
                improved = true;
            }
            next_pair:;
        }

        if (!applied)
            ++pos_1;
    }

    return improved;
}

template <bool Blocking>
void local_search(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data)
{
    std::vector<std::tuple<Neighborhood, JobId, JobId>> neighborhoods = {
        {Neighborhood::Shift, 1, -1},
        {Neighborhood::Shift, 2, -1},
        {Neighborhood::Shift, 3, -1},
        //{Neighborhood::Shift, 4, -1},
        //{Neighborhood::Shift, 5, -1},
        {Neighborhood::Swap, 1, 1},
        //{Neighborhood::Swap, 1, 2},
        //{Neighborhood::Swap, 2, 1},
        //{Neighborhood::Swap, 2, 2},
        //{Neighborhood::Swap, 1, 3},
        //{Neighborhood::Swap, 3, 1},
        //{Neighborhood::Swap, 2, 3},
        //{Neighborhood::Swap, 3, 2},
        //{Neighborhood::Swap, 3, 3},
    };
    for (;;) {
        std::shuffle(neighborhoods.begin(), neighborhoods.end(), generator);

        bool improved = false;
        for (auto neighborhood: neighborhoods) {
            switch (std::get<0>(neighborhood)) {
            case Neighborhood::Shift: {
                improved = explore_shift_neighborhood<Blocking>(instance, data, generator, std::get<1>(neighborhood));
                break;
            } case Neighborhood::Swap: {
                improved = explore_swap_neighborhood<Blocking>(instance, data, generator, std::get<1>(neighborhood), std::get<2>(neighborhood));
                break;
            }
            }
            if (improved)
                break;
        }
        if (!improved)
            break;

        if (data.solution.jobs.size() == (size_t)instance.number_of_jobs()
                && (!output.solution.feasible()
                    || output.solution.objective_value() > data.solution.objective)) {
            std::stringstream ss;
            ss << "it " << output.number_of_iterations;
            algorithm_formatter.update_solution(
                    build_solution(instance, data.solution), ss.str());
        }
    }
}

// ---- Construction / perturbation helpers ----

// Insert job_id at the position that minimises the current objective.
// Uses completion_times (current solution CTs) and prefix_objective.
template <bool Blocking>
void add_job_at_best_position(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        JobId job_id,
        JobId forbidden_position = -1)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    const Job& job = instance.job(job_id);

    // Accumulate all positions achieving the minimum objective; pick one at random.
    std::vector<JobId> best_positions;
    Time objective_best = 0;  // stores best+1 (the +1 trick) when best_positions is non-empty
    JobId n = data.solution.jobs.size();

    for (JobId pos = 0; pos <= n; ++pos) {
        if (pos == forbidden_position)
            continue;
        // prefix_objective is non-decreasing and the suffix is non-negative,
        // so no later position can achieve a strictly better total.
        if (!best_positions.empty() && data.prefix_objective[pos] >= objective_best)
            break;

        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
            data.completion_times_tmp_1[machine_id] = data.completion_times[pos][machine_id];

        advance_ct<Blocking>(instance, data.completion_times_tmp_1, job);

        Time objective = data.prefix_objective[pos]
                 + job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);

        if (!best_positions.empty() && objective >= objective_best)
            continue;

        bool improved = true;
        for (JobId k = pos; k < n; ++k) {
            JobId k_job_id = data.solution.jobs[k];
            advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(k_job_id));
            objective += job_contribution(instance, k_job_id, data.completion_times_tmp_1[last_machine_id]);
            if (!best_positions.empty() && objective >= objective_best) {
                improved = false;
                break;
            }
        }

        if (!improved)
            continue;

        if (objective + 1 < objective_best)
            best_positions.clear();
        objective_best = objective + 1;
        best_positions.push_back(pos);
    }

    if (best_positions.empty())
        throw std::runtime_error(FUNC_SIGNATURE + ": best_positions is empty.");

    JobId pos_best = best_positions[
        std::uniform_int_distribution<size_t>(0, best_positions.size() - 1)(generator)];
    add_job<Blocking>(instance, data, job_id, pos_best);
}

template <bool Blocking>
void add_block_at_best_position(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        const std::vector<JobId>& job_ids)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;

    std::vector<JobId> best_positions;
    Time objective_best = 0;
    JobId n = data.solution.jobs.size();

    for (JobId pos = 0; pos <= n; ++pos) {
        if (!best_positions.empty() && data.prefix_objective[pos] >= objective_best)
            break;

        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
            data.completion_times_tmp_1[machine_id] = data.completion_times[pos][machine_id];

        Time objective = data.prefix_objective[pos];

        bool improved = true;
        for (JobId b_job_id: job_ids) {
            advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(b_job_id));
            objective += job_contribution(instance, b_job_id, data.completion_times_tmp_1[last_machine_id]);
            if (!best_positions.empty() && objective >= objective_best) {
                improved = false;
                break;
            }
        }

        if (!improved)
            continue;

        for (JobId k = pos; k < n; ++k) {
            JobId k_job_id = data.solution.jobs[k];
            advance_ct<Blocking>(instance, data.completion_times_tmp_1, instance.job(k_job_id));
            objective += job_contribution(instance, k_job_id, data.completion_times_tmp_1[last_machine_id]);
            if (!best_positions.empty() && objective >= objective_best) {
                improved = false;
                break;
            }
        }

        if (!improved)
            continue;

        if (objective + 1 < objective_best)
            best_positions.clear();
        objective_best = objective + 1;
        best_positions.push_back(pos);
    }

    if (best_positions.empty())
        throw std::runtime_error(FUNC_SIGNATURE + ": best_positions is empty.");

    JobId pos_best = best_positions[
        std::uniform_int_distribution<size_t>(0, best_positions.size() - 1)(generator)];
    add_block<Blocking>(instance, data, job_ids, pos_best);
}

// Among unscheduled jobs (jobs_positions == -1), append the one with the
// smallest objective contribution when placed at the end of the current solution.
template <bool Blocking>
void append_best_job(
        const Instance& instance,
        LocalSearchData& data)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId n = data.solution.jobs.size();
    JobId best_job_id = -1;
    Time best_contribution = -1;

    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        if (data.solution.jobs_positions[job_id] != -1)
            continue;

        const Job& job = instance.job(job_id);

        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
            data.completion_times_tmp_1[machine_id] = data.completion_times[n][machine_id];

        advance_ct<Blocking>(instance, data.completion_times_tmp_1, job);

        Time contribution = job_contribution(instance, job_id, data.completion_times_tmp_1[last_machine_id]);

        if (best_job_id == -1 || contribution < best_contribution) {
            best_job_id = job_id;
            best_contribution = contribution;
        }
    }

    add_job<Blocking>(instance, data, best_job_id, n);
}

template <bool Blocking>
JobId remove_random_job(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data)
{
    std::uniform_int_distribution<JobId> d_pos(0, (JobId)data.solution.jobs.size() - 1);
    JobId pos = d_pos(generator);
    JobId job_id = data.solution.jobs[pos];
    remove_block<Blocking>(instance, data, pos, 1);
    return job_id;
}

template <bool Blocking>
std::vector<JobId> remove_random_block(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data)
{
    JobId max_size = std::min((JobId)4, (JobId)data.solution.jobs.size());
    std::uniform_int_distribution<JobId> d_size(1, max_size);
    JobId size = d_size(generator);
    std::uniform_int_distribution<JobId> d_pos(0, (JobId)data.solution.jobs.size() - size);
    JobId pos = d_pos(generator);
    std::vector<JobId> removed(data.solution.jobs.begin() + pos, data.solution.jobs.begin() + pos + size);
    remove_block<Blocking>(instance, data, pos, size);
    return removed;
}

template <bool Blocking>
std::vector<JobId> remove_random_jobs(
        const Instance& instance,
        std::mt19937_64& generator,
        LocalSearchData& data,
        JobId number_of_jobs_removed)
{
    std::vector<JobId> positions = optimizationtools::bob_floyd(
            number_of_jobs_removed,
            (JobId)data.solution.jobs.size(),
            generator);
    std::vector<JobId> removed_job_ids;
    for (JobId pos: positions)
        removed_job_ids.push_back(data.solution.jobs[pos]);
    std::sort(positions.rbegin(), positions.rend());
    for (JobId pos: positions)
        data.solution.jobs.erase(data.solution.jobs.begin() + pos);
    JobId min_pos = positions.back();
    for (JobId k = min_pos; k < (JobId)data.solution.jobs.size(); ++k)
        data.solution.jobs_positions[data.solution.jobs[k]] = k;
    update_completion_times<Blocking>(instance, data, 0);
    update_prefix_objective(instance, data);
    return removed_job_ids;
}

template <bool Blocking>
void generate_initial_solution_neh(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data)
{
    data.solution.jobs.clear();
    std::fill(data.solution.jobs_positions.begin(), data.solution.jobs_positions.end(), -1);
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
        data.completion_times[0][machine_id] = 0;
    data.prefix_objective[0] = 0;
    data.solution.objective = 0;

    // Sort jobs by mean processing time (descending).
    std::vector<JobId> sorted_jobs(instance.number_of_jobs());
    std::iota(sorted_jobs.begin(), sorted_jobs.end(), 0);
    std::sort(
            sorted_jobs.begin(),
            sorted_jobs.end(),
            [&instance](JobId job_1_id, JobId job_2_id) -> bool
            {
                return instance.job(job_1_id).mean_processing_time
                     > instance.job(job_2_id).mean_processing_time;
            });
    for (JobId job_id: sorted_jobs) {
        add_job_at_best_position<Blocking>(instance, parameters, generator, data, job_id);
        local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
    }
}

template <bool Blocking>
void generate_initial_solution_neh_edd(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data)
{
    data.solution.jobs.clear();
    std::fill(data.solution.jobs_positions.begin(), data.solution.jobs_positions.end(), -1);
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
        data.completion_times[0][machine_id] = 0;
    data.prefix_objective[0] = 0;
    data.solution.objective = 0;

    // Sort jobs by mean processing time (descending).
    std::vector<JobId> sorted_jobs(instance.number_of_jobs());
    std::iota(sorted_jobs.begin(), sorted_jobs.end(), 0);
    std::sort(
            sorted_jobs.begin(),
            sorted_jobs.end(),
            [&instance](JobId job_1_id, JobId job_2_id) -> bool
            {
                return instance.job(job_1_id).due_date
                     < instance.job(job_2_id).due_date;
            });
    for (JobId job_id: sorted_jobs)
        add_job_at_best_position<Blocking>(instance, parameters, generator, data, job_id);
}

template <bool Blocking>
void generate_initial_solution_pf_neh(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data)
{
    data.solution.jobs.clear();
    std::fill(data.solution.jobs_positions.begin(), data.solution.jobs_positions.end(), -1);
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
        data.completion_times[0][machine_id] = 0;
    data.prefix_objective[0] = 0;
    data.solution.objective = 0;

    // Pick a random first job for diversity.
    std::uniform_int_distribution<JobId> d_first(0, instance.number_of_jobs() - 1);
    add_job<Blocking>(instance, data, d_first(generator), 0);

    while ((JobId)data.solution.jobs.size() < instance.number_of_jobs())
        append_best_job<Blocking>(instance, data);
}

// Initial solution: schedule jobs in earliest-due-date order.
template <bool Blocking>
void generate_initial_solution_edd(
        const Instance& instance,
        LocalSearchData& data)
{
    data.solution.jobs.clear();
    std::fill(data.solution.jobs_positions.begin(), data.solution.jobs_positions.end(), -1);
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
        data.completion_times[0][machine_id] = 0;
    data.prefix_objective[0] = 0;
    data.solution.objective = 0;

    std::vector<JobId> sorted_jobs(instance.number_of_jobs());
    std::iota(sorted_jobs.begin(), sorted_jobs.end(), 0);
    std::sort(
            sorted_jobs.begin(),
            sorted_jobs.end(),
            [&instance](JobId a, JobId b) {
                return instance.job(a).due_date < instance.job(b).due_date;
            });
    for (JobId job_id: sorted_jobs)
        add_job<Blocking>(instance, data, job_id, data.solution.jobs.size());
}

// Perturbation: d random adjacent swaps (IARAS from Fernandez-Viagas et al. 2018).
template <bool Blocking>
void random_adjacent_swaps(
        const Instance& instance,
        LocalSearchData& data,
        std::mt19937_64& generator,
        JobId d = 4)
{
    JobId n = data.solution.jobs.size();
    std::uniform_int_distribution<JobId> d_pos(0, n - 2);
    for (JobId i = 0; i < d; ++i) {
        JobId pos = d_pos(generator);
        std::swap(data.solution.jobs[pos], data.solution.jobs[pos + 1]);
        data.solution.jobs_positions[data.solution.jobs[pos]] = pos;
        data.solution.jobs_positions[data.solution.jobs[pos + 1]] = pos + 1;
    }
    update_completion_times<Blocking>(instance, data, 0);
    update_prefix_objective(instance, data);
}

enum class Perturbation
{
    RandomAdjacentSwaps,
    RandomShifts,
    RuinAndRecreate1,
    RuinAndRecreateJobs,
    RuinAndRecreateBlock,
};

template <bool Blocking>
void random_shifts(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        JobId d = 4)
{
    std::uniform_int_distribution<JobId> distribution(2, d);
    std::vector<JobId> positions = optimizationtools::bob_floyd(
            distribution(generator),
            (JobId)data.solution.jobs.size(),
            generator);
    std::vector<JobId> job_ids;
    for (JobId pos: positions)
        job_ids.push_back(data.solution.jobs[pos]);

    for (JobId job_id: job_ids) {
        JobId forbidden_position = data.solution.jobs_positions[job_id];
        remove_job<Blocking>(instance, data, forbidden_position);
        add_job_at_best_position<Blocking>(instance, parameters, generator, data, job_id, forbidden_position);
    }
}

template <bool Blocking>
void ruin_and_recreate_1(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        LocalSearchData& data,
        std::mt19937_64& generator,
        const LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter)
{
    JobId removed_job_id = remove_random_job<Blocking>(instance, parameters, generator, data);
    local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
    add_job_at_best_position<Blocking>(instance, parameters, generator, data, removed_job_id);
}

template <bool Blocking>
void ruin_and_recreate_jobs(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        LocalSearchData& data,
        std::mt19937_64& generator,
        const LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        JobId d = 4)
{
    std::vector<JobId> removed = remove_random_jobs<Blocking>(instance, generator, data, d);
    local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
    for (JobId job_id: removed)
        add_job_at_best_position<Blocking>(instance, parameters, generator, data, job_id);
}

template <bool Blocking>
void ruin_and_recreate_block(
        const Instance& instance,
        const LocalSearchPfssParameters& parameters,
        LocalSearchData& data,
        std::mt19937_64& generator,
        const LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter)
{
    std::vector<JobId> removed = remove_random_block<Blocking>(instance, parameters, generator, data);
    local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
    add_block_at_best_position<Blocking>(instance, parameters, generator, data, removed);
}

// Run the full population-based iterated local search.
// Blocking is resolved at compile time; called from local_search_pfss via
// if/else dispatch so there is only one runtime branch.
template <bool Blocking>
void run_algorithm(
        const Instance& instance,
        std::mt19937_64& generator,
        Solution* initial_solution,
        const LocalSearchPfssParameters& parameters,
        LocalSearchPfssOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data,
        const localsearchsolver::Population<LocalSearchSolution, Time>::Parameters& pop_params,
        localsearchsolver::Population<LocalSearchSolution, Time>& population)
{
    if (initial_solution != nullptr) {
        load_solution<Blocking>(data, instance, *initial_solution);
        local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
    } else {
        TreeSearchPfssParameters ts_parameters;
        ts_parameters.timer = parameters.timer;
        ts_parameters.verbosity_level = 0;
        ts_parameters.minimum_size_of_the_queue = 1024;
        ts_parameters.maximum_size_of_the_queue = 1024;
        auto ts_output = tree_search_pfss(
                instance,
                ts_parameters);
        load_solution<Blocking>(data, instance, ts_output.solution);
        std::stringstream ss;
        ss << "ts";
        algorithm_formatter.update_solution(
                build_solution(instance, data.solution), ss.str());

        //generate_initial_solution_neh_edd<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
        //std::stringstream ss;
        //ss << "NEH EDD";
        //algorithm_formatter.update_solution(
        //        build_solution(instance, data.solution), ss.str());

        //generate_initial_solution_edd<Blocking>(instance, data);

        local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
        population.add(data.solution, generator);

        //for (JobId i = 0;
        //        i < pop_params.maximum_size && !parameters.timer.needs_to_end();
        //        ++i) {
        //    generate_initial_solution_pf_neh<Blocking>(instance, parameters, generator, data);
        //    std::stringstream ss;
        //    ss << "neh " << i;
        //    algorithm_formatter.update_solution(
        //            build_solution(instance, data.solution), ss.str());
        //    local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);
        //    population.add(data.solution, generator);
        //}
    }

    if (population.size() == 0)
        return;

    Counter number_of_iterations_without_improvement = 0;
    double previous_best = output.solution.objective_value();

    std::vector<Perturbation> perturbations = {
        Perturbation::RandomShifts,
        Perturbation::RuinAndRecreateJobs,
    };
    std::vector<Counter> perturbation_successes(perturbations.size(), 0);

    for (output.number_of_iterations = 1;
            !parameters.timer.needs_to_end();
            ++output.number_of_iterations) {

        if (output.solution.feasible()
                && output.solution.objective_value() <= output.bound())
            break;
        if (parameters.maximum_number_of_iterations >= 0
                && output.number_of_iterations > parameters.maximum_number_of_iterations)
            break;
        if (parameters.maximum_number_of_iterations_without_improvement >= 0
                && number_of_iterations_without_improvement
                   > parameters.maximum_number_of_iterations_without_improvement)
            break;

        data.solution = (output.number_of_iterations < 64)?
            population.best_solution():
            population.binary_tournament_single(generator);
        update_completion_times<Blocking>(instance, data, 0);
        update_prefix_objective(instance, data);
        Time objective_before = data.solution.objective;

        // Draw perturbation using adaptive weights.
        std::vector<double> weights;
        for (Counter i = 0; i < (Counter)perturbations.size(); ++i)
            weights.push_back(2.0 + perturbation_successes[i]);
        std::discrete_distribution<Counter> distribution_perturbation(weights.begin(), weights.end());
        Counter perturbation_pos = distribution_perturbation(generator);
        Perturbation perturbation = perturbations[perturbation_pos];

        switch (perturbation) {
        case Perturbation::RandomAdjacentSwaps:
            random_adjacent_swaps<Blocking>(instance, data, generator);
            break;
        case Perturbation::RandomShifts:
            random_shifts<Blocking>(instance, parameters, generator, data);
            break;
        case Perturbation::RuinAndRecreate1:
            ruin_and_recreate_1<Blocking>(instance, parameters, data, generator, output, algorithm_formatter);
            break;
        case Perturbation::RuinAndRecreateJobs:
            ruin_and_recreate_jobs<Blocking>(instance, parameters, data, generator, output, algorithm_formatter);
            break;
        case Perturbation::RuinAndRecreateBlock:
            ruin_and_recreate_block<Blocking>(instance, parameters, data, generator, output, algorithm_formatter);
            break;
        }

        local_search<Blocking>(instance, parameters, generator, output, algorithm_formatter, data);

        if (data.solution.objective < objective_before)
            perturbation_successes[perturbation_pos]++;

        population.add(data.solution, generator);

        if (output.solution.objective_value() < previous_best) {
            previous_best = output.solution.objective_value();
            number_of_iterations_without_improvement = 0;
        } else {
            ++number_of_iterations_without_improvement;
        }
    }
}

} // namespace

const LocalSearchPfssOutput shopschedulingsolver::local_search_pfss(
        const Instance& instance,
        std::mt19937_64& generator,
        Solution* initial_solution,
        const LocalSearchPfssParameters& parameters)
{
    LocalSearchPfssOutput output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Local search (pfss)");

    if (parameters.timer.needs_to_end()) {
        algorithm_formatter.end();
        return output;
    }

    if (instance.objective() != Objective::TotalFlowTime
            && instance.objective() != Objective::TotalTardiness) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": unsupported objective; "
                "only TotalFlowTime and TotalTardiness are supported.");
    }

    if (instance.no_wait() || instance.no_idle()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": no-wait and no-idle are not supported.");
    }

    algorithm_formatter.print_header();

    JobId n = instance.number_of_jobs();

    LocalSearchData data;
    data.completion_times.assign(n + 1, std::vector<Time>(instance.number_of_machines(), 0));
    data.completion_times_tmp_1.resize(instance.number_of_machines(), 0);
    data.completion_times_tmp_2.resize(instance.number_of_machines(), 0);
    data.prefix_objective.resize(n + 1, 0);
    data.best_moves.reserve(n);
    data.solution.jobs_positions.resize(n, -1);

    localsearchsolver::PenalizedCostCallback<LocalSearchSolution, Time> penalized_cost_callback =
        [](const LocalSearchSolution& s) { return s.objective; };

    localsearchsolver::DistanceCallback<LocalSearchSolution> distance_callback =
        [&instance](const LocalSearchSolution& s1, const LocalSearchSolution& s2) {
            localsearchsolver::Distance dist = 0;
            for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
                JobId p1 = s1.jobs_positions[job_id];
                JobId p2 = s2.jobs_positions[job_id];
                if (p1 == 0) {
                    if (p2 != 0) dist++;
                } else if (p2 == 0) {
                    dist++;
                } else {
                    if (s1.jobs[p1 - 1] != s2.jobs[p2 - 1]) dist++;
                }
                JobId last_pos = (JobId)s1.jobs.size() - 1;
                if (p1 == last_pos) {
                    if (p2 != last_pos) dist++;
                } else if (p2 == last_pos) {
                    dist++;
                } else {
                    if (s1.jobs[p1 + 1] != s2.jobs[p2 + 1]) dist++;
                }
            }
            return dist;
        };

    localsearchsolver::Population<LocalSearchSolution, Time>::Parameters pop_params;
    pop_params.minimum_size = 20;
    pop_params.maximum_size = 40;
    pop_params.number_of_elite_solutions = 10;
    pop_params.number_of_closest_neighbors = 3;
    localsearchsolver::Population<LocalSearchSolution, Time> population(
            penalized_cost_callback, distance_callback, pop_params);

    // Dispatch once here; all inner functions are templated so no further
    // runtime branch occurs.
    if (instance.blocking()) {
        run_algorithm<true>(
                instance, generator, initial_solution, parameters,
                output, algorithm_formatter, data, pop_params, population);
    } else {
        run_algorithm<false>(
                instance, generator, initial_solution, parameters,
                output, algorithm_formatter, data, pop_params, population);
    }

    algorithm_formatter.end();
    return output;
}
