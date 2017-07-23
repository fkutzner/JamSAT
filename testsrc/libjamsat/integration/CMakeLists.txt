add_subdirectory(solver)

add_executable(libjamsat_integration_tests
  $<TARGET_OBJECTS:solver_integration_tests>
)

target_link_libraries(libjamsat_integration_tests jamcore gtest gtest_main)
add_test(NAME libjamsat_integration_tests COMMAND libjamsat_integration_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
