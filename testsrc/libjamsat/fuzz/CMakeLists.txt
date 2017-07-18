add_fuzzing_target(trivial_fuzzer TrivialFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/trivial)
add_fuzzing_target(cnfproblem_fuzzer CNFProblemFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/cnfproblem)
