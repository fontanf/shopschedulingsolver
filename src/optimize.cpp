#include "shopschedulingsolver/optimize.hpp"

using namespace shopschedulingsolver;

Output shopschedulingsolver::optimize(
        const Instance& instance,
        const OptimizeParameters& parameters)
{
    Output output;
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.print_header();

    // TODO

    algorithm_formatter.end();
    return output;
}
