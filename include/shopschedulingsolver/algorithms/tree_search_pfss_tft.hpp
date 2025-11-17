#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

namespace shopschedulingsolver
{

Output tree_search_pfss_tft(
        const Instance& instance,
        const Parameters& parameters = {});

}
