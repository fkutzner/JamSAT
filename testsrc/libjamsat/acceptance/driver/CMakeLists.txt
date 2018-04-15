add_executable(CDCLTestDriver
  CDCLTestDriver.cpp
)
jamsat_configure_target(CDCLTestDriver)

target_link_libraries(CDCLTestDriver jamcore jamsatTestUtils)
