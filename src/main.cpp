#include "shopschedulingsolver/optimize.hpp"
#include "shopschedulingsolver/instance_builder.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>

using namespace shopschedulingsolver;
namespace po = boost::program_options;

void read_args(
        shopschedulingsolver::Parameters& parameters,
        const po::variables_map& vm)
{
    parameters.timer.set_sigint_handler();
    parameters.messages_to_stdout = true;
    if (vm.count("time-limit"))
        parameters.timer.set_time_limit(vm["time-limit"].as<double>());
    if (vm.count("verbosity-level"))
        parameters.verbosity_level = vm["verbosity-level"].as<int>();
    if (vm.count("log2stderr"))
        parameters.log_to_stderr = vm["log-to-stderr"].as<bool>();
    if (vm.count("log"))
        parameters.log_path = vm["log"].as<std::string>();
    parameters.log_to_stderr = vm.count("log-to-stderr");
    bool only_write_at_the_end = vm.count("only-write-at-the-end");
    if (!only_write_at_the_end) {

        std::string certificate_path = "";
        if (vm.count("certificate"))
            certificate_path = vm["certificate"].as<std::string>();

        std::string json_output_path = "";
        if (vm.count("output"))
            json_output_path = vm["output"].as<std::string>();
        std::string certificate_format;
        if (vm.count("certificate-format"))
            certificate_format = vm["certificate-format"].as<std::string>();

        parameters.new_solution_callback = [
            json_output_path,
            certificate_path,
            certificate_format](
                    const shopschedulingsolver::Output& output)
        {
            if (!json_output_path.empty())
                output.write_json_output(json_output_path);
            if (!certificate_path.empty()) {
                output.solution.write(
                        certificate_path,
                        certificate_format);
            }
        };
    }
}

int main(int argc, char *argv[])
{

    // Parse program options
    po::options_description desc("Allowed options");
    desc.add_options()
        (",h", "Produce help message")

        ("input,i", po::value<std::string>()->required(), "Input path")
        ("format,f", po::value<std::string>(), "Input format")

        ("output,o", po::value<std::string>(), "Output path")
        ("certificate,c", po::value<std::string>(), "Certificate path")
        ("log,l", po::value<std::string>(), "Log path")
        ("time-limit,t", po::value<double>(), "Time limit in seconds")
        ("seed,s", po::value<Seed>(), "Seed (not used)")
        ("only-write-at-the-end,e", "Only write output and certificate files at the end")
        ("verbosity-level,v", po::value<int>(), "Verbosity level")
        ("log-to-stderr,w", "Write log in stderr")
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
    std::string input_format = "";
    if (vm.count("format"))
        input_format = vm["format"].as<std::string>(),
    instance_builder.read(
            vm["input"].as<std::string>(),
            input_format);
    Instance instance = instance_builder.build();

    // Read optimize parameters.
    OptimizeParameters parameters;
    read_args(parameters, vm);

    // Solve.
    const Output output = optimize(instance, parameters);

    // Write output.
    if (vm.count("certificate")) {
        std::string certificate_format;
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
