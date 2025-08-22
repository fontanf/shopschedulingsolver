#include "shopschedulingsolver/solution_builder.hpp"

using namespace shopschedulingsolver;

SolutionBuilder& SolutionBuilder::set_instance(
        const Instance& instance)
{
    solution_.instance_ = &instance;
    return *this;
}

void SolutionBuilder::append_operation(
        JobId job_id,
        OperationId operation_id,
        OperationMachineId operation_machine_id,
        Time start)
{
    const Instance& instance = solution_.instance();
    const Job& job = instance.job(job_id);
    const Operation& operation = job.operations[operation_id];
    const OperationMachine& operation_machine = operation.machines[operation_machine_id];
    Solution::Machine& solution_machine = this->solution_.machines_[operation_machine.machine_id];
    Solution::MachineOperation solution_machine_operation;
    solution_machine_operation.job_id = job_id;
    solution_machine_operation.operation_id = operation_id;
    solution_machine_operation.operation_machine_id = operation_machine_id;
    solution_machine_operation.start = start;
    solution_machine.operations.push_back(solution_machine_operation);
}

Solution SolutionBuilder::build()
{
    const Instance& instance = this->solution_.instance();

    this->solution_.jobs_ = std::vector<Solution::Job>(instance.number_of_jobs());
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        this->solution_.jobs_[job_id].operations = std::vector<Solution::Operation>(job.operations.size());
    }

    std::vector<Time> jobs_next_availabilities(instance.number_of_jobs(), 0);
    std::vector<Time> machines_next_availabilities(instance.number_of_machines(), 0);
    std::vector<OperationId> jobs_last_operations(instance.number_of_jobs(), -1);
    // Check constraints.
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        Solution::Machine& solution_machine = this->solution_.machines_[machine_id];
        for (OperationId pos = 0;
                pos < (OperationId)solution_machine.operations.size();
                ++pos) {
            const Solution::MachineOperation& smo = solution_machine.operations[pos];
            const Job& job = instance.job(smo.job_id);
            const Operation& operation = job.operations[smo.operation_id];
            const OperationMachine& operation_machine = operation.machines[smo.operation_machine_id];
            Time end = smo.start + operation_machine.processing_time;

            // Check job and machine overlap.
            if (smo.start < jobs_next_availabilities[smo.job_id])
                this->solution_.number_of_job_overlaps_++;
            if (smo.start < machines_next_availabilities[machine_id])
                this->solution_.number_of_machine_overlaps_++;
            jobs_next_availabilities[smo.job_id] = end;
            // Check precedences.
            if (!instance.operations_arbitrary_order()) {
                if (smo.operation_id < jobs_last_operations[smo.job_id])
                    this->solution_.number_of_precedence_violations_++;
            }
            jobs_last_operations[smo.job_id] = smo.operation_id;

            Solution::Job& solution_job = this->solution_.jobs_[smo.job_id];
            solution_job.operations[smo.operation_id].machine_id = machine_id;
            solution_job.operations[smo.operation_id].operation_pos = pos;
            solution_job.number_of_operations++;
            if (solution_job.number_of_operations == 1)
                solution_job.start = smo.start;
            if (solution_job.number_of_operations == job.operations.size())
                solution_job.end = end;
        }
    }

    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        const Solution::Job& solution_job = this->solution_.jobs_[job_id];
        if (solution_job.start < job.release_date)
            solution_.number_of_release_date_violations_++;
        this->solution_.makespan_ = (std::max)(
                solution_.makespan_,
                solution_job.end);
        this->solution_.total_flow_time_ += job.weight * (solution_job.end - job.release_date);
        if (solution_job.number_of_operations == job.operations.size())
            this->solution_.throughput_ += job.weight;
        if (solution_job.end > job.due_date)
            this->solution_.total_tardiness_ += job.weight * (solution_job.end - job.due_date);

    }
    return std::move(solution_);
}
