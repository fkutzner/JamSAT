add_library(branching_unit_tests OBJECT
  BranchingHeuristicsBaseUnitTests.cpp
  VSIDSBranchingHeuristicUnitTests.cpp
)

target_include_directories(branching_unit_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                       ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
target_include_directories(branching_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                        ${PROJECT_SOURCE_DIR}/src)
