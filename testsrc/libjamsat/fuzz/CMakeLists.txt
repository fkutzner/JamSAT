add_fuzzing_target(trivial_fuzzer TrivialFuzz.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/trivial)
add_fuzzing_target(cnfproblem_fuzzer CNFProblemFuzz.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/cnfproblem)
