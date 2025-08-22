#include "shopschedulingsolver/instance_builder.hpp"

using namespace shopschedulingsolver;

void InstanceBuilder::set_number_of_machines(MachineId number_of_machines)
{
    if (number_of_machines <= 0) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_number_of_machines: "
                "'number_of_machines' must be > 0; "
                "number_of_machines: " + std::to_string(number_of_machines) + ".");
    }

    instance_.machines_ = std::vector<Machine>(number_of_machines);
}

JobId InstanceBuilder::add_job()
{
    JobId job_id = instance_.jobs_.size();
    Job job;
    instance_.jobs_.push_back(job);
    return job_id;
}

OperationId InstanceBuilder::add_operation(
        JobId job_id)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }

    Job& job = instance_.jobs_[job_id];
    OperationId operation_id = job.operations.size();
    Operation operation;
    job.operations.push_back(operation);
    return operation_id;
}

void InstanceBuilder::add_operation_machine(
        JobId job_id,
        OperationId operation_id,
        MachineId machine_id,
        Time processing_time)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    Job& job = instance_.jobs_[job_id];
    if (operation_id < 0 || operation_id >= job.operations.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::add_operation: "
                "invalid 'operation_id'; "
                "operation_id: " + std::to_string(operation_id) + "; "
                "job.operations.size(): " + std::to_string(job.operations.size()) + ".");
    }
    Operation& operation = job.operations[operation_id];
    if (machine_id < 0 || machine_id >= instance_.machines_.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "invalid 'machine_id'; "
                "machine_id: " + std::to_string(machine_id) + "; "
                "instance_.machines_.size(): " + std::to_string(instance_.machines_.size()) + ".");
    }
    if (processing_time <= 0) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_processing_time: "
                "'processing_time' must be > 0; "
                "processing_time: " + std::to_string(processing_time) + ".");
    }

    OperationMachine operation_machine;
    operation_machine.machine_id = machine_id;
    operation_machine.processing_time = processing_time;
    operation.machines.push_back(operation_machine);
}

void InstanceBuilder::set_job_release_date(
        JobId job_id,
        Time release_date)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_release_date: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (release_date < 0) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_release_date: "
                "'release_date' must be >= 0; "
                "release_date: " + std::to_string(release_date) + ".");
    }

    instance_.jobs_[job_id].release_date = release_date;
}

void InstanceBuilder::set_job_due_date(
        JobId job_id,
        Time due_date)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_due_date: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (due_date < 0) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_due_date: "
                "'due_date' must be >= 0; "
                "due_date: " + std::to_string(due_date) + ".");
    }

    instance_.jobs_[job_id].due_date = due_date;
}

void InstanceBuilder::set_job_weight(
        JobId job_id,
        Time weight)
{
    if (job_id < 0 || job_id >= instance_.jobs_.size()) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_weight: "
                "invalid 'job_id'; "
                "job_id: " + std::to_string(job_id) + "; "
                "instance_.jobs_.size(): " + std::to_string(instance_.jobs_.size()) + ".");
    }
    if (weight < 0) {
        throw std::invalid_argument(
                "shopschedulingsolver::InstanceBuilder::set_job_weight: "
                "'weight' must be >= 0; "
                "weight: " + std::to_string(weight) + ".");
    }

    instance_.jobs_[job_id].weight = weight;
}

void InstanceBuilder::read(
        const std::string& instance_path,
        const std::string& format)
{
    // TODO
}

Instance InstanceBuilder::build()
{
    for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
        const Job& job = instance_.jobs_[job_id];
        instance_.number_of_operations_ += job.operations.size();
    }
    return std::move(instance_);
}
