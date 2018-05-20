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

set(JAMSAT_UNITTEST_FOLDER_NAME "JamSAT Frontend/Unit Tests")

# Adds a JamSAT unit test library.
# Usage: add_jamsat_frontend_unittest_library(TargetName Source1.cpp Source2.cpp ...)
# Creates an object library target ${TargetName} containing the objects
# of the given source files.
function(add_jamsat_frontend_unittest_library TARGET)
  add_library(${TARGET} OBJECT ${ARGN})
  jamsat_configure_target(${TARGET})
  set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_UNITTEST_FOLDER_NAME})

  get_property(jamcore_include_dirs TARGET jamcore PROPERTY INCLUDE_DIRECTORIES)
  get_property(gtest_include_dirs TARGET gtest PROPERTY INCLUDE_DIRECTORIES)
  target_include_directories(${TARGET} PRIVATE ${jamcore_include_dirs})
  target_include_directories(${TARGET} PRIVATE ${gtest_include_dirs})
  target_include_directories(${TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
endfunction()

add_jamsat_frontend_unittest_library(problem_parsing_tests ProblemParsingTests.cpp)

add_executable(jamsat_frontend_unit_testsuite
  $<TARGET_OBJECTS:problem_parsing_tests>

  # From jamsat/ipasirmock:
  $<TARGET_OBJECTS:ipasirmock>
)
jamsat_configure_target(jamsat_frontend_unit_testsuite)

target_link_libraries(jamsat_frontend_unit_testsuite jamsatFrontend gtest gtest_main)
set_property(TARGET jamsat_frontend_unit_testsuite
             PROPERTY FOLDER ${JAMSAT_UNITTEST_FOLDER_NAME})
add_test(
    NAME jamsat_frontend_unit_testsuite
    COMMAND jamsat_frontend_unit_testsuite
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ProblemParsingTestInputs
)
