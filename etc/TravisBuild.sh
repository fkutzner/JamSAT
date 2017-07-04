#!/bin/bash
set -e

# For this script, it's assumed that the current working directory is an empty
# directory where the build files can be placed.

if [ "${SONARSOURCE_SCAN}" != "1" ]
then
  echo "Building JamSAT in ${JAMSAT_MODE} mode."

  if [ "${JAMSAT_MODE}" = "COVERAGE" ]
  then
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON ${TRAVIS_BUILD_DIR}
  elif [ "${JAMSAT_MODE}" = "SANITIZERS" ]
  then
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_SANITIZERS=ON -DJAMSAT_DISABLE_OPTIMIZATIONS=ON ${TRAVIS_BUILD_DIR}
  else
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ${TRAVIS_BUILD_DIR}
  fi

  echo "Compilation database:"
  cat compile_commands.json

  echo "Building..."
  cmake --build . -- -j2

  echo "Testing..."
  ctest -V
else
  cd ${TRAVIS_BUILD_DIR}
  cmake -DCMAKE_BUILD_TYPE=Debug -DJAMSAT_ENABLE_COVERAGE=ON .
  build-wrapper-linux-x86-64 --out-dir bw-output make clean all
  ctest -V
  sonar-scanner
fi
