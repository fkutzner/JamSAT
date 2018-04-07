add_executable(SmallRandomSATTestDriver
  SmallRandomSATTestDriver.cpp
)

target_link_libraries(SmallRandomSATTestDriver jamcore)
target_include_directories(SmallRandomSATTestDriver PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                            ${PROJECT_SOURCE_DIR}/src)

file(GLOB cnfproblems *.cnf)
foreach(cnfproblem ${cnfproblems})
    get_filename_component(cnfproblemBasename ${cnfproblem} NAME)
    string(REGEX MATCH "^unsat-" match_unsat ${cnfproblemBasename})

    set(expected_result_regex "^SATISFIABLE")
    if (match_unsat)
        set(expected_result_regex "^UNSATISTIABLE")
    endif()

    set(testname "libjamsat_acceptance_tests/small-randomsat/${cnfproblemBasename}")
    add_test(NAME ${testname}
             COMMAND SmallRandomSATTestDriver ${cnfproblem})
    set_tests_properties(${testname} PROPERTIES PASS_REGULAR_EXPRESSION ${expected_result_regex})
endforeach()
