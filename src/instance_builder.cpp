#include "shopschedulingsolver/instance_builder.hpp"

#include <sstream>

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

void InstanceBuilder::set_machine_no_idle(
        MachineId machine_id,
        bool no_idle)
{
    this->instance_.machines_[machine_id].no_idle = no_idle;
}

void InstanceBuilder::set_all_machines_no_idle(
        bool no_idle)
{
    for (MachineId machine_id = 0;
            machine_id < this->instance_.number_of_machines();
            ++machine_id) {
        instance_.machines_[machine_id].no_idle = no_idle;
    }
}

JobId InstanceBuilder::add_job()
{
    JobId job_id = instance_.jobs_.size();
    Job job;
    instance_.jobs_.push_back(job);
    return job_id;
}

void InstanceBuilder::add_jobs(JobId number_of_jobs)
{
    instance_.jobs_.insert(instance_.jobs_.end(), number_of_jobs, Job());
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

void InstanceBuilder::add_alternative(
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

    Alternative alternative;
    alternative.machine_id = machine_id;
    alternative.processing_time = processing_time;
    operation.machines.push_back(alternative);
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
    std::ifstream file(instance_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + instance_path + "\".");
    }

    if (format == "" || format == "json") {
        read_json(file);
    } else if (format == "flow-shop") {
        read_flow_shop(file);
    } else if (format == "flow-shop-vallada2008" || format == "vallada2008") {
        read_flow_shop_vallada2008(file);
    } else if (format == "job-shop") {
        read_job_shop(file);
    } else if (format == "flexible-job-shop") {
        read_flexible_job_shop(file);
    } else {
        throw std::invalid_argument(
                "Unknown instance format \"" + format + "\".");
    }
    file.close();
}

void InstanceBuilder::read_json(std::ifstream& file)
{
    nlohmann ::json j;
    file >> j;

    if (j.contains("objective")) {
        std::stringstream objective_ss;
        objective_ss << std::string(j["objective"]);
        Objective objective;
        objective_ss >> objective;
        set_objective(objective);
    }

    if (j.contains("permutation"))
        set_permutation(j["permutation"]);
    if (j.contains("operations_arbitrary_order"))
        set_operations_arbitrary_order(j["operations_arbitrary_order"]);
    if (j.contains("no_wait"))
        set_no_wait(j["no_wait"]);
    if (j.contains("blocking"))
        set_blocking(j["blocking"]);

    // Read machines.
    set_number_of_machines(j["machines"].size());
    MachineId machine_id = 0;
    for (const auto& json_machine: j["machines"]) {
        if (json_machine.contains("no_idle"))
            set_machine_no_idle(machine_id, json_machine["no_idle"]);
        machine_id++;
    }

    // Read jobs.
    for (const auto& json_job: j["jobs"]) {
        JobId job_id = add_job();
        if (json_job.contains("release_date"))
            set_job_release_date(job_id, json_job["release_date"]);
        if (json_job.contains("due_date"))
            set_job_due_date(job_id, json_job["due_date"]);
        if (json_job.contains("weight"))
            set_job_weight(job_id, json_job["weight"]);
        for (const auto& json_operation: json_job["operations"]) {
            OperationId operation_id = add_operation(job_id);
            for (const auto& json_alternative: json_operation["alternatives"]) {
                add_alternative(
                        job_id,
                        operation_id,
                        json_alternative["machine"],
                        json_alternative["processing_time"]);
            }
        }
    }
}

void InstanceBuilder::read_flow_shop(std::ifstream& file)
{
    JobId number_of_jobs = -1;
    MachineId number_of_machines = -1;
    file >> number_of_jobs;
    file >> number_of_machines;
    this->add_jobs(number_of_jobs);
    this->set_number_of_machines(number_of_machines);

    Time processing_time = -1;
    for (MachineId machine_id = 0;
            machine_id < number_of_machines;
            ++machine_id) {
        for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
            file >> processing_time;
            OperationId operation_id = this->add_operation(job_id);
            this->add_alternative(
                    job_id,
                    operation_id,
                    machine_id,
                    processing_time);
        }
    }

    this->set_objective(Objective::Makespan);
}

void InstanceBuilder::read_flow_shop_vallada2008(std::ifstream& file)
{
    JobId number_of_jobs;
    MachineId number_of_machines;
    file >> number_of_jobs >> number_of_machines;
    this->set_number_of_machines(number_of_machines);
    this->add_jobs(number_of_jobs);

    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        Time processing_time = -1;
        MachineId machine_id_tmp = -1;
        for (MachineId machine_id = 0;
                machine_id < number_of_machines;
                ++machine_id) {
            file >> machine_id_tmp >> processing_time;
            OperationId operation_id = this->add_operation(job_id);
            this->add_alternative(
                    job_id,
                    operation_id,
                    machine_id,
                    processing_time);
        }
    }

    std::string tmp;
    file >> tmp;
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        Time due_date = -1;
        file >> tmp >> due_date >> tmp >> tmp;
        set_job_due_date(job_id, due_date);
    }

    this->set_objective(Objective::TotalTardiness);
}

void InstanceBuilder::read_job_shop(std::ifstream& file)
{
    JobId number_of_jobs = -1;
    MachineId number_of_machines = -1;
    file >> number_of_jobs;
    file >> number_of_machines;
    this->add_jobs(number_of_jobs);
    this->set_number_of_machines(number_of_machines);

    MachineId machine_id = -1;
    Time processing_time = -1;
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        for (OperationId operation_id = 0;
                operation_id < number_of_machines;
                ++operation_id) {
            file >> machine_id >> processing_time;
            this->add_operation(job_id);
            this->add_alternative(
                    job_id,
                    operation_id,
                    machine_id,
                    processing_time);
        }
    }

    this->set_objective(Objective::Makespan);
}

void InstanceBuilder::read_flexible_job_shop(std::ifstream& file)
{
    JobId number_of_jobs = -1;
    MachineId number_of_machines = -1;
    std::string tmp;
    file >> number_of_jobs;
    file >> number_of_machines;
    file >> tmp;
    this->add_jobs(number_of_jobs);
    this->set_number_of_machines(number_of_machines);

    OperationId number_of_operations = -1;
    OperationId number_of_alternatives = -1;
    MachineId machine_id = -1;
    Time processing_time = -1;
    for (JobId job_id = 0; job_id < number_of_jobs; ++job_id) {
        file >> number_of_operations;
        for (OperationId operation_id = 0;
                operation_id < number_of_operations;
                ++operation_id) {
            this->add_operation(job_id);
            file >> number_of_alternatives;
            for (AlternativeId alternative_id = 0;
                    alternative_id < number_of_alternatives;
                    ++alternative_id) {
                file >> machine_id >> processing_time;
                this->add_alternative(
                        job_id,
                        operation_id,
                        machine_id - 1,
                        processing_time);
            }
        }
    }

    this->set_objective(Objective::Makespan);
}

Instance InstanceBuilder::build()
{
    for (JobId job_id = 0; job_id < instance_.number_of_jobs(); ++job_id) {
        const Job& job = instance_.jobs_[job_id];
        instance_.number_of_operations_ += job.operations.size();
    }

    // Is flow shop?
    this->instance_.flow_shop_ = true;
    if (this->instance_.operations_arbitrary_order())
        this->instance_.flow_shop_ = false;
    this->instance_.flexible_ = false;
    for (JobId job_id = 0; job_id < this->instance_.number_of_jobs(); ++job_id) {
        Job& job = this->instance_.jobs_[job_id];
        // Compute flow_shop_.
        if (job.operations.size() != instance_.number_of_machines())
            this->instance_.flow_shop_ = false;
        for (OperationId operation_id = 0;
                operation_id < (OperationId)job.operations.size();
                ++operation_id) {
            const Operation& operation = job.operations[operation_id];
            // Compute flexible_.
            if (operation.machines.size() != 1)
                this->instance_.flexible_ = true;
            for (AlternativeId alternative_id = 0;
                    alternative_id < (AlternativeId)operation.machines.size();
                    ++alternative_id) {
                const Alternative& alternative = operation.machines[alternative_id];
                job.number_of_machine_operations++;
                job.mean_processing_time += alternative.processing_time;
                // Update machines_.
                MachineOperation machine_operation;
                machine_operation.job_id = job_id;
                machine_operation.operation_id = operation_id;
                machine_operation.alternative_id = alternative_id;
                this->instance_.machines_[alternative.machine_id].operations.push_back(machine_operation);
                // Compute flow_shop_.
                if (alternative.machine_id != operation_id)
                    this->instance_.flow_shop_ = false;
            }
        }

        job.mean_processing_time /= job.number_of_machine_operations;
    }

    instance_.no_idle_ = true;
    instance_.mixed_no_idle_ = false;
    for (MachineId machine_id = 0;
            machine_id < this->instance_.number_of_machines();
            ++machine_id) {
        const Machine& machine = this->instance_.machine(machine_id);
        if (machine.no_idle) {
            this->instance_.mixed_no_idle_ = true;
        } else {
            this->instance_.no_idle_ = false;
        }
    }

    return std::move(instance_);
}
