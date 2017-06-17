add_subdirectory(cnfproblem)

add_library(jamcore STATIC $<TARGET_OBJECTS:jamsatCnfProblem>)
