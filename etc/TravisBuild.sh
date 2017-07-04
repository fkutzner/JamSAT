#!/bin/bash
set -e

# For this script, it's assumed that the current working directory is an empty
# directory where the build files can be placed.

if [ "${SONARSOURCE_SCAN}" != "1" ]
then
  if [ "${JAMSAT_MODE}" = "COVERAGE" ]
  then
    cmake -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON ${TRAVIS_BUILD_DIR}
  else
    cmake -DCMAKE_BUILD_TYPE=Debug ${TRAVIS_BUILD_DIR}
  fi

  cmake --build . -- -j2
  ctest -V
else
  cd ${TRAVIS_BUILD_DIR}
  cmake -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON .
  build-wrapper-linux-x86-64 --out-dir bw-output make clean all
  ctest -V
  sonar-scanner
fi
