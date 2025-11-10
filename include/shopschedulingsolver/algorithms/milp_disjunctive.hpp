/**
 * Disjunctive MILP model
 *
 * This model handles all variants except the permutation flow shop.
 *
 * The model of no-wait property corresponds to model 3 in:
 *
 *     "Modeling and scheduling no-wait open shop problems" (Naderi et Zandieh,
 *     2014)
 *     https://doi.org/10.1016/j.ijpe.2014.06.011
 *
 * For model of flexible property corresponds to model 5 in:
 *
 *     "Mathematical modelling and a meta-heuristic for flexible job shop
 *     scheduling" * (Roshanaei et al., 2013)
 *     https://doi.org/10.1080/00207543.2013.827806
 */

#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

#include "mathoptsolverscmake/milp.hpp"

namespace shopschedulingsolver
{

struct MilpDisjunctiveParameters: Parameters
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

Output milp_disjunctive(
        const Instance& instance,
        const Solution* initial_solution = NULL,
        const MilpDisjunctiveParameters& parameters = {});

void write_mps(
        const Instance& instance,
        mathoptsolverscmake::SolverName solver,
        const std::string& output_path);

}
