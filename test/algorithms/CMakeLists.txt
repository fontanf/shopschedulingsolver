add_library(ShopSchedulingSolver_tests)
target_sources(ShopSchedulingSolver_tests PRIVATE
    tests.cpp)
target_link_libraries(ShopSchedulingSolver_tests PUBLIC
    ShopSchedulingSolver_shopschedulingsolver
    Boost::filesystem
    GTest::gtest_main)
add_library(ShopSchedulingSolver::tests ALIAS ShopSchedulingSolver_tests)

add_executable(ShopSchedulingSolver_milp_positional_test)
target_sources(ShopSchedulingSolver_milp_positional_test PRIVATE
    milp_positional_test.cpp)
target_link_libraries(ShopSchedulingSolver_milp_positional_test
    ShopSchedulingSolver_tests
    ShopSchedulingSolver_milp_positional
    GTest::gtest_main)
gtest_discover_tests(ShopSchedulingSolver_milp_positional_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

add_executable(ShopSchedulingSolver_milp_disjunctive_test)
target_sources(ShopSchedulingSolver_milp_disjunctive_test PRIVATE
    milp_disjunctive_test.cpp)
target_link_libraries(ShopSchedulingSolver_milp_disjunctive_test
    ShopSchedulingSolver_tests
    ShopSchedulingSolver_milp_disjunctive
    GTest::gtest_main)
gtest_discover_tests(ShopSchedulingSolver_milp_disjunctive_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

if(SHOPSCHEDULINGSOLVER_USE_OPTALCP)
    add_executable(ShopSchedulingSolver_constraint_programming_optalcp_test)
    target_sources(ShopSchedulingSolver_constraint_programming_optalcp_test PRIVATE
        constraint_programming_optalcp_test.cpp)
    target_link_libraries(ShopSchedulingSolver_constraint_programming_optalcp_test
        ShopSchedulingSolver_tests
        ShopSchedulingSolver_constraint_programming_optalcp
        GTest::gtest_main)
    gtest_discover_tests(ShopSchedulingSolver_constraint_programming_optalcp_test
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        PROPERTIES TIMEOUT 3600)
endif()

add_executable(ShopSchedulingSolver_tree_search_pfss_makespan_test)
target_sources(ShopSchedulingSolver_tree_search_pfss_makespan_test PRIVATE
    tree_search_pfss_makespan_test.cpp)
target_link_libraries(ShopSchedulingSolver_tree_search_pfss_makespan_test
    ShopSchedulingSolver_tests
    ShopSchedulingSolver_tree_search_pfss_makespan
    GTest::gtest_main)
gtest_discover_tests(ShopSchedulingSolver_tree_search_pfss_makespan_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)

add_executable(ShopSchedulingSolver_tree_search_pfss_tft_test)
target_sources(ShopSchedulingSolver_tree_search_pfss_tft_test PRIVATE
    tree_search_pfss_tft_test.cpp)
target_link_libraries(ShopSchedulingSolver_tree_search_pfss_tft_test
    ShopSchedulingSolver_tests
    ShopSchedulingSolver_tree_search_pfss_tft
    GTest::gtest_main)
gtest_discover_tests(ShopSchedulingSolver_tree_search_pfss_tft_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES TIMEOUT 3600)
