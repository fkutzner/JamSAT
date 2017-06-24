project(JamSAT)
cmake_minimum_required(VERSION 3.5)

### Options
option(JAMSAT_ENABLE_SANITIZERS "Enable code sanitizers" OFF)
option(JAMSAT_DISABLE_OPTIMIZATIONS "Disable compiler optimimzations" OFF)
option(JAMSAT_ENABLE_COVERAGE "Enable code coverage" OFF)

### Find libraries

add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost
        1.63.0
        REQUIRED
        COMPONENTS
          container
          log
)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})


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
  add_definitions(-std=c++11 -Wall -Wextra)

  if(JAMSAT_DISABLE_OPTIMIZATIONS)
    add_definitions(-fno-inline -O0)
  endif()

  if(JAMSAT_ENABLE_SANITIZERS)
    if(COMPILING_WITH_CLANG)
      # Configure the ASAN suppression file to disable warnings in 3rd party libraries
      configure_file(${PROJECT_SOURCE_DIR}/aux/asan.supp.in ${PROJECT_BINARY_DIR}/asan.supp)
      set(sanitizer_flags
          "-fsanitize-blacklist=${PROJECT_BINARY_DIR}/asan.supp -fsanitize=address -fsanitize=undefined")
      add_definitions(-fno-omit-frame-pointer ${sanitizer_flags})
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${sanitizer_flags}")
    else()
      message(ERROR "GCC sanitizer support is not implemented yet")
    endif()
  endif()

  if(JAMSAT_ENABLE_COVERAGE)
    add_definitions(--coverage)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif()

elseif(COMPILING_WITH_MSVC)
  message(ERROR "MSVC support is not implemented yet")
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
file(GLOB_RECURSE TESTSRC_SOURCE_FILES ${PROJECT_SOURCE_DIR}/testsrc/*.cpp *${PROJECT_SOURCE_DIR}/testsrc/*.h)
set(ALL_SOURCE_FILES ${SRC_SOURCE_FILES} ${TESTSRC_SOURCE_FILES})

add_custom_target(
        format-src
        COMMAND clang-format
        -style=LLVM
        -i
        ${ALL_SOURCE_FILES}
)

add_custom_target(
        cppcheck
        COMMAND cppcheck
        -I ${PROJECT_SOURCE_DIR}/src
        -I ${PROJECT_SOURCE_DIR}/testsrc
        --enable=all
        --suppress=missingIncludeSystem
        --std=c++11
        --verbose
        ${ALL_SOURCE_FILES}
)
