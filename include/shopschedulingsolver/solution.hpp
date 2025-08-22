#pragma once

#include "shopschedulingsolver/instance.hpp"

namespace shopschedulingsolver
{

using SolutionOperationId = int64_t;

/**
 * Solution class.
 */
class Solution
{

public:

    /**
     * Structure for an operation scheduled in a solution.
     */
    struct Operation
    {
        MachineId machine_id = -1;

        JobId job_id = -1;

        OperationId operation_id = -1;

        OperationMachineId operation_machine_id = -1;

        Time start = -1;

        JobId machine_position = -1;

        OperationId job_position = -1;
    };

    /**
     * Structure for a machine schedule of a solution.
     */
    struct Machine
    {
        /** Operations scheduled on the machine. */
        std::vector<SolutionOperationId> solution_operations;

        Time start = -1;

        Time end = -1;
    };

    struct Job
    {
        std::vector<SolutionOperationId> operations;

        std::vector<SolutionOperationId> solution_operations;

        Time start = -1;

        Time end = -1;
    };

    /*
     * Getters
     */

    /** Get the instance of the solution. */
    const Instance& instance() const { return *this->instance_; }

    /** Get the number of operations in the solution. */
    SolutionOperationId number_of_operations() const { return this->operations_.size(); }

    /** Get a solution operation. */
    const Operation& operation(SolutionOperationId solution_operation_id) const { return this->operations_[solution_operation_id]; }

    /** Get a machine schedule. */
    const Machine& machine(MachineId machine_id) const { return this->machines_[machine_id]; }

    /** Get the info of a job in the solution. */
    const Job& job(JobId job_id) const { return this->jobs_[job_id]; }

    /** Check if the solution contains a given operation. */
    bool contains(JobId job_id, OperationId operation_id) const { return this->job(job_id).operations[operation_id] != -1; }

    /** Get the number of released dates violated. */
    JobId number_of_release_date_violations() const { return this->number_of_release_date_violations_; }

    /** Get the number of job overlaps. */
    OperationId number_of_job_overlaps() const { return this->number_of_job_overlaps_; }

    /** Get the number of machine overlaps. */
    OperationId number_of_machine_overlaps() const { return this->number_of_machine_overlaps_; }

    /** Get the number of precedence violations. */
    OperationId number_of_precedence_violations() const { return this->number_of_precedence_violations_; }

    /** Get no-wait feasibility. */
    bool no_wait() const { return no_wait_; }

    /** Get no-idle feasibility. */
    bool no_idle() const { return no_idle_; }

    /** Get blocking feasibility. */
    bool blocking() const { return blocking_; }

    /** Get permutation feasibility. */
    bool permutation() const { return permutation_; }

    /** Get the makespan of the solution. */
    Time makespan() const { return makespan_; }

    /** Get the total (weighted) flow time of the solution. */
    Time total_flow_time() const { return total_flow_time_; }

    /** Get the throughput of the solution. */
    Time throughput() const { return throughput_; }

    /** Get the total_tardiness of the solution. */
    Time total_tardiness() const { return total_tardiness_; }

    /** Check if the solution is feasible. */
    bool feasible() const;

    /** Check if the solution is strictly better than another solution. */
    bool strictly_better(const Solution& solution) const;

    /*
     * Export
     */

    /** Write the solution to a file. */
    void write(
            const std::string& certificate_path,
            const std::string& format) const;

    /** Export solution characteristics to a JSON structure. */
    nlohmann::json to_json() const;

    /** Write a formatted output of the instance to a stream. */
    void format(
            std::ostream& os,
            int verbosity_level = 1) const;

private:

    /*
     * Private attributes
     */

    /** Instance. */
    const Instance* instance_;

    /** Operations. */
    std::vector<Operation> operations_;

    /** Machines. */
    std::vector<Machine> machines_;

    /** Jobs. */
    std::vector<Job> jobs_;

    /** Number of released dates violated. */
    JobId number_of_release_date_violations_ = 0;

    /** Number of job overlaps. */
    OperationId number_of_job_overlaps_ = 0;

    /** Number of machine overlaps. */
    OperationId number_of_machine_overlaps_ = 0;

    /** Number of precedence violations. */
    OperationId number_of_precedence_violations_ = 0;

    /** No-wait feasibility. */
    bool no_wait_ = true;

    /** No-idle feasibility. */
    bool no_idle_ = true;

    /** Blocking feasibility. */
    bool blocking_ = true;

    /** Permutation feasibility. */
    bool permutation_ = true;

    /** Makespan. */
    Time makespan_ = 0;

    /** Total (weighted) flow time. */
    Time total_flow_time_ = 0;

    /** Troughput. */
    Time throughput_ = 0;

    /** Total (weighted) tardiness. */
    Time total_tardiness_ = 0;

    friend class SolutionBuilder;

};

}
