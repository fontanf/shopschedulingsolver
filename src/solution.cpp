#include "shopschedulingsolver/solution.hpp"

using namespace shopschedulingsolver;

bool Solution::feasible() const
{
    return (this->number_of_release_date_violations() == 0
        && this->number_of_job_overlaps() == 0
        && this->number_of_machine_overlaps() == 0
        && this->number_of_precedence_violations() == 0
        && this->number_of_operations() == instance().number_of_jobs());
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
    // TODO
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
    // TODO
}
