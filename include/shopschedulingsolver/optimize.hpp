#pragma once

#include "shopschedulingsolver/instance.hpp"
#include "shopschedulingsolver/algorithm_formatter.hpp"

namespace shopschedulingsolver
{

struct OptimizeOutput: shopschedulingsolver::Output
{
};

struct OptimizeParameters: shopschedulingsolver::Parameters
{
};

Output optimize(
        const Instance& instance,
        const OptimizeParameters& parameters = {});

}
