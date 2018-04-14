set(JAMSAT_LIBRARY_UNITTEST_FOLDER_NAME "JamSAT Library/Unit Tests")

# Adds a JamSAT unit test library library.
# Usage: add_jamsat_core_unittest_library(TargetName Source1.cpp Source2.cpp ...)
# Creates an object library target ${TargetName} containing the objects
# of the given source files.
function(add_jamsat_core_unittest_library TARGET)
  add_library(${TARGET} OBJECT ${ARGN})
  set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_LIBRARY_UNITTEST_FOLDER_NAME})

  get_property(jamcore_include_dirs TARGET jamcore PROPERTY INCLUDE_DIRECTORIES)
  get_property(gtest_include_dirs TARGET gtest PROPERTY INCLUDE_DIRECTORIES)
  target_include_directories(${TARGET} PRIVATE ${jamcore_include_dirs})
  target_include_directories(${TARGET} PRIVATE ${gtest_include_dirs})
  target_include_directories(${TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
endfunction()

add_subdirectory(cnfproblem)
add_subdirectory(clausedb)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(utils)
add_subdirectory(proof)


add_executable(libjamsat_unit_testsuite
  $<TARGET_OBJECTS:branching_unit_tests>
  $<TARGET_OBJECTS:cnfproblem_unit_tests>
  $<TARGET_OBJECTS:clausedb_unit_tests>
  $<TARGET_OBJECTS:solver_unit_tests>
  $<TARGET_OBJECTS:utils_unit_tests>
  $<TARGET_OBJECTS:proof_unit_tests>

  # From toolbox/testutils:
  $<TARGET_OBJECTS:jamsatTestUtils>
)

target_link_libraries(libjamsat_unit_testsuite jamcore gtest gtest_main)
set_property(TARGET libjamsat_unit_testsuite PROPERTY FOLDER ${JAMSAT_LIBRARY_UNITTEST_FOLDER_NAME})
add_test(
    NAME libjamsat_unit_testsuite
    COMMAND libjamsat_unit_testsuite
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
