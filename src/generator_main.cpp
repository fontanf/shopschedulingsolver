#include "shopschedulingsolver/generator.hpp"

#include <boost/program_options.hpp>

using namespace shopschedulingsolver;

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;

    GenerateInput input;
    Seed seed = 0;
    std::string output_path = "";

    // Parse program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("objective,", po::value<Objective>(&input.objective), "set objective")
        ("operations-arbitrary-order,", po::value<bool>(&input.operations_arbitrary_order), "set operations arbitrary order")
        ("no-wait,", po::value<bool>(&input.no_wait), "set no-wait property")
        ("no-idle,", po::value<bool>(&input.no_idle), "set no-idle property")
        ("blocking,", po::value<bool>(&input.blocking), "set blocking property")
        ("flow-shop,", po::value<bool>(&input.flow_shop), "set permutation property")
        ("permutation,", po::value<bool>(&input.permutation), "set permutation property")
        ("number-of-machine-groups,", po::value<MachineId>(&input.number_of_machine_groups), "set number of machine groups")
        ("number-of-machines-per-groups,", po::value<MachineId>(&input.number_of_machines_per_group), "set number of machines per group")
        ("number-of-jobs,", po::value<JobId>(&input.number_of_jobs), "set number of jobs")
        ("number-of-operations-per-jobs,", po::value<MachineId>(&input.number_of_operations_per_job), "set number of operations per jobs")
        ("processing-times-range,", po::value<Time>(&input.processing_times_range), "set processing times range")
        ("weights-range,", po::value<Time>(&input.weights_range), "set weights range")
        ("due-date-tightness-factor,", po::value<double>(&input.due_date_tightness_factor)->required(), "set due date factor")
        ("seed,", po::value<Seed>(&seed), "set seed")
        ("output,", po::value<std::string>(&output_path)->required(), "set output path")
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

    std::mt19937_64 generator(seed);
    Instance instance = generate(
            input,
            generator);
    instance.write(output_path);

    return 0;
}
