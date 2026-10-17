#include "tests.hpp"
#include "shopschedulingsolver/algorithms/tree_search_pfss.hpp"

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
        TreeSearchPfss,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return tree_search_pfss(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "tree_search_pfss_test.txt"})),
                })));
