SET(JAMSAT_LIBRARY_CNFGENERATORS_FOLDER_NAME "JamSAT Test Infrastructure")

add_library(cnfgenerators OBJECT
  GateStructure.h
  GateStructure.cpp
)

target_include_directories(cnfgenerators PRIVATE ${PROJECT_SOURCE_DIR}/testsrc
                                                 ${PROJECT_SOURCE_DIR}/src)
set_property(TARGET cnfgenerators PROPERTY FOLDER ${JAMSAT_LIBRARY_CNFGENERATORS_FOLDER_NAME})
