#!/bin/bash
set -e

# For this script, it's assumed that the current working directory is an empty
# directory where the build files can be placed.

build_and_test() {
  echo "Compilation database:"
  cat compile_commands.json

  echo "Building..."
  cmake --build . -- -j2

  echo "Testing..."
  ctest -V
}

if [ "${SONARSOURCE_SCAN}" != "1" ]
then
  echo "Building JamSAT in ${JAMSAT_MODE} mode."

  if [ "${JAMSAT_MODE}" = "COVERAGE" ]
  then
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON ${TRAVIS_BUILD_DIR}
    build_and_test

  elif [ "${JAMSAT_MODE}" = "SANITIZERS" ]
  then
    echo "Running address and leak sanitizer..."
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_SANITIZERS=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON -DJAMSAT_USE_SEPARATE_CLANG_SANITIZERS=ON ${TRAVIS_BUILD_DIR}
    build_and_test

    mkdir ../build2
    cd ../build2
    echo "Running memory sanitizer..."
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_SANITIZERS=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON -DJAMSAT_ENABLE_MEMORY_SANITIZER=ON ${TRAVIS_BUILD_DIR}
    build_and_test
  else
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ${TRAVIS_BUILD_DIR}
    build_and_test
  fi
else
  cd ${TRAVIS_BUILD_DIR}
  cmake -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON .
  build-wrapper-linux-x86-64 --out-dir bw-output make clean all
  ctest -V
  sonar-scanner
fi
