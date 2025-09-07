#include "shopschedulingsolver/solution_builder.hpp"

using namespace shopschedulingsolver;

SolutionBuilder& SolutionBuilder::set_instance(
        const Instance& instance)
{
    this->solution_.instance_ = &instance;
    this->solution_.jobs_ = std::vector<Solution::Job>(instance.number_of_jobs());
    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        this->solution_.jobs_[job_id].operations = std::vector<SolutionOperationId>(job.operations.size(), -1);
    }
    this->solution_.machines_ = std::vector<Solution::Machine>(instance.number_of_machines());
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
    Solution::Job& solution_job = this->solution_.jobs_[job_id];

    SolutionOperationId solution_operation_id = this->solution_.operations_.size();
    Solution::Operation solution_operation;
    solution_operation.machine_id = operation_machine.machine_id;
    solution_operation.job_id = job_id;
    solution_operation.operation_id = operation_id;
    solution_operation.operation_machine_id = operation_machine_id;
    solution_operation.job_position = solution_job.solution_operations.size();
    solution_operation.machine_position = solution_machine.solution_operations.size();
    solution_operation.start = start;
    this->solution_.operations_.push_back(solution_operation);

    solution_machine.solution_operations.push_back(solution_operation_id);
    solution_job.solution_operations.push_back(solution_operation_id);
    solution_job.operations[operation_id] = solution_operation_id;
}

void SolutionBuilder::sort_machines()
{
    const Instance& instance = this->solution_.instance();
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        Solution::Machine& solution_machine = solution_.machines_[machine_id];
        sort(
                solution_machine.solution_operations.begin(),
                solution_machine.solution_operations.end(),
                [this](
                    SolutionOperationId solution_operation_1_id,
                    SolutionOperationId solution_operation_2_id) -> bool
                {
                    const Solution::Operation& solution_operation_1 = this->solution_.operations_[solution_operation_1_id];
                    const Solution::Operation& solution_operation_2 = this->solution_.operations_[solution_operation_2_id];
                    return solution_operation_1.start < solution_operation_2.start;
                });
        for (JobId machine_position = 0;
                machine_position < (JobId)solution_machine.solution_operations.size();
                ++machine_position) {
            Solution::Operation& solution_operation = solution_.operations_[solution_machine.solution_operations[machine_position]];
            solution_operation.machine_position = machine_position;
        }
    }
}

void SolutionBuilder::sort_jobs()
{
    const Instance& instance = this->solution_.instance();
    for (JobId job_id = 0;
            job_id < instance.number_of_jobs();
            ++job_id) {
        Solution::Job& solution_job = solution_.jobs_[job_id];
        sort(
                solution_job.solution_operations.begin(),
                solution_job.solution_operations.end(),
                [this](
                    SolutionOperationId solution_operation_1_id,
                    SolutionOperationId solution_operation_2_id) -> bool
                {
                    const Solution::Operation& solution_operation_1 = this->solution_.operations_[solution_operation_1_id];
                    const Solution::Operation& solution_operation_2 = this->solution_.operations_[solution_operation_2_id];
                    return solution_operation_1.start < solution_operation_2.start;
                });
        for (JobId job_position = 0;
                job_position < (JobId)solution_job.solution_operations.size();
                ++job_position) {
            Solution::Operation& solution_operation = solution_.operations_[solution_job.solution_operations[job_position]];
            solution_operation.job_position = job_position;
        }
    }
}

Solution SolutionBuilder::build()
{
    const Instance& instance = this->solution_.instance();

    for (JobId job_id = 0; job_id < instance.number_of_jobs(); ++job_id) {
        const Job& job = instance.job(job_id);
        Solution::Job& solution_job = this->solution_.jobs_[job_id];
        Time current_time = 0;
        OperationId operation_prev_id = -1;
        for (SolutionOperationId solution_operation_id: solution_job.solution_operations) {
            const Solution::Operation& o = this->solution_.operations_[solution_operation_id];
            if (solution_job.start == -1)
                solution_job.start = o.start;
            Time end = o.start + instance.job(o.job_id).operations[o.operation_id].machines[o.operation_machine_id].processing_time;
            // Check job overlap.
            if (end < current_time)
                this->solution_.number_of_job_overlaps_++;
            // Check no-wait.
            if (end != current_time)
                this->solution_.no_wait_ = false;
            // Check precedences.
            if (!instance.operations_arbitrary_order())
                if (o.operation_id < operation_prev_id)
                    this->solution_.number_of_precedence_violations_++;
            current_time = end;
            operation_prev_id = o.operation_id;
        }
        solution_job.end = current_time;

        // Check release date.
        if (solution_job.start < job.release_date)
            solution_.number_of_release_date_violations_++;
        // Updte makespan.
        this->solution_.makespan_ = (std::max)(
                solution_.makespan_,
                solution_job.end);
        // Update total flow time.
        this->solution_.total_flow_time_ += job.weight * (solution_job.end - job.release_date);
        // Update throughput.
        if (solution_job.solution_operations.size() == job.operations.size())
            this->solution_.throughput_ += job.weight;
        // Update total tardiness.
        if (job.due_date != -1 && solution_job.end > job.due_date)
            this->solution_.total_tardiness_ += job.weight * (solution_job.end - job.due_date);
    }

    std::vector<JobId> jobs_positions(instance.number_of_jobs(), -1);
    for (MachineId machine_id = 0;
            machine_id < instance.number_of_machines();
            ++machine_id) {
        const Machine& machine = instance.machine(machine_id);
        const Solution::Machine& solution_machine = this->solution_.machines_[machine_id];
        Time current_time = 0;
        for (SolutionOperationId solution_operation_id: solution_machine.solution_operations) {
            const Solution::Operation& o = this->solution_.operations_[solution_operation_id];
            Time end = o.start + instance.job(o.job_id).operations[o.operation_id].machines[o.operation_machine_id].processing_time;
            // Check machine overlap.
            if (end < current_time)
                this->solution_.number_of_machine_overlaps_++;
            // Check no-idle.
            if (end != current_time)
                this->solution_.no_idle_ = false;
            // Check blocking.
            if (o.machine_position != 0) {
                // Get the previous operation on the machine.
                OperationMachineId solution_operation_prev_id = solution_machine.solution_operations[o.machine_position - 1];
                const Solution::Operation& o_prev = this->solution_.operations_[solution_operation_prev_id];
                // Get the job of the previous operation.
                const Job& job_prev = instance.job(o_prev.job_id);
                const Solution::Job& solution_job_prev = solution_.job(o_prev.job_id);
                // If the job is not finished.
                if (o_prev.job_position + 1 < solution_job_prev.solution_operations.size()) {
                    // Get the next scheduled operation of the previous job.
                    SolutionOperationId solution_operation_prev_next_id = solution_job_prev.solution_operations[o_prev.job_position + 1];
                    const Solution::Operation& solution_operation_prev_next = solution_.operations_[solution_operation_prev_next_id];
                    // This operation must start before the start of the current operation.
                    if (solution_operation_prev_next.start > o.start)
                        this->solution_.blocking_ = false;
                }
            }
            // Check permutation.
            if (machine_id == 0) {
                jobs_positions[o.job_id] = o.machine_position;
            } else {
                if (jobs_positions[o.job_id] != o.machine_position)
                    this->solution_.permutation_ = false;
            }
            current_time = end;
        }
    }

    return std::move(solution_);
}

void SolutionBuilder::read(
        const std::string& certificate_path)
{
    std::ifstream file(certificate_path);
    if (!file.good()) {
        throw std::runtime_error(
                "shopschedulingsolver::SolutionBuilder::read: "
                "unable to open file \"" + certificate_path + "\".");
    }

    nlohmann ::json j;
    file >> j;
    for (const auto& json_operation: j["operations"]) {
        this->append_operation(
                json_operation["job_id"],
                json_operation["operation_id"],
                json_operation["operation_machine_id"],
                json_operation["start"]);
    }
}
