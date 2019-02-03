#!/bin/sh

lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '*googletest*' --output-file coverage.info
lcov --remove coverage.info '/Applications/*' --output-file coverage.info
lcov --remove coverage.info 'deps/*' 'testsrc/*' '/usr/*' 'gtest-src-wrap/*' --output-file coverage.info
lcov --remove coverage.info '*minisat*' --output-file coverage.info
genhtml coverage.info --output-directory out
