#pragma once

#include "optimizationtools/utils/output.hpp"

#include <cstdint>
#include <vector>
#include <iostream>

namespace shopschedulingsolver
{

using MachineId = int64_t;
using JobId = int64_t;
using OperationId = int64_t;
using OperationMachineId = int64_t;
using Time = int64_t;
using Seed = int64_t;
using Counter = int64_t;

enum class Objective
{
    Makespan,
    TotalFlowTime,
    Throughput,
    TotalTardiness,
};

std::istream& operator>>(
        std::istream& in,
        Objective& objective);

std::ostream& operator<<(
        std::ostream& os,
        Objective objective);

struct MachineOperation
{
    JobId job_id = -1;

    OperationId operation_id = -1;
};

struct Machine
{
    std::vector<MachineOperation> operations;
};

struct OperationMachine
{
    MachineId machine_id = -1;

    Time processing_time = 1;
};

/**
 * Structure for a job operation.
 */
struct Operation
{
    /** Machines that can perform the operation. */
    std::vector<OperationMachine> machines;
};

/**
 * Structure for a job.
 */
struct Job
{
    /** Operations of the job. */
    std::vector<Operation> operations;

    /** Release date. */
    Time release_date = 0;

    /** Due date. */
    Time due_date = -1;

    /** Weight. */
    Time weight = 1;


    /** Number of machine operations. */
    OperationId number_of_machine_operations = 0;

    /** Mean processing time. */
    double mean_processing_time = 0;
};

/**
 * Instance class.
 */
class Instance
{

public:

    /*
     * Getters
     */

    /** Get the objective. */
    Objective objective() const { return objective_; }

    /** Get the number of machines. */
    MachineId number_of_machines() const { return machines_.size(); }

    /** Get a machine. */
    const Machine& machine(MachineId machine_id) const { return machines_[machine_id]; }

    /** Get the number of jobs. */
    JobId number_of_jobs() const { return jobs_.size(); }

    /** Get a job. */
    const Job& job(JobId job_id) const { return jobs_[job_id]; }

    /** Get the number of operations. */
    OperationId number_of_operations() const { return number_of_operations_; }

    /** Get the shop type. */
    bool operations_arbitrary_order() const { return operations_arbitrary_order_; }

    /** Return 'true' if the instance has the no-wait property. */
    bool no_wait() const { return no_wait_; }

    /** Return 'true' if the instance has the no-idle property. */
    bool no_idle() const { return no_idle_; }

    /** Return 'true' if the instance has the blocking property. */
    bool blocking() const { return blocking_; }

    /** Return 'true' if the instance has the permutation property. */
    bool permutation() const { return permutation_; }

    /** Get the shop type. */
    bool flow_shop() const { return flow_shop_; }

    /** Get the flexible property. */
    bool flexible() const { return flexible_; }

    /*
     * Export
     */

    /** Print the instance into a stream. */
    std::ostream& format(
            std::ostream& os,
            int verbosity_level = 1) const;

    /** Write the instance to a file. */
    void write(
            const std::string& instance_path,
            const std::string& format) const;

private:

    /*
     * Private methods
     */

    /** Create an instance manually. */
    Instance() { }

    /*
     * Private attributes
     */

    /** Objective. */
    Objective objective_;

    /** Machines. */
    std::vector<Machine> machines_;

    /** Jobs. */
    std::vector<Job> jobs_;

    /** Number of operations. */
    OperationId number_of_operations_ = 0;

    /** Flow shop / job shop, or open shop. */
    bool operations_arbitrary_order_ = false;

    /** No-wait property. */
    bool no_wait_ = false;

    /** No-idle property. */
    bool no_idle_ = false;

    /** Blocking property. */
    bool blocking_ = false;

    /** Permutation property. */
    bool permutation_ = false;


    /** Flow shop. */
    bool flow_shop_ = false;

    /** Flexible. */
    bool flexible_ = false;

    friend class InstanceBuilder;

};

}
