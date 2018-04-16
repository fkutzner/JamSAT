# Example CMake project showing how to use JamSAT in client projects
#
# Build this with a JamSAT installation in your CMAKE_PREFIX_PATH.

project(JamSAT-IpasirClientExample)
cmake_minimum_required(VERSION 3.6)

find_package(JamSAT REQUIRED)
add_executable(ipasirclient IpasirClient.cpp)
target_link_libraries(ipasirclient jamcore-dso)

# The include directories of ipasirclient are automatically set up so that
# jamsat/JamSatIpasir.h can be included

install(
  TARGETS ipasirclient
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
