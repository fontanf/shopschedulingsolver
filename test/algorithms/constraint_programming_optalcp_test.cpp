#include "tests.hpp"
#include "shopschedulingsolver/algorithms/constraint_programming_optalcp.hpp"


using namespace shopschedulingsolver;

TEST_P(ExactAlgorithmTest, ExactAlgorithm)
{
    TestParams test_params = GetParam();
    const Instance instance = get_instance(test_params.files);
    auto output = test_params.algorithm(instance);
    std::cout << std::endl;
    std::cout << "Reference objective value: " << test_params.files.objective_value << std::endl;
    EXPECT_EQ(output.solution.objective_value(), test_params.files.objective_value);
}

INSTANTIATE_TEST_SUITE_P(
        ConstraintProgrammingOptalcp,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return constraint_programming_optalcp(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "constraint_programming_optalcp_test.txt"})),
                })));
