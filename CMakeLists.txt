cmake_minimum_required(VERSION 3.2.2)
cmake_policy(VERSION 3.2.2)

project(JamSAT VERSION 0.0.1)

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
option(JAMSAT_LOGGING_DEFAULT_START_EPOCH "The default logging epoch in which logging starts" 0)
option(JAMSAT_ENABLE_EXPENSIVE_ASSERTIONS "Enable more thorough, but expensive assertions" OFF)

option(JAMSAT_DISABLE_BOOST_LINKING_SETUP "Don't override linker settings for Boost" OFF)
option(JAMSAT_LINK_VERBOSE "Print information about linked dependencies" OFF)
option(JAMSAT_ENABLE_SYMBOL_HIDING_IN_STATIC_LIB "Enables symbol hiding for the static JamSAT library" OFF)

### Load CMake code

include(etc/cmake/Platform.cmake) # Defines COMPILING_WITH_* and PLATFORM_SUPPORTS_* variables
include(etc/cmake/CompilerOptions.cmake)
include(etc/cmake/Utils.cmake)

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

if(JAMSAT_LINK_VERBOSE)
  message_list(STATUS "${JAMSAT_LIB_DEPS}" "JamSAT will be linked to the following external libraries:")
endif()


### Fix DLL lookup e.g. on Windows

if ((NOT PLATFORM_SUPPORTS_RPATH_LIKE_SO_LOOKUP)
    AND PLATFORM_SUPPORTS_SO_LOOKUP_IN_PATH)
  set(JAMSAT_CTEST_PATH_PREFIIX "${Boost_LIBRARY_DIRS}")
  # TODO: fix separators in ${Boost_LIBRARY_DIRS} add path to ctests
endif()

### Compiler arguments

# Opt out of boost's linker magic, which causes linker errors on Windows.
# See https://www.boost.org/doc/libs/1_57_0/libs/config/doc/html/index.html
add_general_compile_options(-DBOOST_ALL_NO_LIB)

# BOOST_ALL_NO_LIB does not disable boost's linker magic entirely :(
if (JAMSAT_ENABLE_LOGGING AND NOT Boost_USE_STATIC_LIBS)
    add_general_compile_options(-DBOOST_LOG_DYN_LINK=1)
endif()

if(COMPILING_WITH_GNULIKE)
  add_general_compile_options(-Wall -Wextra -pedantic-errors)
  if(JAMSAT_ENABLE_SYMBOL_HIDING_IN_STATIC_LIB)
    add_general_compile_options(-fvisibility=hidden)
  else()
    add_dso_compile_options(-fvisibility=hidden)
  endif()

  if(JAMSAT_DISABLE_OPTIMIZATIONS)
    add_general_compile_options(-fno-inline -O0)
  endif()

  if(JAMSAT_ENABLE_SANITIZERS)
    if(COMPILING_WITH_CLANG)
      # Configure the ASAN suppression file to disable warnings in 3rd party libraries
      configure_file(${PROJECT_SOURCE_DIR}/etc/asan.supp.in ${PROJECT_BINARY_DIR}/asan.supp)
      set(sanitizer_flags
          "-fsanitize-blacklist=${PROJECT_BINARY_DIR}/asan.supp" -fsanitize=address -fsanitize=undefined)
      add_sanitizer_compile_options(-fno-omit-frame-pointer ${sanitizer_flags})

      string(REPLACE ";" " " SANITIZER_LINKER_FLAGS "${sanitizer_flags}")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_LINKER_FLAGS}")

      if(COMPILING_WITH_CLANG AND JAMSAT_USE_SEPARATE_CLANG_SANITIZERS)
        add_sanitizer_compile_options(-fsanitize=leak)
      endif()

      if(COMPILING_WITH_CLANG AND JAMSAT_ENABLE_MEMORY_SANITIZER)
        # TODO: don't enable leak and address sanitizers in the first place since
        # they are not compatible with the memory sanitizer
        remove_sanitizer_compile_options(-fsanitize=leak)
        remove_sanitizer_compile_options(-fsanitize=address)
        add_sanitizer_compile_options(-fsanitize=memory)
      endif()
    else()
      message(ERROR "GCC sanitizer support is not implemented yet")
    endif()
  endif()

  if(JAMSAT_ENABLE_COVERAGE)
    add_general_compile_options(--coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()

  if(JAMSAT_ENABLE_RELEASE_ASSERTIONS)
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL})
  endif()

elseif(COMPILING_WITH_MSVC)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_general_compile_options(/EHsc)

  # Disable warnings about usage of C++ TR1. Google Test apparently still uses
  # TR1 functions.
  add_general_compile_options(/wd4996)

  add_general_compile_options(/W4)
endif()

### Set preprocessor definitions
if(JAMSAT_ENABLE_LOGGING)
  add_general_compile_options(
    -DJAM_ENABLE_LOGGING
    -DJAM_ENABLE_PROPAGATION_LOGGING
    -DJAM_ENABLE_CA_LOGGING
    -DJAM_ENABLE_CNFPROBLEM_LOGGING
    -DJAM_ENABLE_CDCLITEST_LOGGING
    -DJAM_ENABLE_REDUCE_LOGGING
    -DJAM_ENABLE_SOLVER_LOGGING
    -DJAM_ENABLE_MINIMIZER_LOGGING
    -DJAM_LOG_DEFAULT_STARTLOGGINGEPOCH=${JAMSAT_LOGGING_DEFAULT_START_EPOCH}
  )
endif()

if(JAMSAT_ENABLE_EXPENSIVE_ASSERTIONS)
    add_general_compile_options(-DJAM_ENABLE_EXPENSIVE_ASSERTIONS)
endif()

add_general_compile_options(-DJAMSAT_SIGNATURE="JamSAT ${JamSAT_VERSION}")

add_dso_compile_options(-DJAMSAT_SHARED_LIB -DBUILDING_JAMSAT_SHARED_LIB)

# Now: compile options for all targets are listed in JAMSAT_COMPILE_OPTIONS
#      compile options for targets supporting sanitizers are listed in
#        JAMSAT_SANITIZER_COMPILE_OPTIONS

message_list(STATUS "${JAMSAT_COMPILE_OPTIONS}" "General JamSAT compiler options:")
message_list(STATUS "${JAMSAT_SANITIZER_COMPILE_OPTIONS}" "Sanitizing-related compiler options:")
message_list(STATUS "${JAMSAT_DSO_COMPILE_OPTIONS}" "Shared-object/DLL compiler options:")

### Place artifacts in the artifacts/ subdirectories of the build root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts/bin)

### Compile dependencies added via git submodules
add_subdirectory(lib)

### Add JamSAT sources

add_subdirectory(src)


### Enable testing

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
