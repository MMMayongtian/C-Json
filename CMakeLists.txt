cmake_minimum_required (VERSION 3.10)
project (json_test C)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

add_library(json json.c)
add_executable(json_test test.c)
target_link_libraries(json_test json)
