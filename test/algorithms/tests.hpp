#pragma once

#include "shopschedulingsolver/algorithm_formatter.hpp"

#include <gtest/gtest.h>

namespace shopschedulingsolver
{

std::string get_path(const std::vector<std::string>& path);

struct TestInstancePath
{
    std::string instance_path;
    std::string instance_format;

    /** Optimal objective value of the instance. */
    double objective_value = 0;
};

std::vector<TestInstancePath> get_test_instance_paths(
        const std::string& file_path);

using Algorithm = std::function<const Output(const Instance&)>;

struct TestParams
{
    Algorithm algorithm;
    TestInstancePath files;
};

std::vector<TestParams> get_test_params(
        const std::vector<Algorithm>& algorithms,
        const std::vector<std::vector<TestInstancePath>>& instance_paths);

const Instance get_instance(
        const TestInstancePath& files);

class ExactAlgorithmTest: public testing::TestWithParam<TestParams> { };
class ExactNoSolutionAlgorithmTest: public testing::TestWithParam<TestParams> { };

}
