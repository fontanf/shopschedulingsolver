cmake_minimum_required(VERSION 3.28)

project(ShopSchedulingSolver LANGUAGES CXX)

# Build options.
option(SHOPSCHEDULINGSOLVER_BUILD_MAIN "Build main" ON)
option(SHOPSCHEDULINGSOLVER_BUILD_TEST "Build the unit tests" ON)

# Avoid FetchContent warning.
cmake_policy(SET CMP0135 NEW)

# Require C++14.
set(CMAKE_CXX_STANDARD 14)

# Enable output of compile commands during generation.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add sub-directories.
add_subdirectory(extern)
add_subdirectory(src)
if(SHOPSCHEDULINGSOLVER_BUILD_TEST)
    add_subdirectory(test)
endif()
