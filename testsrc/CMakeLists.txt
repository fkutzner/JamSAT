### Find minisat

set(MINISAT_IMPORT_STATIC_LIBRARY ON)
find_package(Minisat 2.1.0)

if (NOT Minisat_FOUND)
  message(FATAL_ERROR "Could not find Minisat. Is an installation of https://github.com/fkutzner/minisat in your CMAKE_PREFIX_PATH?")
endif()

# If this becomes more than one library, create a suitable JAMSAT_TEST_LIB_DEPS variable
if(JAMSAT_LINK_VERBOSE)
  message_list(STATUS ${Minisat_LIBRARY} "JamSAT tests will additionally be linked to the following external libraries:")
endif()

### Set up fuzzing

set(JAMSAT_FUZZ_ADAPTER_LIBRARY jamsatFuzzGenericFuzzAdapter)
if(JAMSAT_ENABLE_AFL_FUZZER)
  # uses the default generic fuzz adapter.

  if(NOT ((${CMAKE_C_COMPILER} MATCHES ".*afl.*")  AND(${CMAKE_CXX_COMPILER} MATCHES ".*afl.*")))
    message(
      WARNING
      "Apparently, you are not compiling with afl-clang rsp. afl-gcc."
      " For improved fuzzing speed, use e.g.:\ncmake -DCMAKE_C_COMPILER=afl-clang"
      " -DCMAKE_CXX_COMPILER=afl-clang++"
    )
  endif()

  find_program(AFL_FUZZER "afl-fuzz")
  if (NOT AFL_FUZZER)
    message(FATAL_ERROR "Could not find afl-fuzz, please check if its directory is in your PATH.")
  endif()
endif()

#
# Creates a fuzzing-runner target for the binary target TARGET_NAME, instructing
# the fuzzer to use the fuzzing seed files given in SEED_DIRECTORY.
#
function(add_fuzzing_execution_target TARGET_NAME SEED_DIRECTORY)
  if(JAMSAT_ENABLE_AFL_FUZZER)
    add_custom_target(${TARGET_NAME}-run)
    add_custom_command(TARGET ${TARGET_NAME}-run PRE_BUILD
                       COMMAND ${AFL_FUZZER}
                                -i ${SEED_DIRECTORY}
                                -o $<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}-fuzzer-findings
                                $<TARGET_FILE:${TARGET_NAME}>)

    add_custom_target(${TARGET_NAME}-continue)
    add_custom_command(TARGET ${TARGET_NAME}-continue PRE_BUILD
                       COMMAND ${AFL_FUZZER}
                                -i -
                                -o $<TARGET_FILE_DIR:${TARGET_NAME}>/${TARGET_NAME}-fuzzer-findings
                                $<TARGET_FILE:${TARGET_NAME}>)

    add_dependencies(${TARGET_NAME}-run ${TARGET_NAME})
    add_dependencies(${TARGET_NAME}-continue ${TARGET_NAME})
  endif()
endfunction()

#
# Given a file TARGET_FILE containing a fuzzer entry point (i.e. when linked
# with jamcore and a fuzzing adapter, results in a complete fuzzing target
# program) and a directory SEED_DIRECTORY containing fuzzing seeds, this
# function defines an executable target TARGET_NAME and a fuzzer-running target
# TARGET_NAME-run. By convention, the TARGET_NAME must end with _fuzzer.
#
function(add_fuzzing_target TARGET_NAME TARGET_FILE SEED_DIRECTORY)
  if(NOT (${TARGET_NAME} MATCHES ".*_fuzzer$"))
    message(FATAL_ERROR "Fuzzing target name ${TARGET_NAME} does not end with _fuzzer")
  endif()

  add_executable(${TARGET_NAME} $<TARGET_OBJECTS:${JAMSAT_FUZZ_ADAPTER_LIBRARY}> ${TARGET_FILE})
  jamsat_configure_target(${TARGET_NAME})
  target_link_libraries(${TARGET_NAME} jamcore)
  target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
  add_fuzzing_execution_target(${TARGET_NAME} ${SEED_DIRECTORY})
endfunction()

add_subdirectory(toolbox)
add_subdirectory(libjamsat)

if(NOT JAMSAT_DISABLE_FRONTEND)
  add_subdirectory(jamsat)
endif()
