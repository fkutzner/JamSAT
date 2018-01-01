set(JAMSAT_LIBRARY_UNITTEST_FOLDER_NAME "JamSAT Library/Unit Tests")

# Adds a JamSAT unit test library library.
# Usage: add_jamsat_core_unittest_library(TargetName Source1.cpp Source2.cpp ...)
# Creates an object library target ${TargetName} containing the objects
# of the given source files.
function(add_jamsat_core_unittest_library TARGET)
  add_library(${TARGET} OBJECT ${ARGN})
  set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_LIBRARY_UNITTEST_FOLDER_NAME})
  target_include_directories(${TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
  target_include_directories(${TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                               ${PROJECT_SOURCE_DIR}/src)

  # TODO: automatically add the objects to libjamsat_unit_tests
endfunction()

add_subdirectory(cnfproblem)
add_subdirectory(clausedb)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(utils)


add_executable(libjamsat_unit_testsuite
  $<TARGET_OBJECTS:branching_unit_tests>
  $<TARGET_OBJECTS:cnfproblem_unit_tests>
  $<TARGET_OBJECTS:clausedb_unit_tests>
  $<TARGET_OBJECTS:solver_unit_tests>
  $<TARGET_OBJECTS:utils_unit_tests>
)

target_link_libraries(libjamsat_unit_testsuite jamcore gtest gtest_main)
set_property(TARGET libjamsat_unit_testsuite PROPERTY FOLDER ${JAMSAT_LIBRARY_UNITTEST_FOLDER_NAME})
add_test(
    NAME libjamsat_unit_testsuite
    COMMAND libjamsat_unit_testsuite
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
