add_definitions(${JAMSAT_COMPILE_OPTIONS})
add_definitions(${JAMSAT_SANITIZER_COMPILE_OPTIONS})

# Disable warnings for dependencies
if(COMPILING_WITH_GNULIKE)
    add_definitions(-w)
    remove_definitions(-pedantic-errors)
endif()
if(COMPILING_WITH_MSVC)
    add_definitions(/w)
endif()

# Relax standard-compliance requirements (e.g. minisat uses zero-length arrays)
set(CMAKE_CXX_EXTENSIONS ON)

add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_subdirectory(minisat EXCLUDE_FROM_ALL)
