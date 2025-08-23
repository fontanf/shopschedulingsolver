#pragma once

#include "shopschedulingsolver/solution.hpp"
#include "shopschedulingsolver/solution_builder.hpp"

#include "optimizationtools/utils/output.hpp"
#include "optimizationtools/utils/utils.hpp"

namespace shopschedulingsolver
{

inline optimizationtools::ObjectiveDirection objective_direction(
        Objective objective)
{
    return optimizationtools::ObjectiveDirection::Minimize;
}

struct Output: optimizationtools::Output
{
    /** Constructor. */
    Output(const Instance& instance):
        solution(SolutionBuilder().set_instance(instance).build()) { }


    Time time;

    Solution solution;

    Time makespan_bound = -1;

    Time total_flow_time_bound = -1;

    Time total_tardiness_bound = -1;

    Time maximum_lateness_bound = -1;


    virtual nlohmann::json to_json() const
    {
        return nlohmann::json {
            {"Solution", this->solution.to_json()},
            {"Time", this->time}
        };
    }

    virtual int format_width() const { return 11; }

    virtual void format(std::ostream& os) const
    {
        int width = format_width();
        os
            << std::setw(width) << std::left << "Time (s): " << time << std::endl
            ;
    }
};

using NewSolutionCallback = std::function<void(const Output&)>;

struct Parameters: optimizationtools::Parameters
{
    /** New solution callback. */
    NewSolutionCallback new_solution_callback = [](const Output&) { };
};

class AlgorithmFormatter
{

public:

    /** Constructor. */
    AlgorithmFormatter(
            const Instance& instance,
            const Parameters& parameters,
            Output& output):
        instance_(instance),
        parameters_(parameters),
        output_(output),
        os_(parameters.create_os()) { }

    /** Print the header. */
    void start(
            const std::string& algorithm_name);

    /** Print the header. */
    void print_header();

    /** Print current state. */
    void print(
            const std::string& s);

    /** Update the solution. */
    void update_solution(
            const Solution& solution,
            const std::string& s);

    /** Update makespan bound. */
    void update_makespan_bound(
            Time bound,
            const std::string& s);

    /** Update total flow time bound. */
    void update_total_flow_time_bound(
            Time bound,
            const std::string& s);

    /** Update total tardiness bound. */
    void update_total_tardiness_bound(
            Time bound,
            const std::string& s);

    /** Update maximum lateness bound. */
    void update_maximum_lateness_bound(
            Time bound,
            const std::string& s);

    /** Method to call at the end of the algorithm. */
    void end();

private:

    /** Instance. */
    const Instance& instance_;

    /** Parameters. */
    const Parameters& parameters_;

    /** Output. */
    Output& output_;

    /** Output stream. */
    std::unique_ptr<optimizationtools::ComposeStream> os_;

};

}
