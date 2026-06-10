#include "shopschedulingsolver/algorithms/local_search_pfss_makespan.hpp"

#include "shopschedulingsolver/algorithm_formatter.hpp"

#include "localsearchsolver/population.hpp"

#include "optimizationtools/utils/common.hpp"

using namespace shopschedulingsolver;

namespace
{

struct LocalSearchSolution
{
    std::vector<JobId> jobs;
    std::vector<JobId> jobs_positions;
    Time makespan = 0;
};

Solution build_solution(
        const Instance& instance,
        LocalSearchSolution& ls_solution)
{
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    solution_builder.from_permutation(ls_solution.jobs);
    Solution solution = solution_builder.build();
    if (solution.makespan() != ls_solution.makespan) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": wrong makespan; "
                "solution.makespan(): " + std::to_string(solution.makespan()) + "; "
                "ls_solution.makespan: " + std::to_string(ls_solution.makespan) + ".");
    }
    return solution;
}

// Per-position critical path data.
// start_machine_id: the machine where the critical path enters this position.
// critical_path[k+1].start_machine_id serves as the end machine of position k.
// critical_path[n] is a sentinel with start_machine_id = m-1.
// Contribution of position k = job_contribution(jobs[k],
//     critical_path[k].start_machine_id, critical_path[k+1].start_machine_id).
// antiblock_start, left_shift_max, right_shift_min: only used for blocking instances.
//   left_shift_max  = first position of this block − 1 (valid left-shift targets are ≤ this).
//   right_shift_min = first position after this block  (valid right-shift targets are ≥ this).
struct CriticalJob
{
    MachineId start_machine_id = 0;
    JobId antiblock_start = 0;
    JobId left_shift_max = -1;
    JobId right_shift_min = 0;
};

// Returns sum of p[job_id][start..end] (inclusive), or 0 when end < start.
inline Time job_contribution(
        const std::vector<std::vector<Time>>& job_prefix_sums,
        JobId job_id,
        MachineId start_machine,
        MachineId end_machine)
{
    if (end_machine < start_machine)
        return 0;
    return job_prefix_sums[job_id][end_machine + 1]
        - job_prefix_sums[job_id][start_machine];
}

struct LocalSearchData
{
    LocalSearchSolution solution;

    std::vector<std::vector<Time>> completion_times_0;
    std::vector<std::vector<Time>> reverse_completion_times_0;

    std::vector<std::vector<Time>> completion_times;
    std::vector<std::vector<Time>> reverse_completion_times;

    std::vector<Time> completion_times_2;
    std::vector<Time> makespans;

    // critical_path[0..n-1]: per-position data; critical_path[n]: sentinel (start_machine_id = m-1).
    std::vector<CriticalJob> critical_path;

    // job_prefix_sums[job_id][k] = sum of p[job_id][0..k-1], with job_prefix_sums[job_id][0] = 0.
    // Range sum: sum(p[job_id][first..last]) = job_prefix_sums[job_id][last+1] - job_prefix_sums[job_id][first].
    std::vector<std::vector<Time>> job_prefix_sums;
};

void update_completion_times(
        const Instance& instance,
        LocalSearchData& data,
        JobId p)
{
    //std::cout << "update_completion_times p " << p << std::endl;
    if (instance.blocking()) {
        MachineId last_machine_id = instance.number_of_machines() - 1;
        for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
            JobId job_id = data.solution.jobs[pos - 1];
            const Job& job = instance.job(job_id);
            {
                Time p0 = job.operations[0].alternatives[0].processing_time;
                // D[pos][0] = max(D[pos-1][0] + p, D[pos-1][1])
                if (last_machine_id > 0) {
                    data.completion_times_0[pos][0] = (std::max)(
                            data.completion_times_0[pos - 1][0] + p0,
                            data.completion_times_0[pos - 1][1]);
                } else {
                    data.completion_times_0[pos][0] = data.completion_times_0[pos - 1][0] + p0;
                }
            }
            for (MachineId machine_id = 1;
                    machine_id < instance.number_of_machines() - 1;
                    ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                // D[pos][m] = max(D[pos][m-1] + p, D[pos-1][m+1])
                data.completion_times_0[pos][machine_id] = (std::max)(
                        data.completion_times_0[pos][machine_id - 1] + p,
                        data.completion_times_0[pos - 1][machine_id + 1]);
            }
            if (last_machine_id > 0) {
                Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                data.completion_times_0[pos][last_machine_id] = data.completion_times_0[pos][last_machine_id - 1] + p;
            }
        }
    } else if (instance.no_idle()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": no-idle not supported.");
    } else if (instance.no_wait()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": no-wait not supported.");
    } else {
        for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
            JobId job_id = data.solution.jobs[pos - 1];
            const Job& job = instance.job(job_id);
            Time p0 = job.operations[0].alternatives[0].processing_time;
            data.completion_times_0[pos][0] = data.completion_times_0[pos - 1][0] + p0;
            //std::cout << "compute c"
            //    << " pos " << pos
            //    << " job_id " << job_id
            //    << " machine_id " << 0
            //    << " p " << p0
            //    << " c " << data.completion_times_0[pos][0]
            //    << std::endl;
            for (MachineId machine_id = 1;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                if (data.completion_times_0[pos - 1][machine_id] > data.completion_times_0[pos][machine_id - 1]) {
                    data.completion_times_0[pos][machine_id] = data.completion_times_0[pos - 1][machine_id] + p;
                } else {
                    data.completion_times_0[pos][machine_id] = data.completion_times_0[pos][machine_id - 1] + p;
                }
                //std::cout << "compute c"
                //    << " pos " << pos
                //    << " job_id " << job_id
                //    << " machine_id " << machine_id
                //    << " p " << p
                //    << " c " << data.completion_times_0[pos][machine_id]
                //    << std::endl;
            }
        }
    }
}

void update_reverse_completion_times(
        const Instance& instance,
        LocalSearchData& data,
        JobId p)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId last_job_pos = data.solution.jobs.size() - 1;
    if (instance.blocking()) {
        for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
            JobId job_id = data.solution.jobs[last_job_pos - pos + 1];
            const Job& job = instance.job(job_id);
            {
                Time p0 = job.operations[last_machine_id].alternatives[0].processing_time;
                // R[pos][M-1] = max(R[pos-1][M-1] + p, R[pos-1][M-2])
                if (last_machine_id > 0) {
                    data.reverse_completion_times_0[pos][last_machine_id] = (std::max)(
                            data.reverse_completion_times_0[pos - 1][last_machine_id] + p0,
                            data.reverse_completion_times_0[pos - 1][last_machine_id - 1]);
                } else {
                    data.reverse_completion_times_0[pos][last_machine_id] = data.reverse_completion_times_0[pos - 1][last_machine_id] + p0;
                }
            }
            for (MachineId machine_id = last_machine_id - 1;
                    machine_id >= 1;
                    --machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                // R[pos][m] = max(R[pos][m+1] + p, R[pos-1][m-1])
                data.reverse_completion_times_0[pos][machine_id] = (std::max)(
                        data.reverse_completion_times_0[pos][machine_id + 1] + p,
                        data.reverse_completion_times_0[pos - 1][machine_id - 1]);
            }
            if (last_machine_id > 0) {
                Time p = job.operations[0].alternatives[0].processing_time;
                data.reverse_completion_times_0[pos][0] = data.reverse_completion_times_0[pos][1] + p;
            }
        }
    } else if (instance.no_idle()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": no-idle not supported.");
    } else if (instance.no_wait()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": no-wait not supported.");
    } else {
        for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
            JobId job_id = data.solution.jobs[last_job_pos - pos + 1];
            const Job& job = instance.job(job_id);
            Time p0 = job.operations[last_machine_id].alternatives[0].processing_time;
            data.reverse_completion_times_0[pos][last_machine_id] = data.reverse_completion_times_0[pos - 1][last_machine_id] + p0;
            for (MachineId machine_id = last_machine_id - 1;
                    machine_id >= 0;
                    --machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                if (data.reverse_completion_times_0[pos - 1][machine_id] > data.reverse_completion_times_0[pos][machine_id + 1]) {
                    data.reverse_completion_times_0[pos][machine_id] = data.reverse_completion_times_0[pos - 1][machine_id] + p;
                } else {
                    data.reverse_completion_times_0[pos][machine_id] = data.reverse_completion_times_0[pos][machine_id + 1] + p;
                }
            }
        }
    }
}

void compute_critical_path_machines(
        const Instance& instance,
        LocalSearchData& data)
{
    if (data.solution.jobs.empty())
        return;

    // Sentinel: the path exits at the last machine.
    data.critical_path[data.solution.jobs.size()].start_machine_id =
        instance.number_of_machines() - 1;

    JobId pos = data.solution.jobs.size();
    MachineId machine_id = instance.number_of_machines() - 1;

    while (pos > 0 || machine_id > 0) {
        bool job_hop;
        bool machine_next = false;

        if (instance.blocking()) {
            if (pos > 0
                    && machine_id < instance.number_of_machines() - 1
                    && data.completion_times_0[pos][machine_id]
                    == data.completion_times_0[pos - 1][machine_id + 1]) {
                job_hop = true;
                machine_next = true;
            } else if (pos > 0
                    && data.completion_times_0[pos][machine_id]
                    == data.completion_times_0[pos - 1][machine_id]
                    + instance.job(data.solution.jobs[pos - 1])
                            .operations[machine_id].alternatives[0].processing_time) {
                job_hop = true;
            } else {
                job_hop = false;
            }
        } else {
            if (machine_id == 0
                    || (pos > 0
                    && data.completion_times_0[pos - 1][machine_id]
                    >= data.completion_times_0[pos][machine_id - 1])) {
                job_hop = true;
            } else {
                job_hop = false;
            }
        }

        if (!job_hop) {
            --machine_id;
        } else {
            data.critical_path[pos - 1].start_machine_id = machine_id;
            --pos;
            if (machine_next)
                ++machine_id;
        }
    }

    // For blocking instances: compute antiblock_start, left_shift_max, right_shift_min.
    // antiblock_start[p] = start of the leaving-F⁻ arc run containing p, or -1 if the
    // leaving arc at p is not F⁻ (i.e. start_machine_id[p+1] != start_machine_id[p] - 1).
    // Position 0 always gets antiblock_start = -1: start_machine_id[0] = 0, so a leaving
    // F⁻ arc would require start_machine_id[1] = -1, which is impossible.
    if (instance.blocking()) {
        // Forward pass: antiblock_start and left_shift_max.
        data.critical_path[0].antiblock_start = -1;
        data.critical_path[0].left_shift_max = -1;
        for (JobId p = 1; p < (JobId)data.solution.jobs.size(); ++p) {
            MachineId prev_machine_id = data.critical_path[p - 1].start_machine_id;
            MachineId start_machine_id = data.critical_path[p].start_machine_id;
            MachineId end_machine_id = data.critical_path[p + 1].start_machine_id;
            data.critical_path[p].left_shift_max = (prev_machine_id == start_machine_id)?
                data.critical_path[p - 1].left_shift_max: p - 1;
            if (end_machine_id != start_machine_id - 1) {
                data.critical_path[p].antiblock_start = -1;
            } else {
                data.critical_path[p].antiblock_start = (data.critical_path[p - 1].antiblock_start != -1)?
                    data.critical_path[p - 1].antiblock_start: p;
                data.critical_path[p].left_shift_max = (std::min)(
                        data.critical_path[p].left_shift_max,
                        data.critical_path[p].antiblock_start);
            }
        }

        // Backward pass: right_shift_min.
        data.critical_path[data.solution.jobs.size() - 1].right_shift_min = data.solution.jobs.size();
        for (JobId p = (JobId)data.solution.jobs.size() - 2; p >= 0; --p) {
            MachineId start_machine_id = data.critical_path[p].start_machine_id;
            MachineId end_machine_id = data.critical_path[p + 1].start_machine_id;
            bool same_block = (start_machine_id == end_machine_id)
                || (data.critical_path[p].antiblock_start != -1
                    && data.critical_path[p].antiblock_start
                       == data.critical_path[p + 1].antiblock_start);
            data.critical_path[p].right_shift_min = same_block?
                data.critical_path[p + 1].right_shift_min: p + 1;
        }

    } else {
        // For non-blocking: a block is a maximal run of positions with the
        // same start_machine_id; left_shift_max/right_shift_min bound it.
        data.critical_path[0].left_shift_max = -1;
        for (JobId p = 1; p < (JobId)data.solution.jobs.size(); ++p) {
            MachineId prev_machine_id = data.critical_path[p - 1].start_machine_id;
            MachineId start_machine_id = data.critical_path[p].start_machine_id;
            data.critical_path[p].left_shift_max = (prev_machine_id == start_machine_id)?
                data.critical_path[p - 1].left_shift_max: p - 1;
        }
        data.critical_path[data.solution.jobs.size() - 1].right_shift_min = data.solution.jobs.size();
        for (JobId p = (JobId)data.solution.jobs.size() - 2; p >= 0; --p) {
            MachineId start_machine_id = data.critical_path[p].start_machine_id;
            MachineId end_machine_id = data.critical_path[p + 1].start_machine_id;
            data.critical_path[p].right_shift_min = (start_machine_id == end_machine_id)?
                data.critical_path[p + 1].right_shift_min: p + 1;
        }
    }
}

void update_data(
        const Instance& instance,
        LocalSearchData& data)
{
    update_completion_times(instance, data, 0);
    update_reverse_completion_times(instance, data, 0);
    compute_critical_path_machines(instance, data);
    JobId n = data.solution.jobs.size();
    data.solution.makespan = data.completion_times_0[n][instance.number_of_machines() - 1];
}

void load_solution(
        LocalSearchData& data,
        const Solution& solution)
{
    const Instance& instance = solution.instance();
    // Update jobs and jobs_positions.
    data.solution.jobs.clear();
    std::fill(data.solution.jobs_positions.begin(), data.solution.jobs_positions.end(), -1);
    for (SolutionOperationId solution_operation_id = 0;
            solution_operation_id < solution.number_of_operations();
            ++solution_operation_id) {
        const Solution::Operation& solution_operation = solution.operation(solution_operation_id);
        if (solution_operation.machine_id != 0)
            continue;
        data.solution.jobs.push_back(solution_operation.job_id);
        data.solution.jobs_positions[solution_operation.job_id] = solution_operation.machine_position;
    }
    update_data(instance, data);
};

void add_job(
        const Instance& instance,
        LocalSearchData& data,
        JobId job_id,
        JobId pos_new)
{
    // Update data.solution.jobs.
    data.solution.jobs.insert(data.solution.jobs.begin() + pos_new, job_id);
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    update_data(instance, data);
}

void add_block(
        const Instance& instance,
        LocalSearchData& data,
        const std::vector<JobId>& job_ids,
        JobId pos_new)
{
    // Update data.solution.jobs.
    data.solution.jobs.insert(
            data.solution.jobs.begin() + pos_new,
            job_ids.begin(),
            job_ids.end());
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    update_data(instance, data);
}

void remove_job(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos_new)
{
    data.solution.jobs.erase(data.solution.jobs.begin() + pos_new);
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    update_data(instance, data);
}

void remove_jobs(
        const Instance& instance,
        LocalSearchData& data,
        std::vector<JobId>& positions)
{
    // Update data.solution.jobs.
    std::sort(positions.rbegin(), positions.rend());
    for (JobId pos: positions)
        data.solution.jobs.erase(data.solution.jobs.begin() + pos);
    // Update data.solution.jobs_positions.
    for (JobId pos = positions.front(); pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    update_data(instance, data);
}

void remove_block(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos_new,
        JobId size)
{
    data.solution.jobs.erase(
            data.solution.jobs.begin() + pos_new,
            data.solution.jobs.begin() + pos_new + size);
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    update_data(instance, data);
}

void shift_jobs(
        const Instance& instance,
        LocalSearchData& data,
        JobId size,
        JobId pos_old,
        JobId pos_new,
        bool reverse = false)
{
    if (pos_new > pos_old) {
        std::rotate(
                data.solution.jobs.begin() + pos_old,
                data.solution.jobs.begin() + pos_old + size,
                data.solution.jobs.begin() + pos_new + size);
    } else {
        std::rotate(
                data.solution.jobs.begin() + pos_new,
                data.solution.jobs.begin() + pos_old,
                data.solution.jobs.begin() + pos_old + size);
    }
    if (reverse) {
        std::reverse(
                data.solution.jobs.begin() + pos_new,
                data.solution.jobs.begin() + pos_new + size);
    }
    // Update data.solution.jobs_positions.
    JobId pos_min = std::min(pos_old, pos_new);
    JobId pos_max = std::max(pos_old, pos_new) + size;
    for (JobId pos = pos_min; pos < pos_max; ++pos)
        data.solution.jobs_positions[data.solution.jobs[pos]] = pos;
    update_data(instance, data);
}

void swap_jobs(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos_1,
        JobId pos_2)
{
    std::swap(data.solution.jobs[pos_1], data.solution.jobs[pos_2]);
    data.solution.jobs_positions[data.solution.jobs[pos_1]] = pos_1;
    data.solution.jobs_positions[data.solution.jobs[pos_2]] = pos_2;
    update_data(instance, data);
}

enum class Neighborhood
{
    Shift1,
    Shift2,
    Shift3,
    Shift4,
    ShiftReverse2,
    ShiftReverse3,
    ShiftReverse4,
    Swap,
};

bool explore_shift_job_neighborhood(
        const Instance& instance,
        LocalSearchData& data,
        std::mt19937_64& generator)
{
    //std::cout << "explore_shift_job_neighborhood" << std::endl;
    MachineId last_machine_id = instance.number_of_machines() - 1;
    const JobId size = 1;

    bool improved = false;

    for (JobId pos_old = 0; pos_old + size <= (JobId)data.solution.jobs.size(); ) {
        JobId pos_new_best = -1;
        Time makespan_new_best = data.solution.makespan;
        if (instance.blocking()) {
            // Compute data.completion_times.
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.completion_times[pos_old][machine_id] = data.completion_times_0[pos_old][machine_id];
            }
            for (JobId pos = pos_old + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_id = data.solution.jobs[pos - 1 + size];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[0].alternatives[0].processing_time;
                if (last_machine_id > 0) {
                    data.completion_times[pos][0] = (std::max)(
                            data.completion_times[pos - 1][0] + p0,
                            data.completion_times[pos - 1][1]);
                } else {
                    data.completion_times[pos][0] = data.completion_times[pos - 1][0] + p0;
                }
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines() - 1;
                        ++machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    data.completion_times[pos][machine_id] = (std::max)(
                            data.completion_times[pos][machine_id - 1] + p,
                            data.completion_times[pos - 1][machine_id + 1]);
                }
                if (last_machine_id > 0) {
                    Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                    data.completion_times[pos][last_machine_id] = data.completion_times[pos][last_machine_id - 1] + p;
                }
            }

            // Compute data.reverse_completion_times.
            JobId p = data.solution.jobs.size() - pos_old - size;
            if (p < 0 || p >= data.reverse_completion_times.size()) {
                throw std::logic_error(
                        FUNC_SIGNATURE + ": wrong 'p'; "
                        "p: " + std::to_string(p) + "; "
                        "data.reverse_completion_times.size(): " + std::to_string(data.reverse_completion_times.size()) + "; "
                        "pos_old: " + std::to_string(pos_old) + ".");
            }
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.reverse_completion_times[p][machine_id] = data.reverse_completion_times_0[p][machine_id];
            }
            for (JobId pos = p + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_pos = data.solution.jobs.size() - pos - size;
                if (job_pos < 0 || job_pos >= data.solution.jobs.size()) {
                    throw std::logic_error(
                            FUNC_SIGNATURE + ": wrong 'job_pos'; "
                            "job_pos: " + std::to_string(job_pos) + "; "
                            "pos: " + std::to_string(pos) + "; "
                            "data.solution.jobs.size(): " + std::to_string(data.solution.jobs.size()) +  ".");
                }
                JobId job_id = data.solution.jobs[job_pos];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[last_machine_id].alternatives[0].processing_time;
                if (last_machine_id > 0) {
                    data.reverse_completion_times[pos][last_machine_id] = (std::max)(
                            data.reverse_completion_times[pos - 1][last_machine_id] + p0,
                            data.reverse_completion_times[pos - 1][last_machine_id - 1]);
                } else {
                    data.reverse_completion_times[pos][last_machine_id] = data.reverse_completion_times[pos - 1][last_machine_id] + p0;
                }
                for (MachineId machine_id = last_machine_id - 1;
                        machine_id >= 1;
                        --machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    data.reverse_completion_times[pos][machine_id] = (std::max)(
                            data.reverse_completion_times[pos][machine_id + 1] + p,
                            data.reverse_completion_times[pos - 1][machine_id - 1]);
                }
                if (last_machine_id > 0) {
                    Time p = job.operations[0].alternatives[0].processing_time;
                    data.reverse_completion_times[pos][0] = data.reverse_completion_times[pos][1] + p;
                }
            }

            // Precompute Ding et al. (2016) LB data for this pos_old.
            // Theorem 2 (GH source):  removal_delta = -p[shifted][machine[pos_old]].
            // Theorem 3 (anti-block source): removal_delta = -p[job[ab_start-1]][machine[ab_start]],
            //   where ab_start = antiblock_start (0 if the anti-block begins at position 0, in
            //   which case there is no preceding job and removal_delta = 0).
            JobId shifted_job_id = data.solution.jobs[pos_old];
            const Job& shifted_job = instance.job(shifted_job_id);
            MachineId start_machine_id = data.critical_path[pos_old].start_machine_id;
            MachineId end_machine_id = data.critical_path[pos_old + 1].start_machine_id;
            Time delta = 0;
            if (end_machine_id != start_machine_id - 1) {
                // Remove contribution of the shifted job.
                delta -= job_contribution(data.job_prefix_sums, shifted_job_id, start_machine_id, end_machine_id);
                if (pos_old != 0) {
                    JobId previous_job_id = data.solution.jobs[pos_old - 1];
                    MachineId machine_id = data.critical_path[pos_old - 1].start_machine_id;
                    // Remove the contribution of the previous job.
                    delta -= job_contribution(data.job_prefix_sums, previous_job_id, machine_id, start_machine_id);
                    // Add the new contribution of the previous job.
                    delta += job_contribution(data.job_prefix_sums, previous_job_id, machine_id, end_machine_id);
                }
            } else {
                JobId ab_start = data.critical_path[pos_old].antiblock_start;
                if (ab_start <= 0) {
                    std::cout << "pos_old " << pos_old << std::endl;
                    for (JobId pos = 0; pos < (JobId)data.solution.jobs.size(); ++pos)
                        std::cout << " " << pos << "," << data.solution.jobs[pos]
                            << "," << data.critical_path[pos].start_machine_id
                            << "," << data.critical_path[pos].antiblock_start;
                    std::cout << std::endl;
                    throw std::invalid_argument(FUNC_SIGNATURE);
                }
                MachineId ab_machine = data.critical_path[ab_start].start_machine_id;
                delta -= instance.job(data.solution.jobs[ab_start - 1])
                    .operations[ab_machine].alternatives[0].processing_time;
            }

            // Evaluate each candidate insertion position.
            // Same-block positions (including pos_old itself) are bounded by
            // left_shift_max and right_shift_min and skipped in one range check
            // (Grabowski & Pempera 2007).
            for (JobId pos_new = 0;
                    pos_new <= (JobId)data.solution.jobs.size() - size;
                    ++pos_new) {
                if (pos_new > data.critical_path[pos_old].left_shift_max
                        && pos_new < data.critical_path[pos_old].right_shift_min) {
                    continue;
                }

                // Ding et al. (2016) lower bound (Theorems 2 and 3).
                MachineId sm_ins = (pos_new < pos_old)?
                    data.critical_path[pos_new].start_machine_id:
                    data.critical_path[pos_new + 1].start_machine_id;
                Time lower_bound = data.solution.makespan + delta + shifted_job.operations[sm_ins].alternatives[0].processing_time;
                if (lower_bound >= makespan_new_best)
                    continue;

                if (pos_new < pos_old) {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times_0[pos_new][machine_id];
                    }
                } else {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times[pos_new][machine_id];
                    }
                }
                {
                    JobId job_id = data.solution.jobs[pos_old];
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    // D2[0] = max(D2[0] + p, D2[1]); D2[1] is still old at this point.
                    if (last_machine_id > 0) {
                        data.completion_times_2[0] = (std::max)(
                                data.completion_times_2[0] + p0,
                                data.completion_times_2[1]);
                    } else {
                        data.completion_times_2[0] = data.completion_times_2[0] + p0;
                    }
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines() - 1;
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        // D2[m] = max(new D2[m-1] + p, old D2[m+1])
                        data.completion_times_2[machine_id] = (std::max)(
                                data.completion_times_2[machine_id - 1] + p,
                                data.completion_times_2[machine_id + 1]);
                    }
                    if (last_machine_id > 0) {
                        Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                        data.completion_times_2[last_machine_id] = data.completion_times_2[last_machine_id - 1] + p;
                    }
                }

                Time makespan = 0;
                const auto& reverse_completion_times = (pos_new > pos_old)?
                    data.reverse_completion_times_0:
                    data.reverse_completion_times;
                JobId p = data.solution.jobs.size() - pos_new - size;
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time completion_time = data.completion_times_2[machine_id]
                        + reverse_completion_times[p][machine_id];
                    makespan = std::max(makespan, completion_time);
                }

                if (makespan_new_best > makespan) {
                    makespan_new_best = makespan;
                    pos_new_best = pos_new;
                }
            }

        } else if (instance.no_idle()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": no-idle not supported.");

        } else if (instance.no_wait()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": no-wait not supported.");

        } else {
            // Compute data.completion_times.
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.completion_times[pos_old][machine_id] = data.completion_times_0[pos_old][machine_id];
            }
            for (JobId pos = pos_old + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_id = data.solution.jobs[pos - 1 + size];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[0].alternatives[0].processing_time;
                data.completion_times[pos][0] = data.completion_times[pos - 1][0] + p0;
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    if (data.completion_times[pos - 1][machine_id] > data.completion_times[pos][machine_id - 1]) {
                        data.completion_times[pos][machine_id] = data.completion_times[pos - 1][machine_id] + p;
                    } else {
                        data.completion_times[pos][machine_id] = data.completion_times[pos][machine_id - 1] + p;
                    }
                }
            }

            // Compute data.reverse_completion_times.
            JobId p = data.solution.jobs.size() - pos_old - size;
            if (p < 0 || p >= data.reverse_completion_times.size()) {
                throw std::logic_error(
                        FUNC_SIGNATURE + ": wrong 'p'; "
                        "p: " + std::to_string(p) + "; "
                        "data.reverse_completion_times.size(): " + std::to_string(data.reverse_completion_times.size()) + "; "
                        "pos_old: " + std::to_string(pos_old) + ".");
            }
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.reverse_completion_times[p][machine_id] = data.reverse_completion_times_0[p][machine_id];
            }
            for (JobId pos = p + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_pos = data.solution.jobs.size() - pos - size;
                if (job_pos < 0 || job_pos >= data.solution.jobs.size()) {
                    throw std::logic_error(
                            FUNC_SIGNATURE + ": wrong 'job_pos'; "
                            "job_pos: " + std::to_string(job_pos) + "; "
                            "pos: " + std::to_string(pos) + "; "
                            "data.solution.jobs.size(): " + std::to_string(data.solution.jobs.size()) +  ".");
                }
                JobId job_id = data.solution.jobs[job_pos];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[last_machine_id].alternatives[0].processing_time;
                data.reverse_completion_times[pos][last_machine_id] = data.reverse_completion_times[pos - 1][last_machine_id] + p0;
                for (MachineId machine_id = last_machine_id - 1;
                        machine_id >= 0;
                        --machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    if (data.reverse_completion_times[pos - 1][machine_id] > data.reverse_completion_times[pos][machine_id + 1]) {
                        data.reverse_completion_times[pos][machine_id] = data.reverse_completion_times[pos - 1][machine_id] + p;
                    } else {
                        data.reverse_completion_times[pos][machine_id] = data.reverse_completion_times[pos][machine_id + 1] + p;
                    }
                }
            }

            // Precompute source LB data (Ding et al. 2016, non-blocking).
            // removal_delta == -p_src iff machine[pos_old] == machine[pos_old+1]
            // (pos_old is not the last position in its block).
            JobId shifted_job_id = data.solution.jobs[pos_old];
            const Job& shifted_job = instance.job(shifted_job_id);
            MachineId start_machine_id = data.critical_path[pos_old].start_machine_id;
            MachineId end_machine_id = data.critical_path[pos_old + 1].start_machine_id;
            // Remove contribution of the shifted job.
            Time delta = -job_contribution(data.job_prefix_sums, shifted_job_id, start_machine_id, end_machine_id);
            if (pos_old != 0) {
                JobId previous_job_id = data.solution.jobs[pos_old - 1];
                MachineId machine_id = data.critical_path[pos_old - 1].start_machine_id;
                // Remove the contribution of the previous job.
                delta -= job_contribution(data.job_prefix_sums, previous_job_id, machine_id, start_machine_id);
                // Add the new contribution of the previous job.
                delta += job_contribution(data.job_prefix_sums, previous_job_id, machine_id, end_machine_id);
            }

            // Evaluate each candidate insertion position.
            // Same-block positions (including pos_old itself) are bounded by
            // left_shift_max and right_shift_min (Grabowski & Pempera 2007).
            for (JobId pos_new = 0;
                    pos_new <= (JobId)data.solution.jobs.size() - size;
                    ++pos_new) {
                if (pos_new > data.critical_path[pos_old].left_shift_max
                        && pos_new < data.critical_path[pos_old].right_shift_min) {
                    continue;
                }

                // Ding et al. (2016) lower bound (Theorems 2 and 3).
                MachineId sm_ins = (pos_new < pos_old)?
                    data.critical_path[pos_new].start_machine_id:
                    data.critical_path[pos_new + 1].start_machine_id;
                Time lower_bound = data.solution.makespan + delta + shifted_job.operations[sm_ins].alternatives[0].processing_time;
                if (lower_bound >= makespan_new_best) {
                    continue;
                }

                if (pos_new < pos_old) {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times_0[pos_new][machine_id];
                    }
                } else {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times[pos_new][machine_id];
                    }
                }
                {
                    JobId job_id = data.solution.jobs[pos_old];
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    data.completion_times_2[0] = data.completion_times_2[0] + p0;
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        if (data.completion_times_2[machine_id] > data.completion_times_2[machine_id - 1]) {
                            data.completion_times_2[machine_id] = data.completion_times_2[machine_id] + p;
                        } else {
                            data.completion_times_2[machine_id] = data.completion_times_2[machine_id - 1] + p;
                        }
                    }
                }

                Time makespan = 0;
                const auto& reverse_completion_times = (pos_new > pos_old)?
                    data.reverse_completion_times_0:
                    data.reverse_completion_times;
                JobId p = data.solution.jobs.size() - pos_new - size;
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time completion_time = data.completion_times_2[machine_id]
                        + reverse_completion_times[p][machine_id];
                    makespan = std::max(makespan, completion_time);
                }

                if (makespan_new_best > makespan) {
                    makespan_new_best = makespan;
                    pos_new_best = pos_new;
                }
            }
        }

        if (pos_new_best != -1) {
            shift_jobs(instance, data, size, pos_old, pos_new_best);
            if (data.solution.makespan != makespan_new_best) {
                throw std::runtime_error(
                        FUNC_SIGNATURE + ": wrong makespan; "
                        "data.solution.makespan: " + std::to_string(data.solution.makespan) + "; "
                        "makespan_new_best: " + std::to_string(makespan_new_best) + ".");
            }
            improved = true;
        } else {
            ++pos_old;
        }
    }

    return improved;
}

bool explore_shift_block_neighborhood(
        const Instance& instance,
        LocalSearchData& data,
        std::mt19937_64& generator,
        JobId size,
        bool reverse = false)
{
    //std::cout << "explore_shift_block_neighborhood " << size << std::endl;
    MachineId last_machine_id = instance.number_of_machines() - 1;

    bool improved = false;

    for (JobId pos_old = 0; pos_old + size <= (JobId)data.solution.jobs.size(); ) {
        JobId pos_new_best = -1;
        Time makespan_new_best = data.solution.makespan;

        if (instance.blocking()) {
            // Compute data.completion_times.
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.completion_times[pos_old][machine_id] = data.completion_times_0[pos_old][machine_id];
            }
            for (JobId pos = pos_old + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_id = data.solution.jobs[pos - 1 + size];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[0].alternatives[0].processing_time;
                if (last_machine_id > 0) {
                    data.completion_times[pos][0] = (std::max)(
                            data.completion_times[pos - 1][0] + p0,
                            data.completion_times[pos - 1][1]);
                } else {
                    data.completion_times[pos][0] = data.completion_times[pos - 1][0] + p0;
                }
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines() - 1;
                        ++machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    data.completion_times[pos][machine_id] = (std::max)(
                            data.completion_times[pos][machine_id - 1] + p,
                            data.completion_times[pos - 1][machine_id + 1]);
                }
                if (last_machine_id > 0) {
                    Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                    data.completion_times[pos][last_machine_id] = data.completion_times[pos][last_machine_id - 1] + p;
                }
            }

            // Compute data.reverse_completion_times.
            JobId p = data.solution.jobs.size() - pos_old - size;
            if (p < 0 || p >= data.reverse_completion_times.size()) {
                throw std::logic_error(
                        FUNC_SIGNATURE + ": wrong 'p'; "
                        "p: " + std::to_string(p) + "; "
                        "data.reverse_completion_times.size(): " + std::to_string(data.reverse_completion_times.size()) + "; "
                        "pos_old: " + std::to_string(pos_old) + "; "
                        "size: " + std::to_string(size) + ".");
            }
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.reverse_completion_times[p][machine_id] = data.reverse_completion_times_0[p][machine_id];
            }
            for (JobId pos = p + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_pos = data.solution.jobs.size() - pos - size;
                if (job_pos < 0 || job_pos >= data.solution.jobs.size()) {
                    throw std::logic_error(
                            FUNC_SIGNATURE + ": wrong 'job_pos'; "
                            "job_pos: " + std::to_string(job_pos) + "; "
                            "pos: " + std::to_string(pos) + "; "
                            "data.solution.jobs.size(): " + std::to_string(data.solution.jobs.size()) +  ".");
                }
                JobId job_id = data.solution.jobs[job_pos];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[last_machine_id].alternatives[0].processing_time;
                if (last_machine_id > 0) {
                    data.reverse_completion_times[pos][last_machine_id] = (std::max)(
                            data.reverse_completion_times[pos - 1][last_machine_id] + p0,
                            data.reverse_completion_times[pos - 1][last_machine_id - 1]);
                } else {
                    data.reverse_completion_times[pos][last_machine_id] = data.reverse_completion_times[pos - 1][last_machine_id] + p0;
                }
                for (MachineId machine_id = last_machine_id - 1;
                        machine_id >= 1;
                        --machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    data.reverse_completion_times[pos][machine_id] = (std::max)(
                            data.reverse_completion_times[pos][machine_id + 1] + p,
                            data.reverse_completion_times[pos - 1][machine_id - 1]);
                }
                if (last_machine_id > 0) {
                    Time p = job.operations[0].alternatives[0].processing_time;
                    data.reverse_completion_times[pos][0] = data.reverse_completion_times[pos][1] + p;
                }
            }

            // Evaluate each candidate insertion position.
            for (JobId pos_new = 0;
                    pos_new <= (JobId)data.solution.jobs.size() - size;
                    ++pos_new) {
                if (pos_new == pos_old)
                    continue;
                if (pos_new < pos_old) {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times_0[pos_new][machine_id];
                    }
                } else {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times[pos_new][machine_id];
                    }
                }
                for (JobId i = 0; i < size; ++i) {
                    JobId pos_0 = reverse ? (pos_old + size - 1 - i) : (pos_old + i);
                    JobId job_id = data.solution.jobs[pos_0];
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    // D2[0] = max(D2[0] + p, D2[1]); D2[1] is still old at this point.
                    if (last_machine_id > 0) {
                        data.completion_times_2[0] = (std::max)(
                                data.completion_times_2[0] + p0,
                                data.completion_times_2[1]);
                    } else {
                        data.completion_times_2[0] = data.completion_times_2[0] + p0;
                    }
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines() - 1;
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        // D2[m] = max(new D2[m-1] + p, old D2[m+1])
                        data.completion_times_2[machine_id] = (std::max)(
                                data.completion_times_2[machine_id - 1] + p,
                                data.completion_times_2[machine_id + 1]);
                    }
                    if (last_machine_id > 0) {
                        Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                        data.completion_times_2[last_machine_id] = data.completion_times_2[last_machine_id - 1] + p;
                    }
                }

                Time makespan = 0;
                const auto& reverse_completion_times = (pos_new > pos_old)?
                    data.reverse_completion_times_0:
                    data.reverse_completion_times;
                JobId p = data.solution.jobs.size() - pos_new - size;
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time completion_time = data.completion_times_2[machine_id]
                        + reverse_completion_times[p][machine_id];
                    makespan = std::max(makespan, completion_time);
                }

                if (makespan_new_best > makespan) {
                    makespan_new_best = makespan;
                    pos_new_best = pos_new;
                }
            }

        } else if (instance.no_idle()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": no-idle not supported.");

        } else if (instance.no_wait()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": no-wait not supported.");

        } else {
            // Compute data.completion_times.
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.completion_times[pos_old][machine_id] = data.completion_times_0[pos_old][machine_id];
            }
            for (JobId pos = pos_old + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_id = data.solution.jobs[pos - 1 + size];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[0].alternatives[0].processing_time;
                data.completion_times[pos][0] = data.completion_times[pos - 1][0] + p0;
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    if (data.completion_times[pos - 1][machine_id] > data.completion_times[pos][machine_id - 1]) {
                        data.completion_times[pos][machine_id] = data.completion_times[pos - 1][machine_id] + p;
                    } else {
                        data.completion_times[pos][machine_id] = data.completion_times[pos][machine_id - 1] + p;
                    }
                }
            }

            // Compute data.reverse_completion_times.
            JobId p = data.solution.jobs.size() - pos_old - size;
            if (p < 0 || p >= data.reverse_completion_times.size()) {
                throw std::logic_error(
                        FUNC_SIGNATURE + ": wrong 'p'; "
                        "p: " + std::to_string(p) + "; "
                        "data.reverse_completion_times.size(): " + std::to_string(data.reverse_completion_times.size()) + "; "
                        "pos_old: " + std::to_string(pos_old) + "; "
                        "size: " + std::to_string(size) + ".");
            }
            for (MachineId machine_id = 0;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                data.reverse_completion_times[p][machine_id] = data.reverse_completion_times_0[p][machine_id];
            }
            for (JobId pos = p + 1;
                    pos <= (JobId)data.solution.jobs.size() - size;
                    ++pos) {
                JobId job_pos = data.solution.jobs.size() - pos - size;
                if (job_pos < 0 || job_pos >= data.solution.jobs.size()) {
                    throw std::logic_error(
                            FUNC_SIGNATURE + ": wrong 'job_pos'; "
                            "job_pos: " + std::to_string(job_pos) + "; "
                            "pos: " + std::to_string(pos) + "; "
                            "data.solution.jobs.size(): " + std::to_string(data.solution.jobs.size()) +  ".");
                }
                JobId job_id = data.solution.jobs[job_pos];
                const Job& job = instance.job(job_id);
                Time p0 = job.operations[last_machine_id].alternatives[0].processing_time;
                data.reverse_completion_times[pos][last_machine_id] = data.reverse_completion_times[pos - 1][last_machine_id] + p0;
                for (MachineId machine_id = last_machine_id - 1;
                        machine_id >= 0;
                        --machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    if (data.reverse_completion_times[pos - 1][machine_id] > data.reverse_completion_times[pos][machine_id + 1]) {
                        data.reverse_completion_times[pos][machine_id] = data.reverse_completion_times[pos - 1][machine_id] + p;
                    } else {
                        data.reverse_completion_times[pos][machine_id] = data.reverse_completion_times[pos][machine_id + 1] + p;
                    }
                }
            }

            // Evaluate each candidate insertion position.
            for (JobId pos_new = 0;
                    pos_new <= (JobId)data.solution.jobs.size() - size;
                    ++pos_new) {
                if (pos_new == pos_old)
                    continue;
                if (pos_new < pos_old) {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times_0[pos_new][machine_id];
                    }
                } else {
                    for (MachineId machine_id = 0;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        data.completion_times_2[machine_id] = data.completion_times[pos_new][machine_id];
                    }
                }
                for (JobId i = 0; i < size; ++i) {
                    JobId pos_0 = reverse ? (pos_old + size - 1 - i) : (pos_old + i);
                    JobId job_id = data.solution.jobs[pos_0];
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    data.completion_times_2[0] = data.completion_times_2[0] + p0;
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        if (data.completion_times_2[machine_id] > data.completion_times_2[machine_id - 1]) {
                            data.completion_times_2[machine_id] = data.completion_times_2[machine_id] + p;
                        } else {
                            data.completion_times_2[machine_id] = data.completion_times_2[machine_id - 1] + p;
                        }
                    }
                }

                Time makespan = 0;
                const auto& reverse_completion_times = (pos_new > pos_old)?
                    data.reverse_completion_times_0:
                    data.reverse_completion_times;
                JobId p = data.solution.jobs.size() - pos_new - size;
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time completion_time = data.completion_times_2[machine_id]
                        + reverse_completion_times[p][machine_id];
                    makespan = std::max(makespan, completion_time);
                }
                if (makespan_new_best > makespan) {
                    makespan_new_best = makespan;
                    pos_new_best = pos_new;
                }
            }
        }

        if (pos_new_best != -1) {
            shift_jobs(instance, data, size, pos_old, pos_new_best, reverse);
            if (data.solution.makespan != makespan_new_best) {
                throw std::runtime_error(
                        FUNC_SIGNATURE + ": wrong makespan; "
                        "data.solution.makespan: " + std::to_string(data.solution.makespan) + "; "
                        "makespan_new_best: " + std::to_string(makespan_new_best) + ".");
            }
            improved = true;
        } else {
            ++pos_old;
        }
    }

    return improved;
}

bool explore_swap_neighborhood(
        const Instance& instance,
        LocalSearchData& data,
        std::mt19937_64& generator)
{
    //std::cout << "explore_swap_neighborhood" << std::endl;
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId n = data.solution.jobs.size();

    bool improved = false;

    for (JobId pos_1 = 0; pos_1 < n - 1; ) {

        bool applied = false;

        if (instance.blocking()) {

            JobId job_1 = data.solution.jobs[pos_1];
            MachineId cp_first_1 = data.critical_path[pos_1].start_machine_id;
            MachineId cp_last_1 = data.critical_path[pos_1 + 1].start_machine_id;
            Time cp_sum_1 = job_contribution(data.job_prefix_sums, job_1, cp_first_1, cp_last_1);

            for (JobId pos_2 = pos_1 + 1; pos_2 < n; ++pos_2) {

                if (pos_2 < data.critical_path[pos_1].right_shift_min)
                    continue;

                JobId job_2 = data.solution.jobs[pos_2];
                MachineId cp_first_2 = data.critical_path[pos_2].start_machine_id;
                MachineId cp_last_2 = data.critical_path[pos_2 + 1].start_machine_id;
                Time cp_sum_2 = job_contribution(data.job_prefix_sums, job_2, cp_first_2, cp_last_2);
                Time new_sum_1 = job_contribution(data.job_prefix_sums, job_2, cp_first_1, cp_last_1);
                Time new_sum_2 = job_contribution(data.job_prefix_sums, job_1, cp_first_2, cp_last_2);
                if (new_sum_1 + new_sum_2 >= cp_sum_1 + cp_sum_2)
                    continue;

                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    data.completion_times_2[machine_id] = data.completion_times_0[pos_1][machine_id];
                }

                // Apply swapped subsequence: job[pos_2], jobs[pos_1+1..pos_2-1], job[pos_1].
                JobId tail = n - pos_2 - 1;
                Time makespan = 0;
                for (JobId pos_0 = pos_1; pos_0 <= pos_2; ++pos_0) {
                    JobId job_id = data.solution.jobs[
                        pos_0 == pos_1?
                        pos_2:
                        (pos_0 < pos_2? pos_0: pos_1)];
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    if (last_machine_id > 0) {
                        data.completion_times_2[0] = (std::max)(
                                data.completion_times_2[0] + p0,
                                data.completion_times_2[1]);
                    } else {
                        data.completion_times_2[0] += p0;
                    }
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines() - 1;
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        data.completion_times_2[machine_id] = (std::max)(
                                data.completion_times_2[machine_id - 1] + p,
                                data.completion_times_2[machine_id + 1]);
                    }
                    if (last_machine_id > 0) {
                        Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                        data.completion_times_2[last_machine_id] =
                                data.completion_times_2[last_machine_id - 1] + p;
                    }
                    if (data.completion_times_2[last_machine_id] >= data.solution.makespan)
                        goto next_pos_2_b;
                }
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time ct = data.completion_times_2[machine_id]
                        + data.reverse_completion_times_0[tail][machine_id];
                    if (ct > makespan)
                        makespan = ct;
                    if (makespan >= data.solution.makespan)
                        goto next_pos_2_b;
                }
                if (data.solution.makespan > makespan) {
                    swap_jobs(instance, data, pos_1, pos_2);
                    if (data.solution.makespan != makespan) {
                        throw std::runtime_error(
                                FUNC_SIGNATURE + ": wrong makespan; "
                                "data.solution.makespan: " + std::to_string(data.solution.makespan) + "; "
                                "makespan: " + std::to_string(makespan) + ".");
                    }
                    job_1 = data.solution.jobs[pos_1];
                    cp_first_1 = data.critical_path[pos_1].start_machine_id;
                    cp_last_1 = data.critical_path[pos_1 + 1].start_machine_id;
                    cp_sum_1 = job_contribution(data.job_prefix_sums, job_1, cp_first_1, cp_last_1);
                    applied = true;
                    improved = true;
                }
                next_pos_2_b:;
            }

        } else if (instance.no_idle()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": no-idle not supported.");
        } else if (instance.no_wait()) {
            throw std::invalid_argument(
                    FUNC_SIGNATURE + ": no-wait not supported.");
        } else {

            JobId job_1 = data.solution.jobs[pos_1];
            MachineId cp_first_1 = data.critical_path[pos_1].start_machine_id;
            MachineId cp_last_1 = data.critical_path[pos_1 + 1].start_machine_id;
            Time cp_sum_1 = job_contribution(data.job_prefix_sums, job_1, cp_first_1, cp_last_1);

            for (JobId pos_2 = pos_1 + 1; pos_2 < n; ++pos_2) {

                if (pos_2 < data.critical_path[pos_1].right_shift_min)
                    continue;

                JobId job_2 = data.solution.jobs[pos_2];
                MachineId cp_first_2 = data.critical_path[pos_2].start_machine_id;
                MachineId cp_last_2 = data.critical_path[pos_2 + 1].start_machine_id;
                Time cp_sum_2 = job_contribution(data.job_prefix_sums, job_2, cp_first_2, cp_last_2);
                Time new_sum_1 = job_contribution(data.job_prefix_sums, job_2, cp_first_1, cp_last_1);
                Time new_sum_2 = job_contribution(data.job_prefix_sums, job_1, cp_first_2, cp_last_2);
                if (new_sum_1 + new_sum_2 >= cp_sum_1 + cp_sum_2)
                    continue;

                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    data.completion_times_2[machine_id] = data.completion_times_0[pos_1][machine_id];
                }

                // Apply swapped subsequence: job[pos_2], jobs[pos_1+1..pos_2-1], job[pos_1].
                JobId tail = n - pos_2 - 1;
                Time makespan = 0;
                for (JobId pos_0 = pos_1; pos_0 <= pos_2; ++pos_0) {
                    JobId job_id = data.solution.jobs[
                        pos_0 == pos_1?
                        pos_2:
                        (pos_0 < pos_2? pos_0: pos_1)];
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].alternatives[0].processing_time;
                    data.completion_times_2[0] += p0;
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        Time p = job.operations[machine_id].alternatives[0].processing_time;
                        if (data.completion_times_2[machine_id]
                                > data.completion_times_2[machine_id - 1]) {
                            data.completion_times_2[machine_id] += p;
                        } else {
                            data.completion_times_2[machine_id] =
                                    data.completion_times_2[machine_id - 1] + p;
                        }
                    }
                    if (data.completion_times_2[last_machine_id] >= data.solution.makespan)
                        goto next_pos_2;
                }
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time ct = data.completion_times_2[machine_id]
                        + data.reverse_completion_times_0[tail][machine_id];
                    if (ct > makespan)
                        makespan = ct;
                    if (makespan >= data.solution.makespan)
                        goto next_pos_2;
                }
                if (data.solution.makespan == makespan) {
                    for (JobId k = pos_2 + 1; k < n; ++k) {
                        JobId job_id = data.solution.jobs[k];
                        const Job& job = instance.job(job_id);
                        data.completion_times_2[0] +=
                            job.operations[0].alternatives[0].processing_time;
                        for (MachineId machine_id = 1;
                                machine_id < instance.number_of_machines();
                                ++machine_id) {
                            Time p = job.operations[machine_id].alternatives[0].processing_time;
                            if (data.completion_times_2[machine_id]
                                    > data.completion_times_2[machine_id - 1]) {
                                data.completion_times_2[machine_id] += p;
                            } else {
                                data.completion_times_2[machine_id] =
                                    data.completion_times_2[machine_id - 1] + p;
                            }
                        }
                    }
                }
                if (data.solution.makespan > makespan) {
                    swap_jobs(instance, data, pos_1, pos_2);
                    if (data.solution.makespan != makespan) {
                        throw std::runtime_error(
                                FUNC_SIGNATURE + ": wrong makespan; "
                                "data.solution.makespan: " + std::to_string(data.solution.makespan) + "; "
                                "makespan: " + std::to_string(makespan) + ".");
                    }
                    job_1 = data.solution.jobs[pos_1];
                    cp_first_1 = data.critical_path[pos_1].start_machine_id;
                    cp_last_1 = data.critical_path[pos_1 + 1].start_machine_id;
                    cp_sum_1 = job_contribution(data.job_prefix_sums, job_1, cp_first_1, cp_last_1);
                    applied = true;
                    improved = true;
                }
                next_pos_2:;
            }
        }

        if (!applied)
            ++pos_1;
    }

    return improved;
}

void local_search(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data)
{
    std::vector<Neighborhood> neighborhoods = {
        Neighborhood::Shift1,
    };
    if (instance.number_of_jobs() < 256) {
        neighborhoods.push_back(Neighborhood::Shift2);
        neighborhoods.push_back(Neighborhood::Shift3);
        neighborhoods.push_back(Neighborhood::Shift4);
        neighborhoods.push_back(Neighborhood::Swap);
    }
    for (;;) {
        //std::cout << "makespan " << data.solution.makespan << std::endl;
        //for (JobId job_id: data.solution.jobs)
        //    std::cout << " " << job_id;
        //std::cout << std::endl;

        bool improved = false;
        for (Neighborhood neighborhood: neighborhoods) {
            switch (neighborhood) {
            case Neighborhood::Shift1: {
                improved = explore_shift_job_neighborhood(instance, data, generator);
                break;
            } case Neighborhood::Shift2: {
                improved = explore_shift_block_neighborhood(instance, data, generator, 2);
                break;
            } case Neighborhood::Shift3: {
                improved = explore_shift_block_neighborhood(instance, data, generator, 3);
                break;
            } case Neighborhood::Shift4: {
                improved = explore_shift_block_neighborhood(instance, data, generator, 4);
                break;
            } case Neighborhood::ShiftReverse2: {
                improved = explore_shift_block_neighborhood(instance, data, generator, 2, true);
                break;
            } case Neighborhood::ShiftReverse3: {
                improved = explore_shift_block_neighborhood(instance, data, generator, 3, true);
                break;
            } case Neighborhood::ShiftReverse4: {
                improved = explore_shift_block_neighborhood(instance, data, generator, 4, true);
                break;
            } case Neighborhood::Swap: {
                improved = explore_swap_neighborhood(instance, data, generator);
                break;
            }
            }
            if (improved)
                break;
        }
        if (!improved)
            break;
    }

    // Update best solution.
    if (data.solution.jobs.size() == instance.number_of_jobs()
            && (!output.solution.feasible()
                || output.solution.makespan() > data.solution.makespan)) {
        std::stringstream ss;
        ss << "it " << output.number_of_iterations;
        // Build solution.
        Solution solution = build_solution(instance, data.solution);
        algorithm_formatter.update_solution(solution, ss.str());
    }
}

// Forward declaration — defined later in this file.
void add_job_at_best_position(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        JobId job_id,
        JobId forbidden_position = -1)
{
    const Job& job = instance.job(job_id);

    std::vector<JobId> best_positions;
    Time makespan_best = 0;  // stores best+1 when best_positions is non-empty

    for (JobId pos = 0; pos <= (JobId)data.solution.jobs.size(); ++pos) {

        if (pos == forbidden_position)
            continue;

        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            data.completion_times_2[machine_id] = data.completion_times_0[pos][machine_id];
        }

        if (instance.blocking()) {
            MachineId last_machine_id = instance.number_of_machines() - 1;
            Time p0 = job.operations[0].alternatives[0].processing_time;
            if (last_machine_id > 0) {
                data.completion_times_2[0] = (std::max)(
                        data.completion_times_2[0] + p0,
                        data.completion_times_2[1]);
            } else {
                data.completion_times_2[0] += p0;
            }
            for (MachineId machine_id = 1;
                    machine_id < instance.number_of_machines() - 1;
                    ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                data.completion_times_2[machine_id] = (std::max)(
                        data.completion_times_2[machine_id - 1] + p,
                        data.completion_times_2[machine_id + 1]);
            }
            if (last_machine_id > 0) {
                Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                data.completion_times_2[last_machine_id] =
                        data.completion_times_2[last_machine_id - 1] + p;
            }
        } else {
            Time p0 = job.operations[0].alternatives[0].processing_time;
            data.completion_times_2[0] = data.completion_times_2[0] + p0;
            for (MachineId machine_id = 1;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                if (data.completion_times_2[machine_id] > data.completion_times_2[machine_id - 1]) {
                    data.completion_times_2[machine_id] = data.completion_times_2[machine_id] + p;
                } else {
                    data.completion_times_2[machine_id] = data.completion_times_2[machine_id - 1] + p;
                }
            }
        }

        Time makespan = 0;
        // pos = solution.jobs.size() => p = 0
        // pos = solution.jobs.size() - 1 => p = 1
        JobId p = data.solution.jobs.size() - pos;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            makespan = std::max(makespan,
                    data.completion_times_2[machine_id]
                    + data.reverse_completion_times_0[p][machine_id]);
        }

        if (makespan + 1 < makespan_best)
            best_positions.clear();
        if (!best_positions.empty() && makespan >= makespan_best)
            continue;
        makespan_best = makespan + 1;
        best_positions.push_back(pos);
    }

    if (best_positions.empty())
        throw std::runtime_error(FUNC_SIGNATURE + ": best_positions is empty.");

    JobId pos_best = best_positions[0];
    add_job(instance, data, job_id, pos_best);
}

void add_block_at_best_position(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        const std::vector<JobId>& job_ids)
{
    std::vector<JobId> best_positions;
    Time makespan_best = 0;

    for (JobId pos = 0; pos <= (JobId)data.solution.jobs.size(); ++pos) {

        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            data.completion_times_2[machine_id] = data.completion_times_0[pos][machine_id];
        }

        for (JobId job_id: job_ids) {
            const Job& job = instance.job(job_id);
            if (instance.blocking()) {
                MachineId last_machine_id = instance.number_of_machines() - 1;
                Time p0 = job.operations[0].alternatives[0].processing_time;
                if (last_machine_id > 0) {
                    data.completion_times_2[0] = (std::max)(
                            data.completion_times_2[0] + p0,
                            data.completion_times_2[1]);
                } else {
                    data.completion_times_2[0] += p0;
                }
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines() - 1;
                        ++machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    data.completion_times_2[machine_id] = (std::max)(
                            data.completion_times_2[machine_id - 1] + p,
                            data.completion_times_2[machine_id + 1]);
                }
                if (last_machine_id > 0) {
                    Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                    data.completion_times_2[last_machine_id] =
                            data.completion_times_2[last_machine_id - 1] + p;
                }
            } else {
                Time p0 = job.operations[0].alternatives[0].processing_time;
                data.completion_times_2[0] = data.completion_times_2[0] + p0;
                for (MachineId machine_id = 1;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    Time p = job.operations[machine_id].alternatives[0].processing_time;
                    if (data.completion_times_2[machine_id] > data.completion_times_2[machine_id - 1]) {
                        data.completion_times_2[machine_id] = data.completion_times_2[machine_id] + p;
                    } else {
                        data.completion_times_2[machine_id] = data.completion_times_2[machine_id - 1] + p;
                    }
                }
            }
        }

        Time makespan = 0;
        // pos = solution.jobs.size() => p = 0
        // pos = solution.jobs.size() - 1 => p = 1
        JobId p = data.solution.jobs.size() - pos;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            makespan = std::max(makespan,
                    data.completion_times_2[machine_id]
                    + data.reverse_completion_times_0[p][machine_id]);
        }

        if (makespan + 1 < makespan_best)
            best_positions.clear();
        if (!best_positions.empty() && makespan >= makespan_best)
            continue;
        makespan_best = makespan + 1;
        best_positions.push_back(pos);
    }

    if (best_positions.empty())
        throw std::runtime_error(FUNC_SIGNATURE + ": best_positions is empty.");

    JobId pos_best = best_positions[
        std::uniform_int_distribution<size_t>(0, best_positions.size() - 1)(generator)];
    add_block(instance, data, job_ids, pos_best);
}

// Among unscheduled jobs (jobs_positions == -1), append the one minimizing
// the sum of departure times across all machines (PF construction, Eq. 4-5).
void append_best_job(
        const Instance& instance,
        LocalSearchData& data)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId n = data.solution.jobs.size();
    JobId best_job_id = -1;
    Time best_t = -1;

    //std::cout << "p";
    //for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
    //    std::cout << " " << instance.job(data.solution.jobs.back()).operations[machine_id].alternatives[0].processing_time;
    //std::cout << "c";
    //for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
    //    std::cout << " " << data.completion_times_0[n][machine_id];
    //std::cout << std::endl;
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        if (data.solution.jobs_positions[job_id] != -1)
            continue;

        const Job& job = instance.job(job_id);

        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
            data.completion_times_2[machine_id] = data.completion_times_0[n][machine_id];

        if (instance.blocking()) {
            Time p0 = job.operations[0].alternatives[0].processing_time;
            if (last_machine_id > 0) {
                data.completion_times_2[0] = (std::max)(
                        data.completion_times_2[0] + p0,
                        data.completion_times_2[1]);
            } else {
                data.completion_times_2[0] += p0;
            }
            for (MachineId machine_id = 1; machine_id < instance.number_of_machines() - 1; ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                data.completion_times_2[machine_id] = (std::max)(
                        data.completion_times_2[machine_id - 1] + p,
                        data.completion_times_2[machine_id + 1]);
            }
            if (last_machine_id > 0) {
                Time p = job.operations[last_machine_id].alternatives[0].processing_time;
                data.completion_times_2[last_machine_id] = data.completion_times_2[last_machine_id - 1] + p;
            }
        } else {
            Time p0 = job.operations[0].alternatives[0].processing_time;
            data.completion_times_2[0] += p0;
            for (MachineId machine_id = 1; machine_id < instance.number_of_machines(); ++machine_id) {
                Time p = job.operations[machine_id].alternatives[0].processing_time;
                data.completion_times_2[machine_id] = (std::max)(
                        data.completion_times_2[machine_id],
                        data.completion_times_2[machine_id - 1]) + p;
            }
        }

        Time t = 0;
        for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id) {
            t += data.completion_times_2[machine_id]
                - job.operations[machine_id].alternatives[0].processing_time
                - data.completion_times_0[n][machine_id];
        }

        if (best_job_id == -1 || t < best_t) {
            best_job_id = job_id;
            best_t = t;
        }
    }

    //std::cout << "add_job " << best_job_id << " t " << best_t << std::endl;
    add_job(instance, data, best_job_id, n);
}

JobId remove_random_job(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        LocalSearchData& data)
{
    std::uniform_int_distribution<JobId> d_pos(0, data.solution.jobs.size() - 1);
    JobId pos = d_pos(generator);
    JobId job_id = data.solution.jobs[pos];
    remove_job(instance, data, pos);
    //Solution solution = build_solution(instance, data.solution);
    return job_id;
}

std::vector<JobId> remove_random_jobs(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        LocalSearchData& data,
        JobId number_of_jobs_removed)
{
    std::vector<JobId> positions = optimizationtools::bob_floyd(
            number_of_jobs_removed,
            (JobId)data.solution.jobs.size(),
            generator);
    std::vector<JobId> remove_job_ids;
    for (JobId pos: positions) {
        if (pos >= data.solution.jobs.size()) {
            throw std::logic_error(
                    FUNC_SIGNATURE + ": wrong pos; "
                    "pos: " + std::to_string(pos) + "; "
                    "data.solution.jobs.size(): " + std::to_string(data.solution.jobs.size()) + ".");
        }
        remove_job_ids.push_back(data.solution.jobs[pos]);
    }
    remove_jobs(instance, data, positions);
    //Solution solution = build_solution(instance, data.solution);
    return remove_job_ids;
}

std::vector<JobId> remove_random_block(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        LocalSearchData& data)
{
    JobId max_size = std::min((JobId)4, (JobId)data.solution.jobs.size());
    std::uniform_int_distribution<JobId> d_size(1, max_size);
    JobId size = d_size(generator);
    std::uniform_int_distribution<JobId> d_pos(0, (JobId)data.solution.jobs.size() - size);
    JobId pos = d_pos(generator);
    std::vector<JobId> removed_jobs_ids(
            data.solution.jobs.begin() + pos,
            data.solution.jobs.begin() + pos + size);
    remove_block(instance, data, pos, size);
    //Solution solution = build_solution(instance, data.solution);
    return removed_jobs_ids;
}

JobId remove_worst_job(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        LocalSearchData& data)
{
    Time p_total = 0;
    for (JobId pos = 0; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        const Job& job = instance.job(job_id);
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            p_total += job.operations[machine_id].alternatives[0].processing_time;
        }
    }

    JobId pos_best = -1;
    double value_best = (double)p_total / data.solution.makespan * instance.number_of_machines();
    Time makespan_best = data.solution.makespan;
    for (JobId pos = 0; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        const Job& job = instance.job(job_id);
        Time p_cur = p_total;
        Time makespan = 0;
        // pos = data.solution.jobs.size() - 1 => p = 0
        // pos = data.solution.jobs.size() - 2 => p = 1
        JobId p = data.solution.jobs.size() - pos - 1;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            p_cur -= job.operations[machine_id].alternatives[0].processing_time;
            makespan = std::max(makespan,
                    data.completion_times_0[pos][machine_id]
                    + data.reverse_completion_times_0[p][machine_id]);
        }

        double value = (double)p_cur / (makespan * instance.number_of_machines());
        if (pos_best == -1
                || value_best < value) {
            pos_best = pos;
            value_best = value;
            makespan_best = makespan;
        }
    }

    // Apply move.
    JobId job_id = data.solution.jobs[pos_best];
    std::vector<JobId> positions = {pos_best};
    remove_jobs(instance, data, positions);
    //Solution solution = build_solution(instance, data.solution);
    return job_id;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Initial solutions ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void generate_initial_solution_neh(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data)
{
    // Clear the solution.
    data.solution.jobs.clear();
    std::fill(
            data.solution.jobs_positions.begin(),
            data.solution.jobs_positions.end(),
            -1);
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        data.completion_times_0[0][machine_id] = 0;
    }
    data.solution.makespan = 0;

    // Sort jobs by total processing time (descending).
    std::vector<JobId> sorted_jobs(instance.number_of_jobs());
    std::iota(sorted_jobs.begin(), sorted_jobs.end(), 0);
    std::sort(
            sorted_jobs.begin(),
            sorted_jobs.end(),
            [&instance](JobId job_1_id, JobId job_2_id) -> bool
            {
                const Job& job_1 = instance.job(job_1_id);
                const Job& job_2 = instance.job(job_2_id);
                return job_1.mean_processing_time > job_2.mean_processing_time;
            });
    for (JobId job_id: sorted_jobs) {
        add_job_at_best_position(instance, parameters, generator, data, job_id);
        local_search(instance, parameters, generator, output, algorithm_formatter, data);
    }
}

void generate_initial_solution_pf_neh(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data)
{
    // Clear the solution.
    data.solution.jobs.clear();
    std::fill(
            data.solution.jobs_positions.begin(),
            data.solution.jobs_positions.end(),
            -1);
    for (MachineId machine_id = 0; machine_id < instance.number_of_machines(); ++machine_id)
        data.completion_times_0[0][machine_id] = 0;
    data.solution.makespan = 0;

    // Pick a random first job for diversity.
    std::uniform_int_distribution<JobId> d_first(0, instance.number_of_jobs() - 1);
    JobId first_job = d_first(generator);
    //first_job = 315;
    //std::cout << "first_job " << first_job << std::endl;
    add_job(instance, data, first_job, 0);

    // PF construction: repeatedly append the job minimizing the sum of
    // departure times across all machines (Eq. 4-5).
    while ((JobId)data.solution.jobs.size() < instance.number_of_jobs())
        append_best_job(instance, data);

    // Remove the last a = 4 jobs.
    //static const JobId a = 4;
    const JobId a = instance.number_of_jobs() > 20? 25: 21;
    std::vector<JobId> removed_jobs;
    for (JobId i = 0; i < a && !data.solution.jobs.empty(); ++i) {
        JobId pos = (JobId)data.solution.jobs.size() - 1;
        removed_jobs.push_back(data.solution.jobs[pos]);
        remove_job(instance, data, pos);
    }

    // Reinsert in random order, each at the position minimizing makespan.
    std::shuffle(removed_jobs.begin(), removed_jobs.end(), generator);
    for (JobId job_id: removed_jobs)
        add_job_at_best_position(instance, parameters, generator, data, job_id);
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Perturbations /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

enum class Perturbation
{
    RandomAdjacentSwaps,
    RandomShifts,
    RuinAndRecreate1,
    RuinAndRecreateJobs,
    RuinAndRecreateBlock,
};

void random_shifts(
        const Instance& instance,
        const LocalSearchParameters& parameters,
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
        remove_job(instance, data, forbidden_position);
        add_job_at_best_position(instance, parameters, generator, data, job_id, forbidden_position);
    }
}

// Perturbation: d random adjacent swaps (IARAS from Fernandez-Viagas et al. 2018).
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
    update_data(instance, data);
}

void ruin_and_recreate_1(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        LocalSearchData& data,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        AlgorithmFormatter& algorithm_formatter)
{
    JobId removed_job_id = remove_random_job(instance, parameters, generator, output, data);
    local_search(instance, parameters, generator, output, algorithm_formatter, data);
    add_job_at_best_position(instance, parameters, generator, data, removed_job_id);
}

void ruin_and_recreate_jobs(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        LocalSearchData& data,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        JobId d = 4)
{
    std::vector<JobId> removed = remove_random_jobs(instance, parameters, generator, output, data, 4);
    local_search(instance, parameters, generator, output, algorithm_formatter, data);
    for (JobId job_id: removed)
        add_job_at_best_position(instance, parameters, generator, data, job_id);
}

void ruin_and_recreate_block(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        LocalSearchData& data,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        JobId d = 4)
{
    std::vector<JobId> removed_jobs_ids = remove_random_block(instance, parameters, generator, output, data);
    local_search(instance, parameters, generator, output, algorithm_formatter, data);
    add_block_at_best_position(instance, parameters, generator, data, removed_jobs_ids);
}

}

const LocalSearchOutput shopschedulingsolver::local_search_pfss_makespan(
        const Instance& instance,
        std::mt19937_64& generator,
        Solution* initial_solution,
        const LocalSearchParameters& parameters)
{
    LocalSearchOutput output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Local search");

    if (parameters.timer.needs_to_end()) {
        algorithm_formatter.end();
        return output;
    }

    algorithm_formatter.print_header();

    // Initialize data.
    LocalSearchData data;
    data.completion_times_0 = std::vector<std::vector<Time>>(
            instance.number_of_jobs() + 1,
            std::vector<Time>(instance.number_of_machines(), 0));
    data.reverse_completion_times_0 = std::vector<std::vector<Time>>(
            instance.number_of_jobs() + 1,
            std::vector<Time>(instance.number_of_machines(), 0));
    data.completion_times = std::vector<std::vector<Time>>(
            instance.number_of_jobs() + 1,
            std::vector<Time>(instance.number_of_machines(), 0));
    data.reverse_completion_times = std::vector<std::vector<Time>>(
            instance.number_of_jobs() + 1,
            std::vector<Time>(instance.number_of_machines(), 0));
    data.completion_times_2 = std::vector<Time>(instance.number_of_machines(), 0);
    data.critical_path = std::vector<CriticalJob>(instance.number_of_jobs() + 1);
    data.job_prefix_sums = std::vector<std::vector<Time>>(
            instance.number_of_jobs(),
            std::vector<Time>(instance.number_of_machines() + 1, 0));
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        for (MachineId k = 0; k < instance.number_of_machines(); ++k) {
            data.job_prefix_sums[job_id][k + 1] = data.job_prefix_sums[job_id][k]
                + job.operations[k].alternatives[0].processing_time;
        }
    }
    data.solution.jobs_positions = std::vector<JobId>(instance.number_of_jobs(), -1);

    // Initialize population.
    localsearchsolver::PenalizedCostCallback<LocalSearchSolution, Time> penalized_cost_callback = [](
                const LocalSearchSolution& solution)
            {
                return solution.makespan;
            };
    localsearchsolver::DistanceCallback<LocalSearchSolution> distance_callback = [&instance](
                const LocalSearchSolution& solution_1,
                const LocalSearchSolution& solution_2)
            {
                localsearchsolver::Distance distance = 0;
                for (JobId job_id = 0;
                        job_id < instance.number_of_jobs();
                        ++job_id) {
                    JobId job_pos_1 = solution_1.jobs_positions[job_id];
                    JobId job_pos_2 = solution_2.jobs_positions[job_id];
                    if (job_pos_1 == 0) {
                        if (job_pos_2 != 0)
                            distance++;
                    } else if (job_pos_2 == 0) {
                        distance++;
                    } else {
                        JobId job_prev_1_id = solution_1.jobs[job_pos_1 - 1];
                        JobId job_prev_2_id = solution_2.jobs[job_pos_2 - 1];
                        if (job_prev_1_id != job_prev_2_id)
                            distance++;
                    }
                    JobId last_job_pos = solution_1.jobs.size() - 1;
                    if (job_pos_1 == last_job_pos) {
                        if (job_pos_2 != last_job_pos)
                            distance++;
                    } else if (job_pos_2 == last_job_pos) {
                        distance++;
                    } else {
                        JobId job_next_1_id = solution_1.jobs[job_pos_1 + 1];
                        JobId job_next_2_id = solution_2.jobs[job_pos_2 + 1];
                        if (job_next_1_id != job_next_2_id)
                            distance++;
                    }
                }
                return distance;
            };
    localsearchsolver::Population<LocalSearchSolution, Time>::Parameters population_parameters;
    population_parameters.minimum_size = 20;
    population_parameters.maximum_size = 40;
    population_parameters.number_of_elite_solutions = 10;
    population_parameters.number_of_closest_neighbors = 3;
    localsearchsolver::Population<LocalSearchSolution, Time> population(
            penalized_cost_callback,
            distance_callback,
            population_parameters);

    if (initial_solution != nullptr) {
        load_solution(data, *initial_solution);
        local_search(instance, parameters, generator, output, algorithm_formatter, data);
    } else {
        // Seed the population with PF-NEH(v=1) solutions (Section 4.1).
        for (JobId i = 0;
                i < population_parameters.minimum_size
                        && !parameters.timer.needs_to_end();
                ++i) {
            generate_initial_solution_pf_neh(instance, parameters, generator, data);
            //std::cout << "[construction] " << data.solution.makespan << std::endl;
            local_search(instance, parameters, generator, output, algorithm_formatter, data);
            //std::cout << "[local search] " << data.solution.makespan << std::endl;
            population.add(data.solution, generator);
        }
    }

    std::vector<Perturbation> perturbations = {
        Perturbation::RandomShifts,
        Perturbation::RuinAndRecreateJobs,
    };
    std::vector<Counter> perturbation_successes(perturbations.size(), 0);

    for (output.number_of_iterations = 1;
            !parameters.timer.needs_to_end();
            ++output.number_of_iterations) {

        // Check stop criteria.
        if (parameters.timer.needs_to_end())
            break;
        //if (parameters.goal != -1
        //        && output.solution.makespan() >= parameters.goal)
        //    break;
        if (output.solution.feasible()
                && output.solution.makespan() == output.makespan_bound) {
            break;
        }

        data.solution = population.binary_tournament_single(generator);
        update_data(instance, data);
        Time makespan_before = data.solution.makespan;

        // Draw the perturbation to use.
        std::vector<double> weights;
        for (Counter perturbation_pos = 0;
                perturbation_pos < (Counter)perturbations.size();
                ++perturbation_pos) {
            weights.push_back(2.0 + perturbation_successes[perturbation_pos]);
        }
        std::discrete_distribution<Counter> distribution_perturbation(weights.begin(), weights.end());
        Counter perturbation_pos = distribution_perturbation(generator);
        Perturbation perturbation = perturbations[perturbation_pos];

        switch (perturbation) {
        case Perturbation::RandomAdjacentSwaps:
            random_adjacent_swaps(instance, data, generator, 4);
            break;
        case Perturbation::RandomShifts:
            random_shifts(instance, parameters, generator, data, 4);
            break;
        case Perturbation::RuinAndRecreate1:
            ruin_and_recreate_1(instance, parameters, data, generator, output, algorithm_formatter);
            break;
        case Perturbation::RuinAndRecreateJobs:
            ruin_and_recreate_jobs(instance, parameters, data, generator, output, algorithm_formatter, 4);
            break;
        case Perturbation::RuinAndRecreateBlock:
            ruin_and_recreate_block(instance, parameters, data, generator, output, algorithm_formatter, 4);
            break;
        }

        local_search(instance, parameters, generator, output, algorithm_formatter, data);

        if (data.solution.makespan < makespan_before)
            perturbation_successes[perturbation_pos]++;

        population.add(data.solution, generator);

        //double mean_makespan = 0;
        //for (Counter solution_id = 0;
        //        solution_id < population.size();
        //        ++solution_id) {
        //    const auto& population_solution = population.solution(solution_id);
        //    mean_makespan += population_solution.solution.makespan;
        //    std::cout << "solution " << solution_id
        //        << " makespan " << population_solution.solution.makespan
        //        << " / " << output.solution.makespan()
        //        << std::endl;
        //}
        //mean_makespan /= population.size();
        //std::cout << "mean_makespan " << mean_makespan << std::endl;
    }

    algorithm_formatter.end();
    return output;
}
