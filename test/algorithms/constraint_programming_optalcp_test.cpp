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

// Hand-made instances, for cases which the generated instances don't cover.
INSTANTIATE_TEST_SUITE_P(
        ConstraintProgrammingOptalcpHandmade,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return constraint_programming_optalcp(instance);
                    },
                },
                {
                    {
                        // Permutation flow shop, makespan, all machines
                        // no-idle, with more jobs (8) than machines (3). The
                        // no-idle constraint of a machine must cover all its
                        // operations, not only the first ones.
                        {get_path({"handmade", "permutation_flow_shop", "pfss_makespan_no_idle_n8_m3.json"}), "json", 478},
                    },
                })));
