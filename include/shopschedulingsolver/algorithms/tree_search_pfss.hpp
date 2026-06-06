#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

namespace shopschedulingsolver
{

struct TreeSearchPfssParameters: Parameters
{
    Counter minimum_size_of_the_queue = 1;
    Counter maximum_size_of_the_queue = 100000000;

    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        //os
        //    << std::setw(width) << std::left << "Max. # of iterations: " << maximum_number_of_iterations << std::endl
        //    << std::setw(width) << std::left << "Max. # of iterations without impr.:  " << maximum_number_of_iterations_without_improvement << std::endl
        //    ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        //json.merge_patch({
        //        {"MaximumNumberOfIterations", maximum_number_of_iterations},
        //        {"MaximumNumberOfIterationsWithoutImprovement", maximum_number_of_iterations_without_improvement},
        //        });
        return json;
    }
};

Output tree_search_pfss(
        const Instance& instance,
        const TreeSearchPfssParameters& parameters = {});

}
