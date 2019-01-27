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

add_fuzzing_target(trivial_fuzzer TrivialFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/trivial)
add_fuzzing_target(cnfproblem_fuzzer CNFProblemFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/cnfproblem)
add_fuzzing_target(barecdclsolver_fuzzer BareCDCLSolverFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/barecdclsolver)


target_link_libraries(barecdclsolver_fuzzer jstest.toolbox.testutils)

SET(JAMSAT_LIBRARY_FUZZ_FOLDER_NAME "JamSAT Library/Fuzzing Targets")
set_property(TARGET trivial_fuzzer PROPERTY FOLDER ${JAMSAT_LIBRARY_FUZZ_FOLDER_NAME})
set_property(TARGET cnfproblem_fuzzer PROPERTY FOLDER ${JAMSAT_LIBRARY_FUZZ_FOLDER_NAME})
set_property(TARGET barecdclsolver_fuzzer PROPERTY FOLDER ${JAMSAT_LIBRARY_FUZZ_FOLDER_NAME})
