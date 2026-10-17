#include "tests.hpp"

#include "shopschedulingsolver/instance_builder.hpp"

#include <boost/filesystem.hpp>

#include <fstream>
#include <sstream>

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

    // Each line contains the path of an instance and its optimal objective
    // value.
    std::vector<TestInstancePath> output;
    std::string line;
    while (getline(file, line)) {
        if (line.empty())
            continue;
        std::istringstream line_stream(line);
        TestInstancePath test_instance_path;
        if (!(line_stream >> test_instance_path.instance_path >> test_instance_path.objective_value)) {
            throw std::runtime_error(
                    "Invalid line in file \"" + file_path + "\": "
                    "\"" + line + "\"; "
                    "expected \"<instance path> <objective value>\".");
        }
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
