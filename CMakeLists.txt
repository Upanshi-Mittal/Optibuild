cmake_minimum_required(VERSION 3.10)
project(CI_Optimizer)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

# All source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# MAIN APP (includes everything)
add_executable(main ${SOURCES})

# TEST RUNNER (exclude main.cpp)
file(GLOB_RECURSE TEST_SOURCES "src/*.cpp")
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

add_executable(test_runner tests/test_main.cpp ${TEST_SOURCES})

enable_testing()
add_test(NAME MyTest COMMAND test_runner)

add_executable(graph_builder analyzer/graph_builder.cpp)
add_executable(impact_analyzer analyzer/impact_analyzer.cpp)
add_executable(build_optimizer analyzer/build_optimizer.cpp)