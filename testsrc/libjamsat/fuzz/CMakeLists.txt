add_fuzzing_target(trivial_fuzzer TrivialFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/trivial)
add_fuzzing_target(cnfproblem_fuzzer CNFProblemFuzzingTarget.cpp ${CMAKE_CURRENT_SOURCE_DIR}/seeds/cnfproblem)

SET(JAMSAT_LIBRARY_FUZZ_FOLDER_NAME "JamSAT Library/Fuzzing Targets")
set_property(TARGET trivial_fuzzer PROPERTY FOLDER ${JAMSAT_LIBRARY_FUZZ_FOLDER_NAME})
set_property(TARGET cnfproblem_fuzzer PROPERTY FOLDER ${JAMSAT_LIBRARY_FUZZ_FOLDER_NAME})
