#pragma once

#include "shopschedulingsolver/instance.hpp"

namespace shopschedulingsolver
{

class InstanceBuilder
{

public:

    /** Constructor. */
    InstanceBuilder() { }

    /** Read instance from a file. */
    void read(
            const std::string& instance_path,
            const std::string& format);

    /** Set the objective. */
    void set_objective(Objective objective) { this->instance_.objective_ = objective; }

    /**
     * Set the number of machines.
     *
     * This resets the jobs of the instance.
     */
    void set_number_of_machines(MachineId number_of_machines);

    /** Add a job. */
    JobId add_job();

    /** Add multiple jobs. */
    void add_jobs(JobId number_of_jobs);

    /** Set the processing-time of a job for a given machine. */
    OperationId add_operation(
            JobId job_id);

    /** Add a machine for an operation. */
    void add_operation_machine(
            JobId job_id,
            OperationId operation_id,
            MachineId machine_id,
            Time processing_time);

    /** Set the release date of a job. */
    void set_job_release_date(
            JobId job_id,
            Time release_date);

    /** Set the due date of a job. */
    void set_job_due_date(
            JobId job_id,
            Time due_date);

    /** Set the weight of a job. */
    void set_job_weight(
            JobId job_id,
            Time weight);

    /** Set shop type. */
    void set_operations_arbitrary_order(bool operations_arbitrary_order = true) { instance_.operations_arbitrary_order_ = operations_arbitrary_order; }

    /** Set no-wait property. */
    void set_no_wait(bool no_wait = true) { instance_.no_wait_ = no_wait; }

    /** Set no-idle property. */
    void set_no_idle(bool no_idle = true) { instance_.no_idle_ = no_idle; }

    /** Set blocking property. */
    void set_blocking(bool no_wait = true) { instance_.blocking_ = no_wait; }

    /** Set permutation property. */
    void set_permutation(bool no_wait = true) { instance_.permutation_ = no_wait; }

    /*
     * Build
     */

    /** Build. */
    Instance build();

private:

    /*
     * Private methods
     */

    void read_flow_shop(std::ifstream& file);

    void read_job_shop(std::ifstream& file);

    /*
     * Private attributes
     */

    /** Instance. */
    Instance instance_;

};

}
