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

    /** Read a solution from a file. */
    void read(const std::string& certificate_path);

    /** Add an operation at the end of a machine. */
    void append_operation(
            JobId job_id,
            OperationId operation_id,
            OperationMachineId operation_machine_id,
            Time start);

    void sort_machines();

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
