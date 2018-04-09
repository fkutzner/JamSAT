file(GLOB cnfproblems *.cnf)
foreach(cnfproblem ${cnfproblems})
    get_filename_component(cnfproblemBasename ${cnfproblem} NAME)
    string(REGEX MATCH "^unsat-" match_unsat ${cnfproblemBasename})

    set(expected_result_regex ".*Satisfiable:1")
    if (match_unsat)
        set(expected_result_regex ".*Satisfiable:0")
    endif()

    set(testname "libjamsat_acceptance_tests/small-randomsat/${cnfproblemBasename}")
    add_test(NAME ${testname}
             COMMAND CDCLTestDriver --fail-on-parse-error ${cnfproblem})
    set_tests_properties(${testname} PROPERTIES PASS_REGULAR_EXPRESSION ${expected_result_regex})
endforeach()
