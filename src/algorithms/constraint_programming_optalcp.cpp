#include "shopschedulingsolver/algorithms/constraint_programming_optalcp.hpp"

using namespace shopschedulingsolver;

Output shopschedulingsolver::constraint_programming_optalcp(
        const Instance& instance,
        const Parameters& parameters)
{
    Output output(instance);
    AlgorithmFormatter algorithm_formatter(instance, parameters, output);
    algorithm_formatter.start("Constraint programming (OptalCP)");

    algorithm_formatter.print_header();

    char instance_path[L_tmpnam];
    tmpnam(instance_path);
    instance.write(instance_path, "");

    char parameters_path[L_tmpnam];
    tmpnam(parameters_path);
    parameters.to_json();
    nlohmann::json parameters_json = parameters.to_json();
    std::ofstream parameters_file{parameters_path};
    parameters_file << std::setw(4) << parameters_json << std::endl;
    parameters_file.close();

    // Run.
    char output_path[L_tmpnam];
    tmpnam(output_path);
    char certificate_path[L_tmpnam];
    tmpnam(certificate_path);
    std::string command = (
            "node ./install/bin/constraint_programming_optalcp.mjs "
            " \"" + std::string(instance_path) + "\" "
            " \"" + parameters_path + "\" "
            + " \"" + output_path + "\" "
            + " \"" + certificate_path + "\" ");
    std::system(command.c_str());

    // Retrieve solution.
    if (std::ifstream(certificate_path).good()) {
        SolutionBuilder solution_builder;
        solution_builder.set_instance(instance);
        solution_builder.read(certificate_path);
        solution_builder.sort_machines();
        solution_builder.sort_jobs();
        Solution solution = solution_builder.build();
        //solution.format(std::cout, 3);
        algorithm_formatter.update_solution(solution, "");
        std::remove(certificate_path);
    }

    std::ifstream output_file{output_path};
    if (!output_file.good()) {
        throw std::runtime_error(
                FUNC_SIGNATURE + ": "
                "unable to open file \"" + std::string(output_path) + "\".");
    }
    nlohmann ::json output_json;
    output_file >> output_json;

    // Retrieve bound.
    Time bound = output_json["bound"];
    switch (instance.objective()) {
    case Objective::Makespan: {
        algorithm_formatter.update_makespan_bound(bound, "");
        break;
    } case Objective::TotalFlowTime: {
        algorithm_formatter.update_total_flow_time_bound(bound, "");
        break;
    } case Objective::Throughput: {
        algorithm_formatter.update_throughput_bound(bound, "");
        break;
    } case Objective::TotalTardiness: {
        algorithm_formatter.update_total_tardiness_bound(bound, "");
        break;
    }
    }

    // Remove temporary files.
    std::remove(instance_path);
    std::remove(parameters_path);
    std::remove(output_path);

    algorithm_formatter.end();
    return output;
}
