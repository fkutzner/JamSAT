# Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name(s) of the above copyright holders
# shall not be used in advertising or otherwise to promote the sale, use or
# other dealings in this Software without prior written authorization.

### Set up fuzzing

set(JAMSAT_FUZZ_ADAPTER_LIBRARY jstest.toolbox.fuzz.genericfuzzadapter)
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
# with libjamsat and a fuzzing adapter, results in a complete fuzzing target
# program) and a directory SEED_DIRECTORY containing fuzzing seeds, this
# function defines an executable target TARGET_NAME and a fuzzer-running target
# TARGET_NAME-run. By convention, the TARGET_NAME must end with _fuzzer.
#
function(add_fuzzing_target TARGET_NAME TARGET_FILE SEED_DIRECTORY)
  if(NOT (${TARGET_NAME} MATCHES ".*_fuzzer$"))
    message(FATAL_ERROR "Fuzzing target name ${TARGET_NAME} does not end with _fuzzer")
  endif()

  nm_add_tool(${TARGET_NAME} $<TARGET_OBJECTS:${JAMSAT_FUZZ_ADAPTER_LIBRARY}> ${TARGET_FILE})
  target_link_libraries(${TARGET_NAME} PRIVATE libjamsat-testing)
  target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
  add_fuzzing_execution_target(${TARGET_NAME} ${SEED_DIRECTORY})
endfunction()

add_subdirectory(toolbox)
add_subdirectory(libjamsat)
add_subdirectory(libjamfrontend)
