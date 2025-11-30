#include "tests.hpp"

#include "shopschedulingsolver/instance_builder.hpp"
#include "shopschedulingsolver/solution_builder.hpp"

#include <boost/filesystem.hpp>

using namespace shopschedulingsolver;

namespace fs = boost::filesystem;

std::string shopschedulingsolver::get_path(
        const std::vector<std::string>& path)
{
    if (path.empty())
        return "";
    fs::path p(path[0]);
    for (size_t i = 1; i < path.size(); ++i)
        p /= fs::path(path[i]);
    return p.string();
}

std::vector<TestInstancePath> shopschedulingsolver::get_test_instance_paths(
        const std::string& file_path)
{
    std::ifstream file(file_path);
    if (!file.good()) {
        throw std::runtime_error(
                "Unable to open file \"" + file_path + "\".");
    }

    std::vector<TestInstancePath> output;
    std::string tmp;
    while (getline(file, tmp)) {
        TestInstancePath test_instance_path;
        test_instance_path.instance_path = tmp;
        test_instance_path.certificate_path = tmp + "_solution.json";
        output.push_back(test_instance_path);
    }
    return output;
}

std::vector<TestParams> shopschedulingsolver::get_test_params(
        const std::vector<Algorithm>& algorithms,
        const std::vector<std::vector<TestInstancePath>>& instance_paths)
{
    std::vector<TestParams> output;
    for (const Algorithm& algorithm: algorithms) {
        for (const auto& v: instance_paths) {
            for (const TestInstancePath& files: v) {
                TestParams test_params;
                test_params.algorithm = algorithm;
                test_params.files = files;
                output.push_back(test_params);
            }
        }
    }
    return output;
}

const Instance shopschedulingsolver::get_instance(
            const TestInstancePath& files)
{
    std::string instance_path = get_path({
            "data",
            files.instance_path});
    InstanceBuilder instance_builder;
    std::cout << "Instance path:  " << instance_path << std::endl;
    instance_builder.read(instance_path, files.instance_format);
    return instance_builder.build();
}

const Solution shopschedulingsolver::get_solution(
        const Instance& instance,
        const TestInstancePath& files)
{
    std::string certificate_path = get_path({
            "data",
            files.certificate_path});
    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    solution_builder.read(certificate_path, files.certificate_format);
    return solution_builder.build();
}
