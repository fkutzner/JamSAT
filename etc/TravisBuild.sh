#!/bin/bash

# For this script, it's assumed that the current working directory is an empty
# directory where the build files can be placed.

cmake -DCMAKE_BUILD_TYPE=Debug ${TRAVIS_BUILD_DIR} || exit 1

if [ "SONARSOURCE_SCAN" != "1" ]
then
  cmake --build . -- -j2 || exit 1
  ctest -V || exit 1
else
  build-wrapper-linux-x86-64 --out-dir bw-output make clean all || exit 1
  ctest -V || exit 1
  sonar-scanner || exit 1
fi
