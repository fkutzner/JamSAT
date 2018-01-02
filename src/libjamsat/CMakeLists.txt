include_directories(${PROJECT_SOURCE_DIR}/src)

set(JAMSAT_LIBRARY_FOLDER_NAME "JamSAT Library")

# Adds a JamSAT core library.
# Usage: add_jamsat_core_library(TargetName Source1.cpp Source2.cpp ...)
# Creates an object library target ${TargetName} containing the objects
# of the given source files.
function(add_jamsat_core_library TARGET)
  add_library(${TARGET} OBJECT ${ARGN})
  set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_LIBRARY_FOLDER_NAME})
  # TODO: automatically add the objects to jamcore
endfunction()

add_subdirectory(cnfproblem)
add_subdirectory(utils)
add_subdirectory(clausedb)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(proof)

add_library(
  jamcore STATIC
  $<TARGET_OBJECTS:jamsatCnfProblem>
  $<TARGET_OBJECTS:jamsatUtils>
  $<TARGET_OBJECTS:jamsatClauseDB>
  $<TARGET_OBJECTS:jamsatSolver>
  $<TARGET_OBJECTS:jamsatBranching>
  $<TARGET_OBJECTS:jamsatProof>
)
set_property(TARGET jamcore PROPERTY FOLDER ${JAMSAT_LIBRARY_FOLDER_NAME})

target_link_libraries(jamcore ${JAMSAT_LIB_DEPS})
