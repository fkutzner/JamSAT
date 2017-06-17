project(JamSAT)
cmake_minimum_required(VERSION 3.5)

add_definitions(-std=c++11 -Wall -Wextra)

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
