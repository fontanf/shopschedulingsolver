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

struct LocalSearchData
{
    LocalSearchSolution solution;

    std::vector<JobId> shuffled_positions_1;
    std::vector<JobId> shuffled_positions_2;

    std::vector<std::vector<Time>> completion_times_0;
    std::vector<std::vector<Time>> reverse_completion_times_0;

    std::vector<std::vector<Time>> completion_times;
    std::vector<std::vector<Time>> reverse_completion_times;

    std::vector<Time> completion_times_2;
    std::vector<Time> makespans;
};

void update_completion_times(
        const Instance& instance,
        LocalSearchData& data,
        JobId p)
{
    //std::cout << "update_completion_times p " << p << std::endl;
    for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos - 1];
        const Job& job = instance.job(job_id);
        Time p0 = job.operations[0].machines[0].processing_time;
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
            Time p = job.operations[machine_id].machines[0].processing_time;
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

void update_reverse_completion_times(
        const Instance& instance,
        LocalSearchData& data,
        JobId p)
{
    MachineId last_machine_id = instance.number_of_machines() - 1;
    JobId last_job_pos = data.solution.jobs.size() - 1;
    for (JobId pos = p + 1; pos <= (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[last_job_pos - pos + 1];
        const Job& job = instance.job(job_id);
        Time p0 = job.operations[last_machine_id].machines[0].processing_time;
        data.reverse_completion_times_0[pos][last_machine_id] = data.reverse_completion_times_0[pos - 1][last_machine_id] + p0;
        for (MachineId machine_id = last_machine_id - 1;
                machine_id >= 0;
                --machine_id) {
            Time p = job.operations[machine_id].machines[0].processing_time;
            if (data.reverse_completion_times_0[pos - 1][machine_id] > data.reverse_completion_times_0[pos][machine_id + 1]) {
                data.reverse_completion_times_0[pos][machine_id] = data.reverse_completion_times_0[pos - 1][machine_id] + p;
            } else {
                data.reverse_completion_times_0[pos][machine_id] = data.reverse_completion_times_0[pos][machine_id + 1] + p;
            }
        }
    }
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
    // Compute completion_times.
    update_completion_times(instance, data, 0);
    // Compute reverse_completion_times.
    update_reverse_completion_times(instance, data, 0);
    // Update makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
};

void add_job(
        const Instance& instance,
        LocalSearchData& data,
        JobId job_id,
        JobId pos_new)
{
    // pos_new == data.solution.jobs.size() => p = 0
    // pos_new == data.solution.jobs.size() - 1 => p = 1
    JobId p = data.solution.jobs.size() - pos_new;
    // Update data.solution.jobs.
    data.solution.jobs.insert(data.solution.jobs.begin() + pos_new, job_id);
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    // Update data.completion_times_0.
    update_completion_times(instance, data, pos_new);
    // Update data.reverse_completion_times_0.
    update_reverse_completion_times(instance, data, p);
    // Update data.solution.makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
}

void add_block(
        const Instance& instance,
        LocalSearchData& data,
        const std::vector<JobId>& job_ids,
        JobId pos_new)
{
    // pos_new == data.solution.jobs.size() => p = 0
    // pos_new == data.solution.jobs.size() - 1 => p = 1
    JobId p = data.solution.jobs.size() - pos_new;
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
    // Update data.completion_times_0.
    update_completion_times(instance, data, pos_new);
    // Update data.reverse_completion_times_0.
    update_reverse_completion_times(instance, data, p);
    // Update data.solution.makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
}

void remove_job(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos_new)
{
    // pos_max == data.solution.jobs.size() - 1 => p = 0
    JobId p = data.solution.jobs.size() - pos_new - 1;
    data.solution.jobs.erase(data.solution.jobs.begin() + pos_new);
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    // Update data.completion_times_0.
    update_completion_times(instance, data, pos_new);
    // Update data.reverse_completion_times_0.
    update_reverse_completion_times(instance, data, p);
    // Update data.solution.makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
}

void remove_jobs(
        const Instance& instance,
        LocalSearchData& data,
        std::vector<JobId>& positions)
{
    // Update data.solution.jobs.
    std::sort(positions.rbegin(), positions.rend());
    // pos_max == data.solution.jobs.size() - 1 => p = 0
    JobId p = data.solution.jobs.size() - positions.front() - 1;
    for (JobId pos: positions)
        data.solution.jobs.erase(data.solution.jobs.begin() + pos);
    // Update data.solution.jobs_positions.
    for (JobId pos = positions.front(); pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    // Update data.completion_times_0.
    update_completion_times(instance, data, positions.back());
    // Update data.reverse_completion_times_0.
    update_reverse_completion_times(instance, data, p);
    // Update data.solution.makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
}

void remove_block(
        const Instance& instance,
        LocalSearchData& data,
        JobId pos_new,
        JobId size)
{
    // pos_new == data.solution.jobs.size() - 1, size == 1 => p = 0
    // pos_new == data.solution.jobs.size() - 2, size == 2 => p = 0
    // pos_new == data.solution.jobs.size() - 2, size == 1 => p = 1
    JobId p = data.solution.jobs.size() - pos_new - size;
    data.solution.jobs.erase(
            data.solution.jobs.begin() + pos_new,
            data.solution.jobs.begin() + pos_new + size);
    // Update data.solution.jobs_positions.
    for (JobId pos = pos_new; pos < (JobId)data.solution.jobs.size(); ++pos) {
        JobId job_id = data.solution.jobs[pos];
        data.solution.jobs_positions[job_id] = pos;
    }
    // Update data.completion_times_0.
    update_completion_times(instance, data, pos_new);
    // Update data.reverse_completion_times_0.
    update_reverse_completion_times(instance, data, p);
    // Update data.solution.makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
}

void shift_jobs(
        const Instance& instance,
        LocalSearchData& data,
        JobId size,
        JobId pos_old,
        JobId pos_new)
{
    if (pos_new > pos_old) {
        // pos_new = data.solution.jobs.size() - 1, size = 1 => p = 0
        // pos_new = data.solution.jobs.size() - 2, size = 2 => p = 0
        JobId p = data.solution.jobs.size() - pos_new - size;
        // Update data.solution.jobs.
        std::rotate(
                data.solution.jobs.begin() + pos_old,
                data.solution.jobs.begin() + pos_old + size,
                data.solution.jobs.begin() + pos_new + size);
        // Update data.solution.jobs_positions.
        for (JobId pos = pos_old; pos < pos_new + size; ++pos) {
            JobId job_id = data.solution.jobs[pos];
            data.solution.jobs_positions[job_id] = pos;
        }
        // Update data.completion_times_0.
        update_completion_times(instance, data, pos_old);
        // Update data.reverse_completion_times_0.
        update_reverse_completion_times(instance, data, p);
    } else {
        // pos_old = data.solution.jobs.size() - 1, size = 1 => p = 0
        // pos_old = data.solution.jobs.size() - 2, size = 2 => p = 0
        JobId p = data.solution.jobs.size() - pos_old - size;
        // Update data.solution.jobs.
        std::rotate(
                data.solution.jobs.begin() + pos_new,
                data.solution.jobs.begin() + pos_old,
                data.solution.jobs.begin() + pos_old + size);
        // Update data.solution.jobs_positions.
        for (JobId pos = pos_new; pos < pos_old + size; ++pos) {
            JobId job_id = data.solution.jobs[pos];
            data.solution.jobs_positions[job_id] = pos;
        }
        // Update data.completion_times_0.
        update_completion_times(instance, data, pos_new);
        // Update data.reverse_completion_times_0.
        update_reverse_completion_times(instance, data, p);
    }
    // Update data.solution.makespan.
    data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];
}

void local_search(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        const LocalSearchOutput& output,
        AlgorithmFormatter& algorithm_formatter,
        LocalSearchData& data)
{
    for (;;) {
        //std::cout << "makespan " << data.solution.makespan << std::endl;
        //for (JobId job_id: data.solution.jobs)
        //    std::cout << " " << job_id;
        //std::cout << std::endl;

        std::shuffle(data.shuffled_positions_1.begin(), data.shuffled_positions_1.end(), generator);
        std::shuffle(data.shuffled_positions_2.begin(), data.shuffled_positions_2.end(), generator);

        MachineId last_machine_id = instance.number_of_machines() - 1;

        JobId size_best = -1;
        JobId pos_old_best = -1;
        JobId pos_new_best = -1;
        Time makespan_best = data.solution.makespan;

        std::vector<JobId> sizes = {1, 2, 3, 4};
        //std::vector<JobId> sizes = {1};
        //std::shuffle(sizes.begin(), sizes.end(), generator);
        for (JobId size: sizes) {
            if (data.solution.jobs.size() <= size)
                continue;
            for (JobId pos_old: data.shuffled_positions_1) {
                if (pos_old > data.solution.jobs.size() - size)
                    continue;

                // Compute data.completion_times.
                for (MachineId machine_id = 0;
                        machine_id < instance.number_of_machines();
                        ++machine_id) {
                    data.completion_times[pos_old][machine_id] = data.completion_times_0[pos_old][machine_id];
                    //std::cout << "compute c"
                    //    << " machine_id " << machine_id
                    //    << " c " << data.completion_times[pos_old][machine_id]
                    //    << std::endl;
                }
                for (JobId pos = pos_old + 1;
                        pos <= (JobId)data.solution.jobs.size() - size;
                        ++pos) {
                    JobId job_id = data.solution.jobs[pos - 1 + size];
                    //std::cout << "compute c"
                    //    << " pos " << pos
                    //    << " job_id " << job_id
                    //    << std::endl;
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[0].machines[0].processing_time;
                    data.completion_times[pos][0] = data.completion_times[pos - 1][0] + p0;
                    //std::cout << "compute c"
                    //    << " machine_id " << 0
                    //    << " p " << p0
                    //    << " c " << data.completion_times[pos][0]
                    //    << std::endl;
                    for (MachineId machine_id = 1;
                            machine_id < instance.number_of_machines();
                            ++machine_id) {
                        Time p = job.operations[machine_id].machines[0].processing_time;
                        if (data.completion_times[pos - 1][machine_id] > data.completion_times[pos][machine_id - 1]) {
                            data.completion_times[pos][machine_id] = data.completion_times[pos - 1][machine_id] + p;
                        } else {
                            data.completion_times[pos][machine_id] = data.completion_times[pos][machine_id - 1] + p;
                        }
                        //std::cout << "compute c"
                        //    << " machine_id " << machine_id
                        //    << " p " << p
                        //    << " c " << data.completion_times[pos][machine_id]
                        //    << std::endl;
                    }
                }

                // Compute data.reverse_completion_times.
                // pos_old == solution.jobs.size() - 1, size == 1 => p = 0
                // pos_old == solution.jobs.size() - 2, size == 2 => p = 0
                // pos_old == solution.jobs.size() - 2, size == 1 => p = 1
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
                    // pos == 1, size == 1 => job_pos = data.solution.jobs.size() - 2
                    // pos == 2, size == 2 => job_pos = data.solution.jobs.size() - 3
                    // pos == 2, size == 1 => job_pos = data.solution.jobs.size() - 3
                    JobId job_pos = data.solution.jobs.size() - pos - size;
                    if (job_pos < 0 || job_pos >= data.solution.jobs.size()) {
                        throw std::logic_error(
                                FUNC_SIGNATURE + ": wrong 'job_pos'; "
                                "job_pos: " + std::to_string(job_pos) + "; "
                                "pos: " + std::to_string(pos) + "; "
                                "data.solution.jobs.size(): " + std::to_string(data.solution.jobs.size()) +  ".");
                    }
                    JobId job_id = data.solution.jobs[job_pos];
                    //std::cout << "compute r"
                    //    << " p " << p
                    //    << " pos " << pos
                    //    << " job_pos " << job_pos
                    //    << " job_id " << job_id
                    //    << std::endl;
                    const Job& job = instance.job(job_id);
                    Time p0 = job.operations[last_machine_id].machines[0].processing_time;
                    data.reverse_completion_times[pos][last_machine_id] = data.reverse_completion_times[pos - 1][last_machine_id] + p0;
                    for (MachineId machine_id = last_machine_id - 1;
                            machine_id >= 0;
                            --machine_id) {
                        Time p = job.operations[machine_id].machines[0].processing_time;
                        if (data.reverse_completion_times[pos - 1][machine_id] > data.reverse_completion_times[pos][machine_id + 1]) {
                            data.reverse_completion_times[pos][machine_id] = data.reverse_completion_times[pos - 1][machine_id] + p;
                        } else {
                            data.reverse_completion_times[pos][machine_id] = data.reverse_completion_times[pos][machine_id + 1] + p;
                        }
                    }
                }

                // Compute data.completion_times.
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
                            //std::cout << "compute c2"
                            //    << " pos_new " << pos_new
                            //    << " machine_id " << machine_id
                            //    << " c2 " << data.completion_times_2[machine_id]
                            //    << std::endl;
                        }
                    } else {
                        for (MachineId machine_id = 0;
                                machine_id < instance.number_of_machines();
                                ++machine_id) {
                            data.completion_times_2[machine_id] = data.completion_times[pos_new][machine_id];
                            //std::cout << "compute c2"
                            //    << " pos_new " << pos_new
                            //    << " machine_id " << machine_id
                            //    << " c2 " << data.completion_times_2[machine_id]
                            //    << std::endl;
                        }
                    }
                    for (JobId pos_0 = pos_old; pos_0 < pos_old + size; ++pos_0) {
                        JobId job_id = data.solution.jobs[pos_0];
                        //std::cout << "compute c2"
                        //    << " pos_0 " << pos_0
                        //    << " job_id " << job_id
                        //    << std::endl;
                        const Job& job = instance.job(job_id);
                        Time p0 = job.operations[0].machines[0].processing_time;
                        data.completion_times_2[0] = data.completion_times_2[0] + p0;
                        //std::cout << "compute c2"
                        //    << " pos_new " << pos_new
                        //    << " machine_id " << 0
                        //    << " p " << p0
                        //    << " c2 " << data.completion_times_2[0]
                        //    << std::endl;
                        for (MachineId machine_id = 1;
                                machine_id < instance.number_of_machines();
                                ++machine_id) {
                            Time p = job.operations[machine_id].machines[0].processing_time;
                            if (data.completion_times_2[machine_id] > data.completion_times_2[machine_id - 1]) {
                                data.completion_times_2[machine_id] = data.completion_times_2[machine_id] + p;
                            } else {
                                data.completion_times_2[machine_id] = data.completion_times_2[machine_id - 1] + p;
                            }
                            //std::cout << "compute c2"
                            //    << " pos_new " << pos_new
                            //    << " machine_id " << machine_id
                            //    << " p " << p
                            //    << " c2 " << data.completion_times_2[machine_id]
                            //    << std::endl;
                        }
                    }

                    data.makespans[pos_new] = 0;
                    if (pos_new > pos_old) {
                        // pos_new = solution.jobs.size() - 1, size == 1, => p = 0
                        // pos_new = solution.jobs.size() - 2, size == 2, => p = 0
                        // pos_new = solution.jobs.size() - 2, size == 1, => p = 1
                        JobId p = data.solution.jobs.size() - pos_new - size;
                        for (MachineId machine_id = 0;
                                machine_id < instance.number_of_machines();
                                ++machine_id) {
                            data.makespans[pos_new] = std::max(
                                    data.makespans[pos_new],
                                    data.completion_times_2[machine_id]
                                    + data.reverse_completion_times_0[p][machine_id]);
                            //std::cout << "compute m"
                            //    << " size " << size
                            //    << " pos_old " << pos_old
                            //    << " pos_new " << pos_new
                            //    << " machine_id " << machine_id
                            //    << " p " << p
                            //    << " c " << data.completion_times_2[machine_id]
                            //    << " r " << data.reverse_completion_times[p][machine_id]
                            //    << " m " << data.makespans[pos_new]
                            //    << std::endl;
                        }
                    } else {
                        JobId p = data.solution.jobs.size() - pos_new - size;
                        for (MachineId machine_id = 0;
                                machine_id < instance.number_of_machines();
                                ++machine_id) {
                            data.makespans[pos_new] = std::max(
                                    data.makespans[pos_new],
                                    data.completion_times_2[machine_id]
                                    + data.reverse_completion_times[p][machine_id]);
                            //std::cout << "size " << size
                            //    << " pos_old " << pos_old
                            //    << " pos_new " << pos_new
                            //    << " machine_id " << machine_id
                            //    << " p " << p
                            //    << " c " << data.completion_times_2[machine_id]
                            //    << " r " << data.reverse_completion_times[p][machine_id]
                            //    << " m " << data.makespans[pos_new]
                            //    << std::endl;
                        }
                    }
                }

                // Fidd best move.
                for (JobId pos_new: data.shuffled_positions_2) {
                    if (pos_new > data.solution.jobs.size() - size)
                        continue;
                    if (pos_new == pos_old)
                        continue;
                    Time makespan = data.makespans[pos_new];
                    if (makespan_best > makespan) {
                        size_best = size;
                        pos_old_best = pos_old;
                        pos_new_best = pos_new;
                        makespan_best = makespan;
                    }
                }
            }
            if (makespan_best < data.solution.makespan)
                break;
        }

        if (pos_old_best == -1)
            break;

        // Apply move.
        //std::cout << "size_best " << size_best
        //    << " pos_old_best " << pos_old_best
        //    << " pos_new_best " << pos_new_best
        //    << std::endl;
        shift_jobs(instance, data, size_best, pos_old_best, pos_new_best);
        if (data.solution.makespan != makespan_best) {
            throw std::runtime_error(
                    FUNC_SIGNATURE + ": wrong makespan; "
                    "data.solution.makespan: " + std::to_string(data.solution.makespan) + "; "
                    "makespan_best: " + std::to_string(makespan_best) + ".");
        }
        //Solution solution = build_solution(instance, data.solution);
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

void add_job(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        JobId job_id)
{
    const Job& job = instance.job(job_id);
    JobId pos_best = -1;
    Time makespan_best = 0;

    // Compute data.completion_times.
    std::shuffle(data.shuffled_positions_1.begin(), data.shuffled_positions_1.end(), generator);
    for (JobId pos: data.shuffled_positions_1) {
        if (pos > data.solution.jobs.size())
            continue;

        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            data.completion_times_2[machine_id] = data.completion_times_0[pos][machine_id];
        }

        Time p0 = job.operations[0].machines[0].processing_time;
        data.completion_times_2[0] = data.completion_times_2[0] + p0;
        for (MachineId machine_id = 1;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            Time p = job.operations[machine_id].machines[0].processing_time;
            if (data.completion_times_2[machine_id] > data.completion_times_2[machine_id - 1]) {
                data.completion_times_2[machine_id] = data.completion_times_2[machine_id] + p;
            } else {
                data.completion_times_2[machine_id] = data.completion_times_2[machine_id - 1] + p;
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
                    + data.reverse_completion_times[p][machine_id]);
        }

        if (pos_best == -1
                || makespan_best > makespan) {
            pos_best = pos;
            makespan_best = makespan;
        }
    }

    if (pos_best == -1) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": add, pos_best == -1.");
    }

    // Apply move.
    add_job(instance, data, job_id, pos_best);
    //Solution solution = build_solution(instance, data.solution);
}

void add_block(
        const Instance& instance,
        const LocalSearchParameters& parameters,
        std::mt19937_64& generator,
        LocalSearchData& data,
        const std::vector<JobId>& job_ids)
{
    JobId pos_best = -1;
    Time makespan_best = 0;

    // Compute data.completion_times.
    std::shuffle(data.shuffled_positions_1.begin(), data.shuffled_positions_1.end(), generator);
    for (JobId pos: data.shuffled_positions_1) {
        if (pos > data.solution.jobs.size())
            continue;

        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            data.completion_times_2[machine_id] = data.completion_times_0[pos][machine_id];
        }

        for (JobId job_id: job_ids) {
            const Job& job = instance.job(job_id);
            Time p0 = job.operations[0].machines[0].processing_time;
            data.completion_times_2[0] = data.completion_times_2[0] + p0;
            for (MachineId machine_id = 1;
                    machine_id < instance.number_of_machines();
                    ++machine_id) {
                Time p = job.operations[machine_id].machines[0].processing_time;
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
                    + data.reverse_completion_times[p][machine_id]);
        }

        if (pos_best == -1
                || makespan_best > makespan) {
            pos_best = pos;
            makespan_best = makespan;
        }
    }

    if (pos_best == -1) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": add, pos_best == -1.");
    }

    // Apply move.
    add_block(instance, data, job_ids, pos_best);
    //Solution solution = build_solution(instance, data.solution);
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
    std::uniform_int_distribution<JobId> d_size(1, 4);
    JobId size = d_size(generator);
    std::uniform_int_distribution<JobId> d_pos(0, data.solution.jobs.size() - size);
    JobId pos = d_pos(generator);
    std::vector<JobId> removed_jobs_ids(
            data.solution.jobs.begin() + pos,
            data.solution.jobs.begin() + pos + size);
    remove_block(instance, data, pos, size);
    //Solution solution = build_solution(instance, data.solution);
    return removed_jobs_ids;
}

JobId remove_job(
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
            p_total += job.operations[machine_id].machines[0].processing_time;
        }
    }

    std::shuffle(data.shuffled_positions_1.begin(), data.shuffled_positions_1.end(), generator);
    JobId pos_best = -1;
    double value_best = (double)p_total / data.solution.makespan * instance.number_of_machines();
    Time makespan_best = data.solution.makespan;
    JobId tabu_size = std::min(instance.number_of_jobs(), parameters.tabu_size);
    for (JobId pos: data.shuffled_positions_1) {
        if (pos >= data.solution.jobs.size())
            continue;
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
            p_cur -= job.operations[machine_id].machines[0].processing_time;
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
    data.shuffled_positions_1 = std::vector<JobId>(instance.number_of_jobs());
    data.shuffled_positions_2 = std::vector<JobId>(instance.number_of_jobs());
    std::iota(data.shuffled_positions_1.begin(), data.shuffled_positions_1.end(), 0);
    std::iota(data.shuffled_positions_2.begin(), data.shuffled_positions_2.end(), 0);
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
    data.makespans = std::vector<Time>(instance.number_of_jobs(), 0);
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
        // Sort jobs by total processing time.
        std::vector<JobId> sorted_jobs(instance.number_of_jobs());
        std::iota(sorted_jobs.begin(), sorted_jobs.end(), 0);
        //std::shuffle(sorted_jobs.begin(), sorted_jobs.end(), generator);
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
            //std::cout << "add job " << job_id
            //    << " size " << data.solution.jobs.size()
            //    << " makespan " << data.solution.makespan
            //    << std::endl;
            add_job(instance, parameters, generator, data, job_id);
            local_search(instance, parameters, generator, output, algorithm_formatter, data);
        }
        population.add(data.solution, generator);
    }

    for (output.number_of_iterations = 0;
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
        update_completion_times(instance, data, 0);
        update_reverse_completion_times(instance, data, 0);
        data.solution.makespan = data.completion_times_0[data.solution.jobs.size()][instance.number_of_machines() - 1];

        std::vector<JobId> removed_jobs_ids = remove_random_block(instance, parameters, generator, output, data);
        local_search(instance, parameters, generator, output, algorithm_formatter, data);
        add_block(instance, parameters, generator, data, removed_jobs_ids);
        local_search(instance, parameters, generator, output, algorithm_formatter, data);

        //JobId removed_job_id = remove_random_job(instance, parameters, generator, output, data);
        //local_search(instance, parameters, generator, output, algorithm_formatter, data);
        //add_job(instance, parameters, generator, data, removed_job_id);
        //local_search(instance, parameters, generator, output, algorithm_formatter, data);

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
