add_executable(CDCLTestDriver
  CDCLTestDriver.cpp
)

target_link_libraries(CDCLTestDriver jamcore)
target_include_directories(CDCLTestDriver PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                  ${PROJECT_SOURCE_DIR}/src)
