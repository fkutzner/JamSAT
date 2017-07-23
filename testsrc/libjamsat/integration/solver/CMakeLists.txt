add_library(solver_integration_tests OBJECT
  SimpleDPLL.cpp
)

target_include_directories(solver_integration_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                    ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
target_include_directories(solver_integration_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                    ${PROJECT_SOURCE_DIR}/src)
