#include "tests.hpp"
#include "shopschedulingsolver/algorithms/constraint_programming_optalcp.hpp"
#include "shopschedulingsolver/instance_builder.hpp"
#include "shopschedulingsolver/solution_builder.hpp"


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
// Each instance is stored with a reference optimal solution.

struct ConstraintProgrammingOptalcpHandmadeTestParams
{
    std::string instance_path;
    std::string certificate_path;
};

inline std::ostream& operator<<(
        std::ostream& os,
        const ConstraintProgrammingOptalcpHandmadeTestParams& test_params)
{
    os << test_params.instance_path;
    return os;
}

class ConstraintProgrammingOptalcpHandmadeTest: public testing::TestWithParam<ConstraintProgrammingOptalcpHandmadeTestParams> { };

TEST_P(ConstraintProgrammingOptalcpHandmadeTest, ConstraintProgrammingOptalcpHandmade)
{
    ConstraintProgrammingOptalcpHandmadeTestParams test_params = GetParam();
    InstanceBuilder instance_builder;
    instance_builder.read(test_params.instance_path, "json");
    const Instance instance = instance_builder.build();

    Output output = constraint_programming_optalcp(instance);

    SolutionBuilder solution_builder;
    solution_builder.set_instance(instance);
    solution_builder.read(test_params.certificate_path);
    Solution solution = solution_builder.build();
    std::cout << std::endl
        << "Reference solution" << std::endl
        << "------------------" << std::endl;
    solution.format(std::cout);

    ASSERT_TRUE(solution.feasible());
    EXPECT_TRUE(output.solution.feasible());
    EXPECT_EQ(output.solution.objective_value(), solution.objective_value());
}

INSTANTIATE_TEST_SUITE_P(
        ConstraintProgrammingOptalcp,
        ConstraintProgrammingOptalcpHandmadeTest,
        testing::ValuesIn(std::vector<ConstraintProgrammingOptalcpHandmadeTestParams>{
            {
                // Permutation flow shop, makespan, all machines no-idle,
                // with more jobs (8) than machines (3). The no-idle
                // constraint of a machine must cover all its operations, not
                // only the first ones.
                get_path({"data", "handmade", "permutation_flow_shop", "pfss_makespan_no_idle_n8_m3.json"}),
                get_path({"data", "handmade", "permutation_flow_shop", "pfss_makespan_no_idle_n8_m3.json_solution.json"}),
            }}));
