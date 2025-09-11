#pragma once

#include "shopschedulingsolver/solution.hpp"

namespace shopschedulingsolver
{

class SolutionBuilder
{

public:

    /** Constructor. */
    SolutionBuilder() { }

    /** Set the instance of the solution. */
    SolutionBuilder& set_instance(const Instance& instance);

    /** Add an operation at the end of a machine. */
    void append_operation(
            JobId job_id,
            OperationId operation_id,
            OperationMachineId operation_machine_id,
            Time start);

    void sort_machines();

    void sort_jobs();

    void from_permutation(
            const std::vector<JobId>& job_ids);

    /** Read a solution from a file. */
    void read(
            const std::string& certificate_path,
            const std::string& format = "default");

    /*
     * Build
     */

    /** Build. */
    Solution build();

private:

    /*
     * Private methods
     */

    /*
     * Private attributes
     */

    /** Solution. */
    Solution solution_;

};

}
