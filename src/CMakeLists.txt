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
add_library(ShopSchedulingSolver::box ALIAS ShopSchedulingSolver_shopschedulingsolver)

if(SHOPSCHEDULINGSOLVER_BUILD_MAIN)
  add_executable(ShopSchedulingSolver_main)
  target_sources(ShopSchedulingSolver_main PRIVATE
      main.cpp)
  target_link_libraries(ShopSchedulingSolver_main PUBLIC
      ShopSchedulingSolver_shopschedulingsolver
      Boost::program_options
      Boost::filesystem)
  set_target_properties(ShopSchedulingSolver_main PROPERTIES OUTPUT_NAME "shopschedulingsolver")
  install(TARGETS ShopSchedulingSolver_main)
endif()
