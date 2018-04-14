file(GLOB cnfproblems *.cnf)
foreach(cnfproblem ${cnfproblems})
    get_filename_component(cnfproblemBasename ${cnfproblem} NAME)
    set(testname "libjamsat_acceptance_tests/fuzz-regression/${cnfproblemBasename}")

    # Just check that the assertions hold:
    add_test(NAME ${testname} COMMAND CDCLTestDriver --no-fail-on-parse-error --check-result ${cnfproblem})
endforeach()
