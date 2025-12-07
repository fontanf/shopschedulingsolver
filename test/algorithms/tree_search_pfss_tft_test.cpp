#include "tests.hpp"
#include "shopschedulingsolver/algorithms/tree_search_pfss_tft.hpp"

using namespace shopschedulingsolver;

TEST_P(ExactAlgorithmTest, ExactAlgorithm)
{
    TestParams test_params = GetParam();
    const Instance instance = get_instance(test_params.files);
    const Solution solution = get_solution(instance, test_params.files);
    auto output = test_params.algorithm(instance);
    std::cout << std::endl;
    std::cout << "Reference solution" << std::endl;
    std::cout << "------------------" << std::endl;
    solution.format(std::cout, 1);
    EXPECT_EQ(output.solution.objective_value(), solution.objective_value());
}

INSTANTIATE_TEST_SUITE_P(
        TreeSearchPfssTft,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return tree_search_pfss_tft(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "tree_search_pfss_tft_test.txt"})),
                })));
