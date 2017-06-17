include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(cnfproblem)
add_subdirectory(utils)

add_library(jamcore STATIC $<TARGET_OBJECTS:jamsatCnfProblem>)
