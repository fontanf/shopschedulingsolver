#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

namespace shopschedulingsolver
{

Output constraint_programming_optalcp(
        const Instance& instance,
        const Parameters& parameters = {});

}
