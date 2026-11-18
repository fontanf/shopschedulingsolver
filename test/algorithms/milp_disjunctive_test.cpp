#include "tests.hpp"
#include "shopschedulingsolver/algorithms/milp_disjunctive.hpp"

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
        MilpDisjunctive,
        ExactAlgorithmTest,
        testing::ValuesIn(get_test_params(
                {
                    [](const Instance& instance)
                    {
                        return milp_disjunctive(instance);
                    },
                },
                {
                    get_test_instance_paths(get_path({"test", "algorithms", "milp_disjunctive_test.txt"})),
                })));

TEST_P(HandmadeAlgorithmTest, HandmadeAlgorithm)
{
    test_handmade(GetParam());
}

INSTANTIATE_TEST_SUITE_P(
        MilpDisjunctive,
        HandmadeAlgorithmTest,
        testing::ValuesIn(std::vector<HandmadeTestParams>{
            {
                // Flexible job shop, makespan, all machines no-idle. In the
                // flexible case, the no-idle constraints are written on the
                // completion times of the alternatives, with big-M terms,
                // which must be inactive for the alternatives which are not
                // selected.
                [](const Instance& instance) { return milp_disjunctive(instance); },
                get_path({"data", "handmade", "flexible_job_shop", "fjss_makespan_no_idle_n4_m2x2.json"}),
                get_path({"data", "handmade", "flexible_job_shop", "fjss_makespan_no_idle_n4_m2x2.json_solution.json"}),
            }}));
