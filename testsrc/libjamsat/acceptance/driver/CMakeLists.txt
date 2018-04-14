add_executable(CDCLTestDriver
  CDCLTestDriver.cpp
)

target_link_libraries(CDCLTestDriver jamcore ${Minisat_LIBRARY})
target_include_directories(CDCLTestDriver SYSTEM PRIVATE ${Minisat_INCLUDE_DIRS})
