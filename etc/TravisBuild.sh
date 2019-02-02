#!/bin/bash
set -e

# For this script, it's assumed that the current working directory is an empty
# directory where the build files can be placed.

build_and_test() {
  echo "Compilation database:"
  cat compile_commands.json

  echo "Building..."
  cmake --build . -- -j2

  echo -n "Testing "
  if [ "$1" == "--only-unit-and-integration-tests" ]
  then
    echo "(only unit and integration tests)..."
    ctest -V -R "unit\$"
    ctest -V -R "integration\$"
  else
    echo "..."
    ctest -V
  fi
}

process_coverage_results() {
  lcov --directory . --capture --output-file coverage.info
  lcov --remove coverage.info 'deps/*' 'testsrc/*' '/usr/*' --output-file coverage.info
  lcov --list coverage.info
}

echo "Tool versions:"
cmake --version
clang --version
gcc --version
echo "(End tool versions)"

if [ "${SONARSOURCE_SCAN}" != "1" ]
then
  echo "Building JamSAT in ${JAMSAT_MODE} mode."

  if [ "${JAMSAT_MODE}" = "COVERAGE" ]
  then
    cmake -DJAMSAT_ENABLE_TESTING=ON -DJAMSAT_LINK_VERBOSE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON ${TRAVIS_BUILD_DIR}
    build_and_test --only-unit-and-integration-tests
    process_coverage_results
    pushd $TRAVIS_BUILD_DIR
    coveralls-lcov ../build/coverage.info
    popd

  elif [ "${JAMSAT_MODE}" = "SANITIZERS" ]
  then
    echo "Running address and leak sanitizer..."
    cmake -DJAMSAT_ENABLE_TESTING=ON -DJAMSAT_LINK_VERBOSE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_SANITIZERS=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON -DJAMSAT_USE_SEPARATE_CLANG_SANITIZERS=ON ${TRAVIS_BUILD_DIR}
    build_and_test
  else
    cmake -DJAMSAT_ENABLE_TESTING=ON -DJAMSAT_LINK_VERBOSE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ${TRAVIS_BUILD_DIR}
    build_and_test
  fi
else
  cd ${TRAVIS_BUILD_DIR}
  cmake -DJAMSAT_ENABLE_TESTING=ON -DJAMSAT_LINK_VERBOSE=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON .
  build-wrapper-linux-x86-64 --out-dir bw-output make clean all
  ctest -V
  find lib -name "*.h" -or -name "*.cpp" | xargs -I% gcov --branch-probabilities --branch-counts % -o .
  sonar-scanner
fi
