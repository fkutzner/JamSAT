set(JAMSAT_LIBRARY_FOLDER_NAME "JamSAT Library")

# Sets include directories, folders, ... for the given jamcore sub-library.
macro(jamsat_configure_jamcore_library TARGET IS_SHARED)
    target_include_directories(${TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/src)
    set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_LIBRARY_FOLDER_NAME})

    if(${IS_SHARED})
        jamsat_configure_solib_target(${TARGET})
        set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE)
    else()
        jamsat_configure_target(${TARGET})
    endif()
endmacro()

# Adds a JamSAT core library.
# Usage: add_jamsat_core_library(TargetName Source1.cpp Source2.cpp ...)
# Creates an object library target ${TargetName} containing the objects
# of the given source files. If neccessary, a target ${TargetName}-so
# is created with compiler flags for shared-object building.
function(add_jamsat_core_library TARGET)
  add_library(${TARGET} OBJECT ${ARGN})
  jamsat_configure_jamcore_library(${TARGET} FALSE)

  if(JAMSAT_BUILD_SOLIBS_SEPARATELY)
    add_library(${TARGET}-so OBJECT ${ARGN})
    jamsat_configure_jamcore_library(${TARGET}-so TRUE)
  endif()
endfunction()

add_subdirectory(cnfproblem)
add_subdirectory(utils)
add_subdirectory(clausedb)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(proof)
add_subdirectory(api)

set(jamcore-objects
  $<TARGET_OBJECTS:jamsatCnfProblem>
  $<TARGET_OBJECTS:jamsatUtils>
  $<TARGET_OBJECTS:jamsatClauseDB>
  $<TARGET_OBJECTS:jamsatSolver>
  $<TARGET_OBJECTS:jamsatBranching>
  $<TARGET_OBJECTS:jamsatProof>
  $<TARGET_OBJECTS:jamsatApiIPASIR>
)

if(JAMSAT_BUILD_SOLIBS_SEPARATELY)
  set(jamcore-shared-objects
    $<TARGET_OBJECTS:jamsatCnfProblem-so>
    $<TARGET_OBJECTS:jamsatUtils-so>
    $<TARGET_OBJECTS:jamsatClauseDB-so>
    $<TARGET_OBJECTS:jamsatSolver-so>
    $<TARGET_OBJECTS:jamsatBranching-so>
    $<TARGET_OBJECTS:jamsatProof-so>
    $<TARGET_OBJECTS:jamsatApiIPASIR-so>
  )
else()
  set(jamcore-shared-objects ${jamcore-objects})
endif()

add_library(jamcore STATIC ${jamcore-objects})
jamsat_configure_jamcore_library(jamcore FALSE)
target_link_libraries(jamcore ${JAMSAT_LIB_DEPS})
set_property(TARGET jamcore PROPERTY OUTPUT_NAME "jamsats")

add_library(jamcore-shared SHARED ${jamcore-shared-objects})
jamsat_configure_jamcore_library(jamcore-shared TRUE)
target_link_libraries(jamcore-shared ${JAMSAT_LIB_DEPS})
set_property(TARGET jamcore-shared PROPERTY OUTPUT_NAME "jamsatd")
