add_subdirectory(cnfproblem)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(utils)


add_executable(libjamsat_unit_tests
  $<TARGET_OBJECTS:branching_unit_tests>
  $<TARGET_OBJECTS:cnfproblem_unit_tests>
  $<TARGET_OBJECTS:solver_unit_tests>
  $<TARGET_OBJECTS:utils_unit_tests>
)

target_link_libraries(libjamsat_unit_tests jamcore gtest gtest_main)
add_test(NAME libjamsat_unit_tests COMMAND libjamsat_unit_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
