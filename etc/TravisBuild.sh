#!/bin/bash
set -e

# For this script, it's assumed that the current working directory is an empty
# directory where the build files can be placed.

if [ "${TRAVIS_OS_NAME}" == "linux" ]
then
  sudo apt-get install -y libboost-all-dev
fi

if [ "${SONARSOURCE_SCAN}" != "1" ]
then
  cmake -DCMAKE_BUILD_TYPE=Debug ${TRAVIS_BUILD_DIR}
  cmake --build . -- -j2
  ctest -V
else
  cd ${TRAVIS_BUILD_DIR}
  cmake -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON .
  build-wrapper-linux-x86-64 --out-dir bw-output make clean all
  ctest -V
  sonar-scanner
fi
