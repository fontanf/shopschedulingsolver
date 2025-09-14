add_library(ShopSchedulingSolver_tree_search_pfss_makespan)
target_sources(ShopSchedulingSolver_tree_search_pfss_makespan PRIVATE
    tree_search_pfss_makespan.cpp)
target_include_directories(ShopSchedulingSolver_tree_search_pfss_makespan PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ShopSchedulingSolver_tree_search_pfss_makespan PUBLIC
    ShopSchedulingSolver_shopschedulingsolver
    TreeSearchSolver::treesearchsolver)
add_library(ShopSchedulingSolver::tree_search_pfss_makespan ALIAS ShopSchedulingSolver_tree_search_pfss_makespan)

add_library(ShopSchedulingSolver_tree_search_pfss_tct)
target_sources(ShopSchedulingSolver_tree_search_pfss_tct PRIVATE
    tree_search_pfss_tct.cpp)
target_include_directories(ShopSchedulingSolver_tree_search_pfss_tct PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ShopSchedulingSolver_tree_search_pfss_tct PUBLIC
    ShopSchedulingSolver_shopschedulingsolver
    TreeSearchSolver::treesearchsolver)
add_library(ShopSchedulingSolver::tree_search_pfss_tct ALIAS ShopSchedulingSolver_tree_search_pfss_tct)

add_library(ShopSchedulingSolver_milp_positional)
target_sources(ShopSchedulingSolver_milp_positional PRIVATE
    milp_positional.cpp)
target_include_directories(ShopSchedulingSolver_milp_positional PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ShopSchedulingSolver_milp_positional PUBLIC
    ShopSchedulingSolver_shopschedulingsolver
    MathOptSolversCMake::milp)
add_library(ShopSchedulingSolver::milp_positional ALIAS ShopSchedulingSolver_milp_positional)

add_library(ShopSchedulingSolver_milp_disjunctive)
target_sources(ShopSchedulingSolver_milp_disjunctive PRIVATE
    milp_disjunctive.cpp)
target_include_directories(ShopSchedulingSolver_milp_disjunctive PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ShopSchedulingSolver_milp_disjunctive PUBLIC
    ShopSchedulingSolver_shopschedulingsolver
    MathOptSolversCMake::milp)
add_library(ShopSchedulingSolver::milp_disjunctive ALIAS ShopSchedulingSolver_milp_disjunctive)

if(SHOPSCHEDULINGSOLVER_USE_OPTALCP)
    add_custom_command(
        OUTPUT constraint_programming_optalcp.mjs
        COMMAND npx tsc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS constraint_programming_optalcp.mts tsconfig.json
        COMMENT "Compiling TypeScript solver")

    add_custom_target(ShopSchedulingSolver_constraint_programming_optalcp_ts ALL
        DEPENDS constraint_programming_optalcp.mjs)
    set_target_properties(ShopSchedulingSolver_constraint_programming_optalcp_ts PROPERTIES OUTPUT_NAME "shopschedulingsolver_constraint_programming_optalcp.mjs")
    install(FILES constraint_programming_optalcp.mjs
        DESTINATION bin)

    add_library(ShopSchedulingSolver_constraint_programming_optalcp)
    target_sources(ShopSchedulingSolver_constraint_programming_optalcp PRIVATE
        constraint_programming_optalcp.cpp)
    target_compile_definitions(ShopSchedulingSolver_constraint_programming_optalcp PUBLIC
        OPTALCP_FOUND=1)
    target_include_directories(ShopSchedulingSolver_constraint_programming_optalcp PUBLIC
        ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(ShopSchedulingSolver_constraint_programming_optalcp PUBLIC
        ShopSchedulingSolver_shopschedulingsolver)
    add_dependencies(ShopSchedulingSolver_constraint_programming_optalcp
        ShopSchedulingSolver_constraint_programming_optalcp_ts)
    add_library(ShopSchedulingSolver::constraint_programming_optalcp ALIAS ShopSchedulingSolver_constraint_programming_optalcp)
endif()
