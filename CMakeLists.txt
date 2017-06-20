project(JamSAT)
cmake_minimum_required(VERSION 3.5)

add_definitions(-std=c++11 -Wall -Wextra)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -O0 -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined")

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
find_package(Boost
        1.63.0
        REQUIRED
        COMPONENTS
          container
          log
)


include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

add_subdirectory(doc)

add_subdirectory(lib/googletest)
enable_testing()

add_subdirectory(src)

add_subdirectory(testsrc)


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
