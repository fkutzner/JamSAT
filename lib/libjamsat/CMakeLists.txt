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

if(JAMSAT_BUILD_STATIC_LIB)
  set(JAMSAT_LIBRARY_KIND STATIC)
  set(JAMSAT_OBJ_LIBRARY_KIND OBJECT)
  set(JAMSAT_LIB_PACKAGE_NAME "JamSATStatic")
  set(JAMSAT_LIB_ARTIFACT_NAME "jamsats")
else()
  nm_add_lib_compile_definitions(PRIVATE -DBUILDING_JAMSAT_SHARED_LIB)
  nm_add_lib_compile_definitions(PUBLIC -DJAMSAT_SHARED_LIB)
  set(JAMSAT_LIBRARY_KIND SHARED)
  set(JAMSAT_OBJ_LIBRARY_KIND OBJECT-SHARED)
  set(JAMSAT_LIB_PACKAGE_NAME "JamSATShared")
  set(JAMSAT_LIB_ARTIFACT_NAME "jamsatd")
endif()


# Adds a JamSAT core library.
#
# Usage: add_jamsat_core_library(TargetName Source1.cpp Source2.cpp ...)
#
# Creates an object library target ${TargetName} containing the objects
# of the given source files. The object library is created using nm_add_library()
# with library kind ${JAMSAT_OBJ_LIBRARY_KIND}.
function(add_jamsat_core_library TARGET)
  nm_add_library(${TARGET} ${JAMSAT_OBJ_LIBRARY_KIND} ${ARGN})

  # Add lib/ to the include search path, as JamSAT modules currently contain their public
  # headers:
  target_include_directories(${TARGET} PRIVATE "${PROJECT_SOURCE_DIR}/${NM_CONF_LIB_DIR}")
endfunction()

add_subdirectory(concepts)
add_subdirectory(cnfproblem)
add_subdirectory(drivers)
add_subdirectory(utils)
add_subdirectory(clausedb)
add_subdirectory(solver)
add_subdirectory(branching)
add_subdirectory(proof)
add_subdirectory(api)
add_subdirectory(simplification)

# Collect libjamsat.* object libraries:
nm_collect_objs_from_subdirectories(COLLECT_IN libjamsat-objects
                                    TARGET_NAME_PREFIX "libjamsat."
                                    EXCLUDE_REGEX "^api$")
nm_add_library(libjamsat ${JAMSAT_LIBRARY_KIND} ${libjamsat-objects})
set_property(TARGET libjamsat PROPERTY OUTPUT_NAME ${JAMSAT_LIB_ARTIFACT_NAME})


# Due to symbol hiding, the tests can't directly use the libjamsat DSO, so the object
# files are packaged into a static library archive as well:
nm_add_library(libjamsat-testing STATIC ${libjamsat-objects})
if(NOT JAMSAT_BUILD_STATIC_LIB)
  # On Windows/MSVC, clients of libjamsat-testing can't use dllimport declarations to refer
  # to dllexport symbols defined within libjamsat-testing. Thus a workaround: make sure
  # libjamsat-testing clients see the JamSAT API functions as dllexport declarations, not
  # as dllimport declarations
  target_compile_definitions(libjamsat-testing INTERFACE -DBUILDING_JAMSAT_SHARED_LIB)
endif()
target_include_directories(libjamsat-testing INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${NM_CONF_LIB_DIR}>)
set_property(TARGET libjamsat-testing PROPERTY OUTPUT_NAME ${JAMSAT_LIB_ARTIFACT_NAME}_objs)


### Installation

# Automatically add the directory containing the jamsat header directory
# to clients linking against libjamsat
target_include_directories(libjamsat INTERFACE $<INSTALL_INTERFACE:include>)

# Install API headers
install(FILES ${PROJECT_SOURCE_DIR}/${NM_CONF_INCLUDE_DIR}/libjamsat/JamSatIpasir.h
        DESTINATION include/jamsat)

# Install JamSAT libraries
install(
  TARGETS libjamsat
  EXPORT ${JAMSAT_LIB_PACKAGE_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# Generate and install CMake scripts for find_package
install(EXPORT ${JAMSAT_LIB_PACKAGE_NAME}
        DESTINATION lib/cmake/${JAMSAT_LIB_PACKAGE_NAME}
        FILE "${JAMSAT_LIB_PACKAGE_NAME}Config.cmake")
