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

cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

project(JamSAT VERSION 0.2.0)

### Options

# The default for all boolean options is "OFF".
option(JAMSAT_ENABLE_AFL_FUZZER "Use afl-clang rsp. afl-gcc for compilation and build the fuzzing targets for AFL" OFF)
option(JAMSAT_DISABLE_OPTIMIZATIONS "Disable compiler optimimzations" OFF)
option(JAMSAT_ENABLE_RELEASE_ASSERTIONS "Enable release-mode assertions" OFF)
option(JAMSAT_ENABLE_COVERAGE "Enable code coverage" OFF)
option(JAMSAT_ENABLE_LOGGING "Enable logging for debug" OFF)
option(JAMSAT_LOGGING_DEFAULT_START_EPOCH "The default logging epoch in which logging starts" 0)
option(JAMSAT_ENABLE_EXPENSIVE_ASSERTIONS "Enable more thorough, but expensive assertions" OFF)
option(JAMSAT_DISABLE_BOOST_LINKING_SETUP "Don't override linker settings for Boost" OFF)
option(JAMSAT_ENABLE_TESTING "Enable building the JamSAT test suite." OFF)
option(JAMSAT_BUILD_STATIC_LIB "Build libjamsat as a static library" OFF)
option(JAMSAT_USE_SYSTEM_GSL "Search for Microsoft GSL in the CMake module path." OFF)

### Load CMake code

set(NM_CONF_GTEST_TAG="release-1.8.1")
include(etc/cmake/nicemake/NiceMake.cmake)
include(etc/cmake/nicemake/experimental/CollectObjsFromSubdirectories.cmake)

### Language options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_EXTENSIONS OFF)

### Set up 3rd party libraries that shall be ubiquitously available
### (Boost; state_ptr; pthread if required by Boost).
### Note: zlib is required by the JamSAT frontend and is added to the
### 3rd party libraries in lib/libjamfrontend
include(etc/cmake/JamSATThirdPartyLibs.cmake)


### Compiler arguments

# Opt out of boost's linker magic, which causes linker errors on Windows.
# See https://www.boost.org/doc/libs/1_57_0/libs/config/doc/html/index.html
nm_add_compile_definitions(PRIVATE -DBOOST_ALL_NO_LIB)

# BOOST_ALL_NO_LIB does not disable boost's linker magic entirely :(
if (JAMSAT_ENABLE_LOGGING AND NOT Boost_USE_STATIC_LIBS)
    nm_add_compile_definitions(PRIVATE -DBOOST_LOG_DYN_LINK=1)
endif()

nm_enforce_lang_standard_adherence()
nm_use_high_compiler_warning_level()

if(JAMSAT_ENABLE_RELEASE_ASSERTIONS)
  nm_enable_release_assertions()
endif()

if(NM_COMPILING_WITH_GNULIKE)
  if(NOT JAMSAT_BUILD_STATIC_LIB)
    nm_add_lib_compile_options(PRIVATE -fvisibility=hidden)
  endif()

  if(JAMSAT_DISABLE_OPTIMIZATIONS)
    nm_add_compile_options(PRIVATE -fno-inline -O0)
  endif()

  if(JAMSAT_ENABLE_COVERAGE)
    nm_add_compile_options(PRIVATE --coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()
elseif(NM_COMPILING_WITH_MSVC)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  nm_add_compile_options(PUBLIC /EHsc)

  # Suppress warnings about unused parameters. Some dependencies have functions
  # with parameters that are only used in debug builds.
  # TODO: move this. nm_add_compile_options(/wd4100)

  # Disable warnings about missed opportunities for using constexpr if (enable
  # this when moving to C++17)
  nm_add_compile_options(PRIVATE /wd4127)

  nm_add_compile_options(PRIVATE /W4)
endif()

### Set preprocessor definitions
if(JAMSAT_ENABLE_LOGGING)
  nm_add_compile_definitions(PRIVATE
    -DJAM_ENABLE_LOGGING
    -DJAM_ENABLE_ASSIGNMENT_LOGGING
    -DJAM_ENABLE_CA_LOGGING
    -DJAM_ENABLE_CNFPROBLEM_LOGGING
    -DJAM_ENABLE_CDCLITEST_LOGGING
    -DJAM_ENABLE_CLAUSEDB_LOGGING
    -DJAM_ENABLE_REDUCE_LOGGING
    -DJAM_ENABLE_SOLVER_LOGGING
    -DJAM_ENABLE_MINIMIZER_LOGGING
    -DJAM_ENABLE_INFLIGHTSIMP_LOGGING
    -DJAM_ENABLE_CERT_LOGGING
    -DJAM_LOG_DEFAULT_STARTLOGGINGEPOCH=${JAMSAT_LOGGING_DEFAULT_START_EPOCH}
  )
endif()

if(JAMSAT_ENABLE_EXPENSIVE_ASSERTIONS)
    nm_add_compile_definitions(PRIVATE -DJAM_ENABLE_EXPENSIVE_ASSERTIONS)
endif()

nm_add_compile_definitions(PUBLIC -DJAMSAT_SIGNATURE="JamSAT ${JamSAT_VERSION}")

if(NOT JAMSAT_BUILD_STATIC_LIB)
  nm_add_lib_compile_definitions(PUBLIC -DJAMSAT_SHARED_LIB)
endif()


### Configure RPATH on macOS

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  # See https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endif()


### Set up dependencies added via git submodules

add_subdirectory(deps)


### Add JamSAT sources

add_subdirectory(lib)
add_subdirectory(tools)


### Enable testing

if(JAMSAT_ENABLE_TESTING)
  nm_add_gtest()
  enable_testing()
  add_subdirectory(testsrc)
endif()

### Auxiliary targets

nm_add_doxygen()
include(etc/cmake/HelperTargets.cmake)
