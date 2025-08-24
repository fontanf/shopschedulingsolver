#include "shopschedulingsolver/algorithm_formatter.hpp"

#include "optimizationtools/utils/utils.hpp"

#include <iomanip>

using namespace shopschedulingsolver;

void AlgorithmFormatter::start(
        const std::string& algorithm_name)
{
    output_.json["Parameters"] = parameters_.to_json();

    if (parameters_.verbosity_level == 0)
        return;
    *os_
        << "====================================" << std::endl
        << "        ShopSchedulingSolver        " << std::endl
        << "====================================" << std::endl
        << std::endl
        << "Instance" << std::endl
        << "--------" << std::endl;
    output_.solution.instance().format(*os_, parameters_.verbosity_level);
    *os_
        << std::endl
        << "Algorithm" << std::endl
        << "---------" << std::endl
        << algorithm_name << std::endl
        << std::endl
        << "Parameters" << std::endl
        << "----------" << std::endl;
    parameters_.format(*os_);
}

void AlgorithmFormatter::print_header()
{
    if (parameters_.verbosity_level == 0)
        return;
    *os_ << std::endl << std::right;
    switch (instance_.objective()) {
    case Objective::Makespan: {
        *os_
                << std::setw(12) << "Time"
                << std::setw(12) << "Makespan"
                << std::setw(32) << "Comment"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "--------"
                << std::setw(32) << "-------"
                << std::endl;
        break;
    } case Objective::TotalFlowTime: {
        *os_
                << std::setw(12) << "Time"
                << std::setw(12) << "TFT"
                << std::setw(12) << "Makespan"
                << std::setw(32) << "Comment"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "---"
                << std::setw(12) << "---------"
                << std::setw(32) << "-------"
                << std::endl;
        break;
    } case Objective::Throughput: {
        *os_
                << std::setw(12) << "Time"
                << std::setw(12) << "Throughput"
                << std::setw(12) << "Makespan"
                << std::setw(32) << "Comment"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "----------"
                << std::setw(12) << "---------"
                << std::setw(32) << "-------"
                << std::endl;
        break;
    } case Objective::TotalTardiness: {
        *os_
                << std::setw(12) << "Time"
                << std::setw(12) << "TT"
                << std::setw(12) << "Makespan"
                << std::setw(32) << "Comment"
                << std::endl
                << std::setw(12) << "----"
                << std::setw(12) << "--"
                << std::setw(12) << "---------"
                << std::setw(32) << "-------"
                << std::endl;
        break;
    }
    }
}

void AlgorithmFormatter::print(
        const std::string& s)
{
    if (parameters_.verbosity_level == 0)
        return;
    std::streamsize precision = std::cout.precision();

    switch (instance_.objective()) {
    case Objective::Makespan: {
        *os_
                << std::setw(12) << std::fixed << std::setprecision(3) << output_.time << std::defaultfloat << std::setprecision(precision)
                << std::setw(12) << output_.solution.makespan()
                << std::setw(32) << s
                << std::endl;
        break;
    } case Objective::TotalFlowTime: {
        *os_
                << std::setw(12) << std::fixed << std::setprecision(3) << output_.time << std::defaultfloat << std::setprecision(precision)
                << std::setw(12) << output_.solution.total_flow_time()
                << std::setw(12) << output_.solution.makespan()
                << std::setw(32) << s
                << std::endl;
        break;
    } case Objective::Throughput: {
        *os_
                << std::setw(12) << std::fixed << std::setprecision(3) << output_.time << std::defaultfloat << std::setprecision(precision)
                << std::setw(12) << output_.solution.throughput()
                << std::setw(12) << output_.solution.makespan()
                << std::setw(32) << s
                << std::endl;
        break;
    } case Objective::TotalTardiness: {
        *os_
                << std::setw(12) << std::fixed << std::setprecision(3) << output_.time << std::defaultfloat << std::setprecision(precision)
                << std::setw(12) << output_.solution.total_tardiness()
                << std::setw(12) << output_.solution.makespan()
                << std::setw(32) << s
                << std::endl;
        break;
    }
    }
}

void AlgorithmFormatter::update_solution(
        const Solution& solution,
        const std::string& s)
{
    if (solution.strictly_better(output_.solution)) {
        output_.time = parameters_.timer.elapsed_time();
        output_.solution = solution;
        print(s);
        output_.json["IntermediaryOutputs"].push_back(output_.to_json());
        parameters_.new_solution_callback(output_);
    }
}

void AlgorithmFormatter::update_makespan_bound(
        Time bound,
        const std::string& s)
{
    if (optimizationtools::is_bound_strictly_better(
            objective_direction(this->output_.solution.instance().objective()),
            output_.makespan_bound,
            bound)) {
        output_.time = parameters_.timer.elapsed_time();
        output_.makespan_bound = bound;
        print(s);
        output_.json["IntermediaryOutputs"].push_back(output_.to_json());
        parameters_.new_solution_callback(output_);
    }
}

void AlgorithmFormatter::end()
{
    output_.time = parameters_.timer.elapsed_time();
    output_.json["Output"] = output_.to_json();

    if (parameters_.verbosity_level == 0)
        return;
    *os_
        << std::endl
        << "Final statistics" << std::endl
        << "----------------" << std::endl;
    output_.format(*os_);
    *os_
        << std::endl
        << "Solution" << std::endl
        << "--------" << std::endl;
    output_.solution.format(*os_, parameters_.verbosity_level);
}
