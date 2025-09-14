add_library(ShopSchedulingSolver_shopschedulingsolver)
target_sources(ShopSchedulingSolver_shopschedulingsolver PRIVATE
    instance.cpp
    instance_builder.cpp
    solution.cpp
    solution_builder.cpp
    algorithm_formatter.cpp)
target_include_directories(ShopSchedulingSolver_shopschedulingsolver PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
target_include_directories(ShopSchedulingSolver_shopschedulingsolver PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ShopSchedulingSolver_shopschedulingsolver PUBLIC
    OptimizationTools::utils)
add_library(ShopSchedulingSolver::shopschedulingsolver ALIAS ShopSchedulingSolver_shopschedulingsolver)

add_subdirectory(algorithms)

if(SHOPSCHEDULINGSOLVER_BUILD_MAIN)
    add_executable(ShopSchedulingSolver_main)
    target_sources(ShopSchedulingSolver_main PRIVATE
        main.cpp)
    target_link_libraries(ShopSchedulingSolver_main PUBLIC
        ShopSchedulingSolver_shopschedulingsolver
        ShopSchedulingSolver_tree_search_pfss_makespan
        ShopSchedulingSolver_tree_search_pfss_tct
        ShopSchedulingSolver_milp_positional
        ShopSchedulingSolver_milp_disjunctive
        Boost::program_options
        Boost::filesystem)
    if(SHOPSCHEDULINGSOLVER_USE_OPTALCP)
        target_link_libraries(ShopSchedulingSolver_main PUBLIC
            ShopSchedulingSolver_constraint_programming_optalcp)
    endif()
    set_target_properties(ShopSchedulingSolver_main PROPERTIES OUTPUT_NAME "shopschedulingsolver")
    install(TARGETS ShopSchedulingSolver_main)

    add_executable(ShopSchedulingSolver_mps_writer_main)
    target_sources(ShopSchedulingSolver_mps_writer_main PRIVATE
        mps_writer_main.cpp)
    target_link_libraries(ShopSchedulingSolver_mps_writer_main PUBLIC
        ShopSchedulingSolver_shopschedulingsolver
        ShopSchedulingSolver_milp_disjunctive
        Boost::program_options
        Boost::filesystem)
    set_target_properties(ShopSchedulingSolver_mps_writer_main PROPERTIES OUTPUT_NAME "shopschedulingsolver_mps_writer")
    install(TARGETS ShopSchedulingSolver_mps_writer_main)
endif()
