add_library(solver_unit_tests OBJECT
  TrailUnitTests.cpp
  VariableStateUnitTests.cpp
  ClauseUnitTests.cpp
  PropagationUnitTests.cpp
  WatcherUnitTests.cpp
)

target_include_directories(solver_unit_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                    ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
target_include_directories(solver_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                     ${PROJECT_SOURCE_DIR}/src)
