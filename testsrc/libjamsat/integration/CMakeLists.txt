set(JAMSAT_LIBRARY_INTEGRATIONTEST_FOLDER_NAME "JamSAT Library/Integration Tests")

# Adds a JamSAT integration test library.
# Usage: add_jamsat_core_integrationtest_library(TargetName Source1.cpp Source2.cpp ...)
# Creates an object library target ${TargetName} containing the objects
# of the given source files.
function(add_jamsat_core_integrationtest_library TARGET)
  add_library(${TARGET} OBJECT ${ARGN})
  set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_LIBRARY_INTEGRATIONTEST_FOLDER_NAME})
  target_include_directories(${TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
  target_include_directories(${TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                               ${PROJECT_SOURCE_DIR}/src)

  # TODO: automatically add the objects to libjamsat_integration_tests
endfunction()

add_subdirectory(solver)

add_executable(libjamsat_integration_testsuite
  $<TARGET_OBJECTS:solver_integration_tests>

  # From toolbox/cnfgenerators:
  $<TARGET_OBJECTS:cnfgenerators>
)

target_link_libraries(libjamsat_integration_testsuite jamcore gtest gtest_main)
set_property(TARGET libjamsat_integration_testsuite
             PROPERTY FOLDER ${JAMSAT_LIBRARY_INTEGRATIONTEST_FOLDER_NAME})
add_test(
    NAME libjamsat_integration_testsuite
    COMMAND libjamsat_integration_testsuite
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
