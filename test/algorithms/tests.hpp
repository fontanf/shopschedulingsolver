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

/**
 * Hand-made test instances, for cases which the generated instances don't
 * cover.
 *
 * They are stored in data/handmade/, each with a reference optimal solution.
 */
struct HandmadeTestParams
{
    Algorithm algorithm;

    /** Path of the instance, in the JSON format. */
    std::string instance_path;

    /** Path of an optimal solution of the instance. */
    std::string certificate_path;
};

std::ostream& operator<<(
        std::ostream& os,
        const HandmadeTestParams& test_params);

/**
 * Solve the instance with the algorithm, and check that the solution found
 * is feasible and has the same objective value as the reference solution.
 */
void test_handmade(const HandmadeTestParams& test_params);

class HandmadeAlgorithmTest: public testing::TestWithParam<HandmadeTestParams> { };

}
