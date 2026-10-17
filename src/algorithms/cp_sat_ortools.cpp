#include "shopschedulingsolver/algorithms/cp_sat_ortools.hpp"

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include "ortools/util/time_limit.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using namespace operations_research;
using namespace operations_research::sat;

namespace
{

struct OperationType
{
    IntVar start;
    IntVar end;
    IntervalVar interval;
};

void update_bound(
        const shopschedulingsolver::Instance& instance,
        shopschedulingsolver::AlgorithmFormatter& algorithm_formatter,
        double bound)
{
    using namespace shopschedulingsolver;
    if (!std::isfinite(bound))
        return;
    switch (instance.objective()) {
    case Objective::Makespan:
        algorithm_formatter.update_makespan_bound(std::round(bound), "");
        break;
    case Objective::TotalFlowTime:
        algorithm_formatter.update_total_flow_time_bound(std::round(bound), "");
        break;
    case Objective::TotalTardiness:
        algorithm_formatter.update_total_tardiness_bound(std::round(bound), "");
        break;
    default:
        break;
    }
}

}

shopschedulingsolver::Output shopschedulingsolver::cp_sat_ortools(
        const Instance& instance,
        const Parameters& parameters)
{
    Output output(instance);

    if (instance.objective() == Objective::Throughput) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'objective'; "
                "objective Throughput is not supported.");
    }
    if (instance.flexible()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'flexible'; "
                "flexible instances are not supported.");
    }
    if (instance.operations_arbitrary_order()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'operations_arbitrary_order'; "
                "open-shop instances are not supported.");
    }
    if (instance.blocking()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'blocking'; "
                "blocking instances are not supported.");
    }
    if (instance.no_idle()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'no_idle'; "
                "no-idle instances are not supported.");
    }
    if (instance.mixed_no_idle()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'mixed_no_idle'; "
                "mixed no-idle instances are not supported.");
    }
    if (instance.permutation()) {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid 'permutation'; "
                "permutation instances are not supported.");
    }

    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("CP-SAT (OR-Tools)");

    // Compute time horizon for variable bounds
    int64_t max_release_date = 0;
    int64_t total_duration = 0;
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const auto& job = instance.job(job_id);
        max_release_date = std::max(max_release_date, job.release_date);
        for (const auto& operation: job.operations) {
            Time maximum_processing_time = 0;
            for (const auto& alternative: operation.alternatives)
                maximum_processing_time = std::max(maximum_processing_time, alternative.processing_time);
            total_duration += maximum_processing_time;
        }
    }
    int64_t horizon = max_release_date + total_duration;

    CpModelBuilder cp_model;

    std::vector<std::vector<OperationType>> all_operations(
        instance.number_of_jobs());
    std::vector<std::vector<IntervalVar>> machine_to_intervals(
        instance.number_of_machines());

    // Model variables (jobs and operations)
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const auto& job = instance.job(job_id);
        all_operations[job_id].resize(job.operations.size());

        // Per operation variables (start, end, interval)
        for (OperationId operation_id = 0;
                operation_id < (OperationId)job.operations.size();
                ++operation_id) {
            const auto& operation = job.operations[operation_id];

            // We don't handle the flexible variant for now so we only care on
            // the only first machine alternative
            const auto& alternative = operation.alternatives[0];
            MachineId machine = alternative.machine_id;
            Time duration = alternative.processing_time;

            std::string suffix =
                "_" + std::to_string(job_id) + "_" + std::to_string(operation_id);
            IntVar start = cp_model.NewIntVar({job.release_date, horizon})
                               .WithName(std::string("start") + suffix);
            IntVar end = cp_model.NewIntVar({job.release_date, horizon})
                             .WithName(std::string("end") + suffix);
            IntervalVar interval =
                cp_model.NewIntervalVar(start, duration, end)
                    .WithName(std::string("interval") + suffix);

            all_operations[job_id][operation_id] = OperationType{start, end, interval};
            machine_to_intervals[machine].push_back(interval);
        }
    }

    // No overlap on each machine
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        cp_model.AddNoOverlap(machine_to_intervals[machine_id]);
    }

    // Job constraints
    // The operations of a job are processed in order, so the end of a job is
    // the end of its last operation.
    std::vector<LinearExpr> job_ends(instance.number_of_jobs());
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const auto& job = instance.job(job_id);
        job_ends[job_id] = (job.operations.empty())?
            LinearExpr(job.release_date):
            LinearExpr(all_operations[job_id].back().end);

        // Job's operations precedence constraints
        for (OperationId operation_id = 0;
                operation_id < (OperationId)job.operations.size() - 1;
                ++operation_id) {
            if (instance.no_wait()) {
                // No time between two consecutive job's operations
                cp_model.AddEquality(
                        all_operations[job_id][operation_id + 1].start,
                        all_operations[job_id][operation_id].end);
            } else {
                cp_model.AddGreaterOrEqual(
                        all_operations[job_id][operation_id + 1].start,
                        all_operations[job_id][operation_id].end);
            }
        }
    }

    // Objective function
    if (instance.objective() == Objective::Makespan) {
        IntVar makespan = cp_model.NewIntVar({0, horizon}).WithName("Makespan");
        cp_model.AddMaxEquality(makespan, job_ends);
        cp_model.Minimize(makespan);

    } else if (instance.objective() == Objective::TotalFlowTime) {
        LinearExpr total_flow_time;
        for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
            const auto& job = instance.job(job_id);
            total_flow_time += job.weight * (job_ends[job_id] - job.release_date);
        }
        cp_model.Minimize(total_flow_time);

    } else if (instance.objective() == Objective::TotalTardiness) {
        LinearExpr total_tardiness;
        for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
            const auto& job = instance.job(job_id);
            // Ignore the jobs that don't have deadlines
            if (job.due_date == -1)
                continue;
            IntVar tardiness = cp_model.NewIntVar({0, horizon})
                .WithName("tardiness_" + std::to_string(job_id));
            cp_model.AddGreaterOrEqual(tardiness, job_ends[job_id] - job.due_date);
            total_tardiness += job.weight * tardiness;
        }
        cp_model.Minimize(total_tardiness);
    }

    operations_research::sat::SatParameters cp_parameters;

    // Write solver output to file.
    // TODO
    //cp_parameters.set_log_search_progress(true);

    // Set time limit.
    cp_parameters.set_max_time_in_seconds(parameters.timer.remaining_time());

    Model model;
    model.Add(NewSatParameters(cp_parameters));

    // Stop the search when the timer ends. The timer is checked in the
    // callbacks, as in the MILP algorithms.
    std::atomic<bool> stop(false);
    model.GetOrCreate<TimeLimit>()->RegisterExternalBooleanAsLimit(&stop);

    // The callbacks may be called from different worker threads.
    std::mutex callback_mutex;

    // Report each new solution.
    model.Add(NewFeasibleSolutionObserver(
                [&instance, &parameters, &algorithm_formatter, &all_operations, &stop, &callback_mutex](
                    const CpSolverResponse& response)
                {
                    std::lock_guard<std::mutex> lock(callback_mutex);
                    SolutionBuilder solution_builder;
                    solution_builder.set_instance(instance);
                    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
                        const auto& job = instance.job(job_id);
                        for (OperationId operation_id = 0;
                                operation_id < (OperationId)job.operations.size();
                                ++operation_id) {
                            Time start = SolutionIntegerValue(
                                    response, all_operations[job_id][operation_id].start);
                            solution_builder.append_operation(job_id, operation_id, 0, start);
                        }
                    }
                    solution_builder.sort_machines();
                    solution_builder.sort_jobs();
                    algorithm_formatter.update_solution(solution_builder.build(), "");
                    if (parameters.timer.needs_to_end())
                        stop = true;
                }));

    // Report each new bound.
    model.Add(NewBestBoundCallback(
                [&instance, &parameters, &algorithm_formatter, &stop, &callback_mutex](
                    double bound)
                {
                    std::lock_guard<std::mutex> lock(callback_mutex);
                    update_bound(instance, algorithm_formatter, bound);
                    if (parameters.timer.needs_to_end())
                        stop = true;
                }));

    // Solve the model.
    CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);

    switch (response.status()) {
    case CpSolverStatus::OPTIMAL:
    case CpSolverStatus::FEASIBLE:
    case CpSolverStatus::UNKNOWN:
        // Solutions are reported by the observer; only the final bound is
        // updated here.
        update_bound(instance, algorithm_formatter, response.best_objective_bound());
        break;
    case CpSolverStatus::INFEASIBLE:
        // The horizon is an upper bound of the end of a feasible schedule.
        throw std::logic_error(
                FUNC_SIGNATURE + ": "
                "the model is infeasible.");
    default:
        throw std::logic_error(
                FUNC_SIGNATURE + ": "
                "the model is invalid; "
                "status: " + CpSolverStatus_Name(response.status()) + ".");
    }

    algorithm_formatter.end();
    return output;
}
