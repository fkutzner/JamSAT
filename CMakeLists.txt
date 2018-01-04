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

### Find libraries

#add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost
        1.54.0
        REQUIRED
        COMPONENTS
          log
          log_setup
          iostreams
          thread
          system
)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

set(JAMSAT_LIB_DEPS ${Boost_LIBRARIES})
if(UNIX)
  # TODO: use single-threaded Boost if possible
  set(JAMSAT_LIB_DEPS ${JAMSAT_LIB_DEPS} pthread)
endif()

include_directories(SYSTEM lib/state_ptr/include)


### Compiler arguments

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(COMPILING_WITH_CLANG true)
  set(COMPILING_WITH_GNULIKE true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(COMPILING_WITH_GXX true)
  set(COMPILING_WITH_GNULIKE true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(COMPILING_WITH_MSVC true)
else()
  message(WARNING "Unknown compiler ${CMAKE_CXX_COMPILER_ID}, compiling with default parameters.")
endif()

if(COMPILING_WITH_GNULIKE)
  add_definitions(-std=c++14 -Wall -Wextra -pedantic-errors -fvisibility=hidden)

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
endif()

### Add JamSAT sources

add_subdirectory(src)


### Enable testing

add_subdirectory(lib/googletest)
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
