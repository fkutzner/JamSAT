SET(JAMSAT_LIBRARY_CNFGENERATORS_FOLDER_NAME "JamSAT Test Infrastructure")

add_library(jamsatTestUtils OBJECT
  RangeUtils.h
  Dummy.cpp
)

target_include_directories(jamsatTestUtils PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                   ${PROJECT_SOURCE_DIR}/src)
target_include_directories(jamsatTestUtils PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                  ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)
set_property(TARGET jamsatTestUtils PROPERTY FOLDER ${JAMSAT_LIBRARY_CNFGENERATORS_FOLDER_NAME})
