#include "shopschedulingsolver/solution.hpp"

using namespace shopschedulingsolver;

bool Solution::feasible() const
{
    return (this->number_of_release_date_violations() == 0
        && this->number_of_job_overlaps() == 0
        && this->number_of_machine_overlaps() == 0
        && this->number_of_precedence_violations() == 0
        && (!this->instance().no_wait() || this->no_wait())
        && (!this->instance().no_idle() || this->no_idle())
        && (!this->instance().blocking() || this->blocking())
        && (!this->instance().permutation() || this->permutation())
        && this->number_of_operations() == instance().number_of_operations());
}

bool Solution::strictly_better(const Solution& solution) const
{
    if (!this->feasible())
        return false;
    if (!solution.feasible())
        return true;
    switch (solution.instance().objective()) {
    case Objective::Makespan: {
        return this->makespan() < solution.makespan();
    } case Objective::TotalFlowTime: {
        return this->total_flow_time() < solution.total_flow_time();
    } case Objective::Throughput: {
        return this->throughput() < solution.throughput();
    } case Objective::TotalTardiness: {
        return this->total_tardiness() < solution.total_tardiness();
    }
    }
    return false;
}

void Solution::write(
        const std::string& certificate_path,
        const std::string& format) const
{
    if (certificate_path.empty())
        return;
    std::ofstream file{certificate_path};
    if (!file.good()) {
        throw std::runtime_error(
                "shopschedulingsolver::Solution::write: "
                "Unable to open file \"" + certificate_path + "\".");
    }

    nlohmann::json json;
    json["number_of_machines"] = this->instance().number_of_machines();
    json["number_of_jobs"] = this->instance().number_of_jobs();
    json["number_of_operations"] = this->instance().number_of_operations();
    for (SolutionOperationId solution_operation_id = 0;
            solution_operation_id < this->number_of_operations();
            ++solution_operation_id) {
        const Solution::Operation& solution_operation = this->operation(solution_operation_id);
        const auto& job = this->instance().job(solution_operation.job_id);
        const auto& operation = job.operations[solution_operation.operation_id];
        const auto& machine_operation = operation.machines[solution_operation.operation_machine_id];
        json["operations"][solution_operation_id]["job_id"] = solution_operation.job_id;
        json["operations"][solution_operation_id]["job_position"] = solution_operation.job_position;
        json["operations"][solution_operation_id]["operation_id"] = solution_operation.operation_id;
        json["operations"][solution_operation_id]["operation_machine_id"] = solution_operation.operation_machine_id;
        json["operations"][solution_operation_id]["machine_id"] = solution_operation.machine_id;
        json["operations"][solution_operation_id]["machine_position"] = solution_operation.machine_position;
        json["operations"][solution_operation_id]["start"] = solution_operation.start;
        json["operations"][solution_operation_id]["processing_time"] = machine_operation.processing_time;
        json["operations"][solution_operation_id]["end"] = solution_operation.start + machine_operation.processing_time;
    }
    file << std::setw(4) << json << std::endl;
}

nlohmann::json Solution::to_json() const
{
    return nlohmann::json {
        {"NumberOfOperations", this->number_of_operations()},
        {"NumberOfJobsOverlaps", this->number_of_job_overlaps()},
        {"NumberOfMachinesOverlaps", this->number_of_machine_overlaps()},
        {"NumberOfPrecedence violations", this->number_of_precedence_violations()},
        {"NumberOfReleaseDateViolations", this->number_of_release_date_violations()},
        {"Makespan", this->makespan()},
        {"TotalFlowTime", this->total_flow_time()},
        {"Throughput", this->throughput()},
        {"TotalTardiness", this->total_tardiness()},
    };
}

void Solution::format(
        std::ostream& os,
        int verbosity_level) const
{
    if (verbosity_level >= 1) {
        os
            << "Number of operations:        " << this->number_of_operations() << std::endl
            << "# job overlaps:              " << this->number_of_job_overlaps() << std::endl
            << "# machine overlaps:          " << this->number_of_machine_overlaps() << std::endl
            << "# precedence violations:     " << this->number_of_precedence_violations() << std::endl
            << "# release dates violations:  " << this->number_of_release_date_violations() << std::endl
            << "No-wait:                     " << this->no_wait() << std::endl
            << "No-idle:                     " << this->no_idle() << std::endl
            << "Blocking:                    " << this->blocking() << std::endl
            << "Permutation:                 " << this->permutation() << std::endl
            << "Feasible:                    " << this->feasible() << std::endl
            << "Makespan:                    " << this->makespan() << std::endl
            << "Total flow time:             " << this->total_flow_time() << std::endl
            << "Throughput:                  " << this->throughput() << std::endl
            << "Total tardiness:             " << this->total_tardiness() << std::endl
            ;
    }

    const Instance& instance = this->instance();
    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Machine"
            << std::setw(12) << "# op."
            << std::setw(12) << "Proc. time"
            << std::setw(12) << "Start"
            << std::setw(12) << "End"
            << std::endl
            << std::setw(12) << "-------"
            << std::setw(12) << "-----"
            << std::setw(12) << "----------"
            << std::setw(12) << "-----"
            << std::setw(12) << "---"
            << std::endl;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            const Solution::Machine& solution_machine = this->machine(machine_id);
            os
                << std::setw(12) << machine_id
                << std::setw(12) << solution_machine.solution_operations.size()
                << std::setw(12) << solution_machine.processing_time
                << std::setw(12) << solution_machine.start
                << std::setw(12) << solution_machine.end
                << std::endl;
        }
    }

    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "# op."
            << std::setw(12) << "Proc. time"
            << std::setw(12) << "Start"
            << std::setw(12) << "End"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "-----"
            << std::setw(12) << "----------"
            << std::setw(12) << "-----"
            << std::setw(12) << "---"
            << std::endl;
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Solution::Job& solution_job = this->job(job_id);
            os
                << std::setw(12) << job_id
                << std::setw(12) << solution_job.solution_operations.size()
                << std::setw(12) << solution_job.processing_time
                << std::setw(12) << solution_job.start
                << std::setw(12) << solution_job.end
                << std::endl;
        }
    }

    if (verbosity_level >= 3) {
        os << std::right << std::endl
            << std::setw(12) << "Machine"
            << std::setw(12) << "Mac. pos."
            << std::setw(12) << "Job"
            << std::setw(12) << "Operation"
            << std::setw(12) << "Op. mach."
            << std::setw(12) << "Job pos."
            << std::setw(12) << "Start"
            << std::setw(12) << "End"
            << std::endl
            << std::setw(12) << "-------"
            << std::setw(12) << "---------"
            << std::setw(12) << "---"
            << std::setw(12) << "---------"
            << std::setw(12) << "---------"
            << std::setw(12) << "--------"
            << std::setw(12) << "-----"
            << std::setw(12) << "---"
            << std::endl;
        for (MachineId machine_id = 0;
                machine_id < instance.number_of_machines();
                ++machine_id) {
            const Solution::Machine& solution_machine = this->machine(machine_id);
            for (SolutionOperationId solution_operation_id: solution_machine.solution_operations) {
                const Solution::Operation& solution_operation = this->operation(solution_operation_id);
                const auto& job = instance.job(solution_operation.job_id);
                const auto& operation = job.operations[solution_operation.operation_id];
                const auto& operation_machine = operation.machines[solution_operation.operation_machine_id];
                Time end = solution_operation.start + operation_machine.processing_time;
                os
                    << std::setw(12) << machine_id
                    << std::setw(12) << solution_operation.machine_position
                    << std::setw(12) << solution_operation.job_id
                    << std::setw(12) << solution_operation.operation_id
                    << std::setw(12) << solution_operation.operation_machine_id
                    << std::setw(12) << solution_operation.job_position
                    << std::setw(12) << solution_operation.start
                    << std::setw(12) << end
                    << std::endl;
            }
        }
    }

    if (verbosity_level >= 3) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "Operation"
            << std::setw(12) << "Op. mach."
            << std::setw(12) << "Job pos."
            << std::setw(12) << "Machine"
            << std::setw(12) << "Mac. pos."
            << std::setw(12) << "Start"
            << std::setw(12) << "End"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "---------"
            << std::setw(12) << "---------"
            << std::setw(12) << "--------"
            << std::setw(12) << "-------"
            << std::setw(12) << "---------"
            << std::setw(12) << "-----"
            << std::setw(12) << "---"
            << std::endl;
        for (JobId job_id = 0;
                job_id < instance.number_of_jobs();
                ++job_id) {
            const Solution::Job& solution_job = this->job(job_id);
            const auto& job = instance.job(job_id);
            for (SolutionOperationId solution_operation_id: solution_job.solution_operations) {
                const Solution::Operation& solution_operation = this->operation(solution_operation_id);
                const auto& operation = job.operations[solution_operation.operation_id];
                const auto& operation_machine = operation.machines[solution_operation.operation_machine_id];
                Time end = solution_operation.start + operation_machine.processing_time;
                os
                    << std::setw(12) << job_id
                    << std::setw(12) << solution_operation.operation_id
                    << std::setw(12) << solution_operation.operation_machine_id
                    << std::setw(12) << solution_operation.job_position
                    << std::setw(12) << solution_operation.machine_id
                    << std::setw(12) << solution_operation.machine_position
                    << std::setw(12) << solution_operation.start
                    << std::setw(12) << end
                    << std::endl;
            }
        }
    }
}
