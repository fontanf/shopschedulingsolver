#include "shopschedulingsolver/instance.hpp"

#include <sstream>

using namespace shopschedulingsolver;

std::istream& shopschedulingsolver::operator>>(
        std::istream& in,
        Objective& objective)
{
    std::string token;
    std::getline(in, token);
    if (token == "makespan" || token == "Makespan") {
        objective = Objective::Makespan;
    } else if (token == "total-flow-time"
            || token == "tft"
            || token == "TFT"
            || token == "total flow time"
            || token == "Total flow time") {
        objective = Objective::TotalFlowTime;
    } else if (token == "throughput"
            || token == "Throughput") {
        objective = Objective::Throughput;
    } else if (token == "total-tardiness"
            || token == "tt"
            || token == "TT"
            || token == "total tardiness"
            || token == "Total tardiness") {
        objective = Objective::TotalTardiness;
    } else  {
        throw std::invalid_argument(
                FUNC_SIGNATURE + ": "
                "invalid input; "
                "in: " + token + ".");
        in.setstate(std::ios_base::failbit);
    }
    return in;
}

std::ostream& shopschedulingsolver::operator<<(
        std::ostream& os,
        Objective objective)
{
    switch (objective) {
    case Objective::Makespan: {
        os << "Makespan";
        break;
    } case Objective::TotalFlowTime: {
        os << "Total flow time";
        break;
    } case Objective::Throughput: {
        os << "Throughput";
        break;
    } case Objective::TotalTardiness: {
        os << "Total tardiness";
        break;
    }
    }
    return os;
}

void Instance::write(
        const std::string& instance_path,
        const std::string& format) const
{
    if (instance_path.empty())
        return;
    std::ofstream file{instance_path};
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + instance_path + "\".");
    }

    nlohmann::json json;

    std::stringstream objective_ss;
    objective_ss << this->objective();
    json["objective"] = objective_ss.str();
    json["operations_arbitrary_order"] = this->operations_arbitrary_order();
    json["no_wait"] = this->no_wait();
    json["blocking"] = this->blocking();
    json["permutation"] = this->permutation();
    for (MachineId machine_id = 0;
            machine_id < this->number_of_machines();
            ++machine_id) {
        const Machine& machine = this->machine(machine_id);
        json["machines"][machine_id]["no_idle"] = machine.no_idle;
    }
    for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
        const Job& job = this->job(job_id);
        json["jobs"][job_id]["release_date"] = job.release_date;
        json["jobs"][job_id]["due_date"] = job.due_date;
        json["jobs"][job_id]["weight"] = job.weight;
        for (OperationId operation_id = 0;
                operation_id < (OperationId)job.operations.size();
                ++operation_id) {
            const Operation& operation = job.operations[operation_id];
            for (AlternativeId alternative_id = 0;
                    alternative_id < (AlternativeId)operation.machines.size();
                    ++alternative_id) {
                const Alternative& alternative = operation.machines[alternative_id];
                json["jobs"][job_id]["operations"][operation_id]["alternatives"][alternative_id]["machine"] = alternative.machine_id;
                json["jobs"][job_id]["operations"][operation_id]["alternatives"][alternative_id]["processing_time"] = alternative.processing_time;
            }
        }
    }

    file << std::setw(4) << json << std::endl;
}

std::ostream& Instance::format(
        std::ostream& os,
        int verbosity_level) const
{
    if (verbosity_level >= 1) {
        os
            << "Number of jobs:              " << this->number_of_jobs() << std::endl
            << "Number of machines:          " << this->number_of_machines() << std::endl
            << "Number of operations:        " << this->number_of_operations() << std::endl
            << "Objective:                   " << this->objective() << std::endl
            << "Operations arbitrary order:  " << this->operations_arbitrary_order() << std::endl
            << "No-wait:                     " << this->no_wait() << std::endl
            << "Mixed no-idle:               " << this->mixed_no_idle() << std::endl
            << "No-idle:                     " << this->no_idle() << std::endl
            << "Blocking:                    " << this->blocking() << std::endl
            << "Permutation:                 " << this->permutation() << std::endl
            << "Flow shop:                   " << this->flow_shop() << std::endl
            << "Flexible:                    " << this->flexible() << std::endl
            ;
    }

    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "# op."
            << std::setw(12) << "# mop."
            << std::setw(12) << "Mean pt"
            << std::setw(12) << "Rel. date"
            << std::setw(12) << "Due date"
            << std::setw(12) << "Weight"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "-----"
            << std::setw(12) << "------"
            << std::setw(12) << "-------"
            << std::setw(12) << "---------"
            << std::setw(12) << "--------"
            << std::setw(12) << "------"
            << std::endl;
        for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
            const Job& job = this->job(job_id);
            os
                << std::setw(12) << job_id
                << std::setw(12) << job.operations.size()
                << std::setw(12) << job.number_of_machine_operations
                << std::setw(12) << job.mean_processing_time
                << std::setw(12) << job.release_date
                << std::setw(12) << job.due_date
                << std::setw(12) << job.weight
                << std::endl;
        }
    }

    if (verbosity_level >= 2) {
        os << std::right << std::endl
            << std::setw(12) << "Machine"
            << std::setw(12) << "# op."
            << std::setw(12) << "No-idle"
            << std::endl
            << std::setw(12) << "-------"
            << std::setw(12) << "-----"
            << std::setw(12) << "-------"
            << std::endl;
        for (MachineId machine_id = 0;
                machine_id < this->number_of_machines();
                ++machine_id) {
            const Machine& machine = this->machine(machine_id);
            os
                << std::setw(12) << machine_id
                << std::setw(12) << machine.operations.size()
                << std::setw(12) << machine.no_idle
                << std::endl;
        }
    }

    if (verbosity_level >= 3) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "Operation"
            << std::setw(12) << "# machines"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "---------"
            << std::setw(12) << "----------"
            << std::endl;
        for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
            const Job& job = this->job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                os
                    << std::setw(12) << job_id
                    << std::setw(12) << operation_id
                    << std::setw(12) << operation.machines.size()
                    << std::endl;
            }
        }
    }

    if (verbosity_level >= 4) {
        os << std::right << std::endl
            << std::setw(12) << "Job"
            << std::setw(12) << "Operation"
            << std::setw(12) << "Mach. op."
            << std::setw(12) << "Machine"
            << std::setw(12) << "Proc. time"
            << std::endl
            << std::setw(12) << "---"
            << std::setw(12) << "---------"
            << std::setw(12) << "---------"
            << std::setw(12) << "-------"
            << std::setw(12) << "----------"
            << std::endl;
        for (JobId job_id = 0; job_id < this->number_of_jobs(); ++job_id) {
            const Job& job = this->job(job_id);
            for (OperationId operation_id = 0;
                    operation_id < (OperationId)job.operations.size();
                    ++operation_id) {
                const Operation& operation = job.operations[operation_id];
                for (AlternativeId alternative_id = 0;
                        alternative_id < (AlternativeId)operation.machines.size();
                        ++alternative_id) {
                    const Alternative& alternative = operation.machines[alternative_id];
                    os
                        << std::setw(12) << job_id
                        << std::setw(12) << operation_id
                        << std::setw(12) << alternative_id
                        << std::setw(12) << alternative.machine_id
                        << std::setw(12) << alternative.processing_time
                        << std::endl;
                }
            }
        }
    }

    return os;
}
