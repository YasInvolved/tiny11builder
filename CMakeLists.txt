cmake_minimum_required(VERSION 3.28)

project(tiny11maker_lib)

set(TINY11LIB_TEST YES) # compile executable for testing

# cpp standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(tiny11maker_lib UNICODE WIN32_LEAN_AND_MEAN TINY11MAKER_LIB_EXPORTS)
add_subdirectory(src)

target_include_directories(tiny11maker_lib PRIVATE include)
target_link_libraries(tiny11maker_lib PRIVATE virtdisk.lib)

if(TINY11LIB_TEST)
target_include_directories(tiny11maker_test PRIVATE include)
endif()
