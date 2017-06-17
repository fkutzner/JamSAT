add_executable(solver_unit_tests
  TrailUnitTests.cpp
)

target_link_libraries(solver_unit_tests jamcore gtest gtest_main)
target_include_directories(solver_unit_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                    ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
target_include_directories(solver_unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                     ${PROJECT_SOURCE_DIR}/src)

add_test(NAME solver_unit_tests COMMAND solver_unit_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
