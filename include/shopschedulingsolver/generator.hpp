#pragma once

#include "shopschedulingsolver/instance.hpp"

#include <random>

namespace shopschedulingsolver
{

struct GenerateInput
{
    Objective objective = Objective::Makespan;
    bool operations_arbitrary_order = false;
    bool blocking = false;
    bool no_wait = false;
    bool no_idle = false;
    bool flow_shop = false;
    bool permutation = false;
    MachineId number_of_machine_groups = 3;
    MachineId number_of_machines_per_group = 1;
    JobId number_of_jobs = 5;
    MachineId number_of_operations_per_job = 3;
    Time processing_times_range = 100;
    Time weights_range = 1;
    double due_date_tightness_factor = 3.0;
};

Instance generate(
        const GenerateInput& input,
        std::mt19937_64& generator);

}
