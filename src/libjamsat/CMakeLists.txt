# Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name(s) of the above copyright holders
# shall not be used in advertising or otherwise to promote the sale, use or
# other dealings in this Software without prior written authorization.

set(JAMSAT_LIBRARY_FOLDER_NAME "JamSAT Library")

# Sets include directories, folders, ... for the given jamcore sub-library.
macro(jamsat_configure_jamcore_library TARGET IS_SHARED)
    target_include_directories(${TARGET} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
    set_property(TARGET ${TARGET} PROPERTY FOLDER ${JAMSAT_LIBRARY_FOLDER_NAME})

    if(${IS_SHARED})
        jamsat_configure_dso_target(${TARGET})
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

  if(JAMSAT_BUILD_DSO)
    add_library(${TARGET}-dso OBJECT ${ARGN})
    jamsat_configure_jamcore_library(${TARGET}-dso TRUE)
  endif()
endfunction()

add_subdirectory(concepts)
add_subdirectory(cnfproblem)
add_subdirectory(utils)
add_subdirectory(clausedb)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(proof)
add_subdirectory(api)
add_subdirectory(simplification)

set(jamcore-objects
  $<TARGET_OBJECTS:jamsatConcepts>
  $<TARGET_OBJECTS:jamsatCnfProblem>
  $<TARGET_OBJECTS:jamsatUtils>
  $<TARGET_OBJECTS:jamsatClauseDB>
  $<TARGET_OBJECTS:jamsatSolver>
  $<TARGET_OBJECTS:jamsatBranching>
  $<TARGET_OBJECTS:jamsatProof>
  $<TARGET_OBJECTS:jamsatApiIPASIR>
  $<TARGET_OBJECTS:jamsatSimplification>
)

if(JAMSAT_BUILD_DSO)
  set(jamcore-dso-objects
    $<TARGET_OBJECTS:jamsatConcepts>
    $<TARGET_OBJECTS:jamsatCnfProblem-dso>
    $<TARGET_OBJECTS:jamsatUtils-dso>
    $<TARGET_OBJECTS:jamsatClauseDB-dso>
    $<TARGET_OBJECTS:jamsatSolver-dso>
    $<TARGET_OBJECTS:jamsatBranching-dso>
    $<TARGET_OBJECTS:jamsatProof-dso>
    $<TARGET_OBJECTS:jamsatApiIPASIR-dso>
    $<TARGET_OBJECTS:jamsatSimplification-dso>
  )
endif()


add_library(jamcore STATIC ${jamcore-objects})
jamsat_configure_jamcore_library(jamcore FALSE)
target_link_libraries(jamcore ${JAMSAT_LIB_DEPS})
set_property(TARGET jamcore PROPERTY OUTPUT_NAME "jamsats")

if(JAMSAT_BUILD_DSO)
  add_library(jamcore-dso SHARED ${jamcore-dso-objects})
  jamsat_configure_jamcore_library(jamcore-dso TRUE)
  target_link_libraries(jamcore-dso ${JAMSAT_LIB_DEPS})
  set_property(TARGET jamcore-dso PROPERTY OUTPUT_NAME "jamsatd")
endif()


### Installation

# Automatically add the directory containing the jamsat header directory
# to clients linking against jamcore or jamcore-dso
target_include_directories(jamcore INTERFACE $<INSTALL_INTERFACE:include>)
set(INSTALL_TARGETS jamcore)

if(JAMSAT_BUILD_DSO)
  target_include_directories(jamcore-dso INTERFACE $<INSTALL_INTERFACE:include>)

  # JamSAT DSO clients should automatically define JAMSAT_SHARED_LIB to make
  # __declspec(dllimport) work properly
  target_compile_definitions(jamcore-dso INTERFACE -DJAMSAT_SHARED_LIB)
  list(APPEND INSTALL_TARGETS jamcore-dso)
endif()

# Install API headers
install(FILES api/ipasir/JamSatIpasir.h DESTINATION include/jamsat)

# Install JamSAT libraries
install(
  TARGETS ${INSTALL_TARGETS}
  EXPORT JamSAT
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# Generate and install CMake scripts for find_package
install(EXPORT JamSAT DESTINATION lib/cmake/JamSAT FILE JamSATConfig.cmake)
