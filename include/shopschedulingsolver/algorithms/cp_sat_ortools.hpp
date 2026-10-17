#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

namespace shopschedulingsolver
{

Output cp_sat_ortools(
        const Instance& instance,
        const Parameters& parameters = {});

}
