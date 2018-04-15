project(JamSAT)
cmake_minimum_required(VERSION 3.2.2)

### Options

# The default for all boolean options is "OFF".
option(JAMSAT_ENABLE_SANITIZERS "Enable code sanitizers" OFF)
option(JAMSAT_USE_SEPARATE_CLANG_SANITIZERS "Activate sanitizers separately for clang" OFF)
option(JAMSAT_ENABLE_MEMORY_SANITIZER "When using sanitizers, also enable clang's memory sanitizer" OFF)
option(JAMSAT_ENABLE_AFL_FUZZER "Use afl-clang rsp. afl-gcc for compilation and build the fuzzing targets for AFL" OFF)

option(JAMSAT_DISABLE_OPTIMIZATIONS "Disable compiler optimimzations" OFF)
option(JAMSAT_ENABLE_RELEASE_ASSERTIONS "Enable release-mode assertions" OFF)
option(JAMSAT_ENABLE_COVERAGE "Enable code coverage" OFF)
option(JAMSAT_ENABLE_LOGGING "Enable logging for debug" OFF)

option(JAMSAT_DISABLE_BOOST_LINKING_SETUP "Don't override linker settings for Boost" OFF)

### Load CMake code

# Defines COMPILING_WITH_* and PLATFORM_SUPPORTS_* variables
include(etc/cmake/Platform.cmake)

### Language options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_EXTENSIONS OFF)

### Find libraries

if (JAMSAT_ENABLE_LOGGING)
  list(APPEND JAMSAT_REQUIRED_BOOST_LIBRARIES log log_setup)
endif()

if (NOT JAMSAT_DISABLE_BOOST_LINKING_SETUP)
  set(Boost_USE_STATIC_LIBS OFF)
endif()

find_package(Boost
    1.54.0
    REQUIRED
    COMPONENTS
      ${JAMSAT_REQUIRED_BOOST_LIBRARIES}
)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

set(JAMSAT_LIB_DEPS ${Boost_LIBRARIES})

if(UNIX AND Boost_USE_MULTITHREADED)
  set(JAMSAT_LIB_DEPS ${JAMSAT_LIB_DEPS} pthread)
endif()

include_directories(SYSTEM lib/state_ptr/include)


### Fix DLL lookup e.g. on Windows

if ((NOT PLATFORM_SUPPORTS_RPATH_LIKE_SO_LOOKUP)
    AND PLATFORM_SUPPORTS_SO_LOOKUP_IN_PATH)
  set(JAMSAT_CTEST_PATH_PREFIIX "${Boost_LIBRARY_DIRS}")
  # TODO: fix separators in ${Boost_LIBRARY_DIRS} add path to ctests
endif()

### Compiler arguments

# Opt out of boost's linker magic, which causes linker errors on Windows.
# See https://www.boost.org/doc/libs/1_57_0/libs/config/doc/html/index.html
add_definitions(-DBOOST_ALL_NO_LIB)

# BOOST_ALL_NO_LIB does not disable boost's linker magic entirely :(
if (JAMSAT_ENABLE_LOGGING AND NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_LOG_DYN_LINK=1)
endif()

if(COMPILING_WITH_GNULIKE)
  add_definitions(-Wall -Wextra -pedantic-errors -fvisibility=hidden)

  if(JAMSAT_DISABLE_OPTIMIZATIONS)
    add_definitions(-fno-inline -O0)
  endif()

  if(JAMSAT_ENABLE_SANITIZERS)
    if(COMPILING_WITH_CLANG)
      # Configure the ASAN suppression file to disable warnings in 3rd party libraries
      configure_file(${PROJECT_SOURCE_DIR}/etc/asan.supp.in ${PROJECT_BINARY_DIR}/asan.supp)
      set(sanitizer_flags
          "-fsanitize-blacklist=${PROJECT_BINARY_DIR}/asan.supp -fsanitize=address -fsanitize=undefined")
      add_definitions(-fno-omit-frame-pointer ${sanitizer_flags})
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${sanitizer_flags}")

      if(COMPILING_WITH_CLANG AND JAMSAT_USE_SEPARATE_CLANG_SANITIZERS)
        add_definitions(-fsanitize=leak)
      endif()

      if(COMPILING_WITH_CLANG AND JAMSAT_ENABLE_MEMORY_SANITIZER)
        # TODO: don't enable leak and address sanitizers in the first place since
        # they are not compatible with the memory sanitizer
        remove_definitions(-fsanitize=leak)
        remove_definitions(-fsanitize=address)
        add_definitions(-fsanitize=memory)
      endif()
    else()
      message(ERROR "GCC sanitizer support is not implemented yet")
    endif()
  endif()

  if(JAMSAT_ENABLE_COVERAGE)
    add_definitions(--coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()

  if(JAMSAT_ENABLE_RELEASE_ASSERTIONS)
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL})
  endif()

elseif(COMPILING_WITH_MSVC)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_definitions(/EHsc)

  # Disable warnings about usage of C++ TR1. Google Test apparently still uses
  # TR1 functions.
  add_definitions(/wd4996)

  add_definitions(/W4)
endif()

### Set preprocessor definitions
if(JAMSAT_ENABLE_LOGGING)
  add_definitions(-DJAM_ENABLE_LOGGING)
  add_definitions(-DJAM_ENABLE_PROPAGATION_LOGGING)
  add_definitions(-DJAM_ENABLE_CA_LOGGING)
  add_definitions(-DJAM_ENABLE_CNFPROBLEM_LOGGING)
  add_definitions(-DJAM_ENABLE_CDCLITEST_LOGGING)
  add_definitions(-DJAM_ENABLE_REDUCE_LOGGING)
  add_definitions(-DJAM_ENABLE_SOLVER_LOGGING)
  add_definitions(-DJAM_ENABLE_MINIMIZER_LOGGING)
endif()

### Place artifacts in the artifacts/ subdirectories of the build root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)

### Add JamSAT sources

add_subdirectory(src)


### Enable testing

add_subdirectory(lib/googletest EXCLUDE_FROM_ALL)
enable_testing()
add_subdirectory(testsrc)


### Auxiliary targets

add_subdirectory(doc)

file(GLOB_RECURSE SRC_SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE TESTSRC_SOURCE_FILES ${PROJECT_SOURCE_DIR}/testsrc/*.cpp ${PROJECT_SOURCE_DIR}/testsrc/*.h)
set(ALL_SOURCE_FILES ${SRC_SOURCE_FILES} ${TESTSRC_SOURCE_FILES})

set(JAMSAT_MAINTENANCE_TARGET_FOLDER "Maintenance Targets")

add_custom_target(
        format-src
        COMMAND clang-format
        -i
        ${ALL_SOURCE_FILES}
)
set_property(TARGET format-src PROPERTY FOLDER ${JAMSAT_MAINTENANCE_TARGET_FOLDER})

add_custom_target(
        cppcheck
        COMMAND cppcheck
        -I ${PROJECT_SOURCE_DIR}/src
        -I ${PROJECT_SOURCE_DIR}/testsrc
        --enable=all
        --suppress=missingIncludeSystem
        --std=c++14
        --verbose
        ${ALL_SOURCE_FILES}
)
set_property(TARGET cppcheck PROPERTY FOLDER ${JAMSAT_MAINTENANCE_TARGET_FOLDER})
