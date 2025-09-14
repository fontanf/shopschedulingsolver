/**
 * Positional MILP model
 *
 * This is the positional model for permutation flow shop problems.
 *
 * The model is described in:
 *
 *     "Matheuristic algorithms for minimizing total tardiness in the m-machine
 *     flow-shop scheduling problem" (Ta et al, 2018)
 *     https://doi.org/10.1007/s10845-015-1046-4
 *
 * The model described in this paper is for the unweighted total tardiness
 * objective.
 *
 * It is adapted here to handle makespan and unweighted total flow time
 * objective as well, and to no-idle, no-wait and blocking constraints.
 */

#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

#include "mathoptsolverscmake/milp.hpp"

namespace shopschedulingsolver
{

struct MilpPositionalParameters: Parameters
{
    mathoptsolverscmake::SolverName solver = mathoptsolverscmake::SolverName::Highs;


    virtual int format_width() const override { return 37; }

    virtual void format(std::ostream& os) const override
    {
        Parameters::format(os);
        int width = format_width();
        os
            << std::setw(width) << std::left << "Solver: " << solver << std::endl
            ;
    }

    virtual nlohmann::json to_json() const override
    {
        nlohmann::json json = Parameters::to_json();
        json.merge_patch({
                //{"Solver", solver},
                });
        return json;
    }
};

Output milp_positional(
        const Instance& instance,
        const Solution* initial_solution = NULL,
        const MilpPositionalParameters& parameters = {});

}
