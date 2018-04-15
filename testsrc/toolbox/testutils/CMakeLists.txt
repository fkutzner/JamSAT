SET(JAMSAT_LIBRARY_CNFGENERATORS_FOLDER_NAME "JamSAT Test Infrastructure")

add_library(jamsatTestUtils STATIC
  RangeUtils.h
  Minisat.cpp
  Minisat.h
)
jamsat_configure_target(jamsatTestUtils)

target_include_directories(jamsatTestUtils PUBLIC ${PROJECT_SOURCE_DIR}/testsrc)

target_include_directories(jamsatTestUtils PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                                  ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_link_libraries(jamsatTestUtils jamcore ${Minisat_LIBRARY})
target_include_directories(jamsatTestUtils SYSTEM PRIVATE ${Minisat_INCLUDE_DIRS})

set_property(TARGET jamsatTestUtils PROPERTY FOLDER ${JAMSAT_LIBRARY_CNFGENERATORS_FOLDER_NAME})
