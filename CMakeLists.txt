project(JamSAT)
cmake_minimum_required(VERSION 3.5)

add_definitions(-std=c++11)

add_subdirectory(lib/googletest)
enable_testing()

add_subdirectory(src)

add_subdirectory(testsrc)
