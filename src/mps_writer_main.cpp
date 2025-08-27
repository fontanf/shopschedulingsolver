#include "shopschedulingsolver/instance_builder.hpp"
#include "shopschedulingsolver/algorithms/milp_disjunctive.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>

using namespace shopschedulingsolver;
namespace po = boost::program_options;

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
        ("solver,", po::value<mathoptsolverscmake::SolverName>()->required(), "set solver")
        ("output,o", po::value<std::string>()->required(), "set output path")
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
    Instance instance = instance_builder.build();

    write_mps(
            instance,
            vm["solver"].as<mathoptsolverscmake::SolverName>(),
            vm["output"].as<std::string>());

    return 0;
}
