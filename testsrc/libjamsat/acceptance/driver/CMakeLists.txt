add_executable(CDCLTestDriver
  CDCLTestDriver.cpp
)

message(STATUS "Minisat: i=${Minisat_INCLUDE_DIRS}, l=${Minisat_LIBRARY}")

target_link_libraries(CDCLTestDriver jamcore ${Minisat_LIBRARY})
target_include_directories(CDCLTestDriver SYSTEM PRIVATE ${Minisat_INCLUDE_DIRS})
