#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

namespace shopschedulingsolver
{

Output tree_search_pfss_tct(
        const Instance& instance,
        const Parameters& parameters = {});

}
