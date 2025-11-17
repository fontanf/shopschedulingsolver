#include "shopschedulingsolver/algorithm_formatter.hpp"
#include "shopschedulingsolver/instance_builder.hpp"
#include "shopschedulingsolver/algorithms/tree_search_pfss_makespan.hpp"
#include "shopschedulingsolver/algorithms/tree_search_pfss_tft.hpp"
#include "shopschedulingsolver/algorithms/milp_positional.hpp"
#include "shopschedulingsolver/algorithms/milp_disjunctive.hpp"
#ifdef OPTALCP_FOUND
#include "shopschedulingsolver/algorithms/constraint_programming_optalcp.hpp"
#endif

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>

using namespace shopschedulingsolver;
namespace po = boost::program_options;

void read_args(
        Parameters& parameters,
        const po::variables_map& vm)
{
    parameters.timer.set_sigint_handler();
    parameters.messages_to_stdout = true;
    if (vm.count("time-limit"))
        parameters.timer.set_time_limit(vm["time-limit"].as<double>());
    if (vm.count("verbosity-level"))
        parameters.verbosity_level = vm["verbosity-level"].as<int>();
    if (vm.count("log"))
        parameters.log_path = vm["log"].as<std::string>();
    parameters.log_to_stderr = vm.count("log-to-stderr");
    bool only_write_at_the_end = vm.count("only-write-at-the-end");
    if (!only_write_at_the_end) {

        std::string certificate_path;
        if (vm.count("certificate"))
            certificate_path = vm["certificate"].as<std::string>();

        std::string certificate_format;
        if (vm.count("certificate-format"))
            certificate_format = vm["certificate-format"].as<std::string>();

        std::string json_output_path;
        if (vm.count("output"))
            json_output_path = vm["output"].as<std::string>();

        parameters.new_solution_callback = [
            json_output_path,
            certificate_path,
            certificate_format](
                    const Output& output)
        {
            if (!json_output_path.empty())
                output.write_json_output(json_output_path);
            if (!certificate_path.empty())
                output.solution.write(certificate_path, certificate_format);
        };
    }
}

Output run(
        const Instance& instance,
        const po::variables_map& vm)
{
    Seed seed = 0;
    if (vm.count("seed"))
        seed = vm["seed"].as<Seed>();
    std::mt19937_64 generator(seed);
    //Solution solution = (vm.count("initial-solution"))?
    //    Solution(instance, vm["initial-solution"].as<std::string>()):
    //    Solution(instance);

    // Run algorithm.
    std::string algorithm = vm["algorithm"].as<std::string>();

    if (algorithm == "tree-search-pfss-makespan") {
        Parameters parameters;
        read_args(parameters, vm);
        return tree_search_pfss_makespan(instance, parameters);

    } else if (algorithm == "tree-search-pfss-tft") {
        Parameters parameters;
        read_args(parameters, vm);
        return tree_search_pfss_tft(instance, parameters);

    } else if (algorithm == "milp-positional") {
        MilpPositionalParameters parameters;
        read_args(parameters, vm);
        if (vm.count("solver"))
            parameters.solver = vm["solver"].as<mathoptsolverscmake::SolverName>();
        return milp_positional(instance, nullptr, parameters);

    } else if (algorithm == "milp-disjunctive") {
        MilpDisjunctiveParameters parameters;
        read_args(parameters, vm);
        if (vm.count("solver"))
            parameters.solver = vm["solver"].as<mathoptsolverscmake::SolverName>();
        return milp_disjunctive(instance, nullptr, parameters);

#ifdef OPTALCP_FOUND
    } else if (algorithm == "constraint-programming-optalcp") {
        Parameters parameters;
        read_args(parameters, vm);
        return constraint_programming_optalcp(instance, parameters);
#endif

    } else {
        throw std::invalid_argument(
                "Unknown algorithm \"" + algorithm + "\".");
    }

    return Output(instance);
}

int main(int argc, char *argv[])
{

    // Parse program options
    po::options_description desc("Allowed options");
    desc.add_options()
        (",h", "Produce help message")

        ("input,i", po::value<std::string>()->required(), "set input path")
        ("format,f", po::value<std::string>()->required(), "set input format")
        ("objective,", po::value<Objective>(), "set objective")
        ("operations-arbitrary-order,", po::value<bool>(), "set operations arbitrary order")
        ("no-wait,", po::value<bool>(), "set no-wait property")
        ("no-idle,", po::value<bool>(), "set no-idle property")
        ("blocking,", po::value<bool>(), "set blocking property")
        ("permutation,", po::value<bool>(), "set permutation property")

        ("algorithm,a", po::value<std::string>()->required(), "set algorithm")

        ("output,o", po::value<std::string>(), "set output path")
        ("certificate,c", po::value<std::string>(), "set certificate path")
        ("log,l", po::value<std::string>(), "set log path")
        ("time-limit,t", po::value<double>(), "set time limit in seconds")
        ("seed,s", po::value<Seed>(), "set seed (not used)")
        ("only-write-at-the-end,e", "only write output and certificate files at the end")
        ("verbosity-level,v", po::value<int>(), "set verbosity level")
        ("log-to-stderr,w", "write log in stderr")

        ("solver,", po::value<mathoptsolverscmake::SolverName>(), "set solver")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << desc << std::endl;;
        return 1;
    }
    try {
        po::notify(vm);
    } catch (const po::required_option& e) {
        std::cout << desc << std::endl;;
        return 1;
    }

    // Build instance.
    InstanceBuilder instance_builder;
    instance_builder.read(
            vm["input"].as<std::string>(),
            vm["format"].as<std::string>());
    if (vm.count("objective"))
        instance_builder.set_objective(vm["objective"].as<Objective>());
    if (vm.count("operations-arbitrary-order"))
        instance_builder.set_operations_arbitrary_order(vm["operations-arbitrary-order"].as<bool>());
    if (vm.count("no-wait"))
        instance_builder.set_no_wait(vm["no-wait"].as<bool>());
    if (vm.count("no-idle"))
        instance_builder.set_all_machines_no_idle(vm["no-idle"].as<bool>());
    if (vm.count("blocking"))
        instance_builder.set_blocking(vm["blocking"].as<bool>());
    if (vm.count("permutation"))
        instance_builder.set_permutation(vm["permutation"].as<bool>());
    Instance instance = instance_builder.build();

    // Run.
    Output output = run(instance, vm);

    // Write outputs.
    if (vm.count("certificate")) {
        std::string certificate_format = "";
        if (vm.count("certificate-format"))
            certificate_format = vm["certificate-format"].as<std::string>();
        output.solution.write(
                vm["certificate"].as<std::string>(),
                certificate_format);
    }
    if (vm.count("output"))
        output.write_json_output(vm["output"].as<std::string>());

    return 0;
}
