add_library(utils_unit_tests OBJECT
  BoundedMapTests.cpp
  BoundedStackTests.cpp
  FaultInjectorTests.cpp
  TruthTests.cpp
  ControlFlowTests.cpp
  StampMapTests.cpp
  SimpleMovingAverageTests.cpp
  LubySequenceTests.cpp
)

target_include_directories(utils_unit_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                   ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
target_include_directories(utils_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                    ${PROJECT_SOURCE_DIR}/src)
