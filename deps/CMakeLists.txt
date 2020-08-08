add_definitions(${JAMSAT_COMPILE_OPTIONS})
add_definitions(${JAMSAT_SANITIZER_COMPILE_OPTIONS})

# Disable warnings for dependencies
if(NM_COMPILING_WITH_GNULIKE)
    add_definitions(-w)
    remove_definitions(-pedantic-errors)
endif()
if(NM_COMPILING_WITH_MSVC)
    add_definitions(/w)
endif()

if(JAMSAT_ENABLE_TESTING)
  # Relax standard-compliance requirements (e.g. minisat uses zero-length arrays)
  set(CMAKE_CXX_EXTENSIONS ON)
  add_subdirectory(minisat EXCLUDE_FROM_ALL)
endif()

add_subdirectory(CLI11)

# Note about header-only libraries used by libjamsat: using interface libraries
# breaks the exports for the static library build, see e.g.
# https://gitlab.kitware.com/cmake/cmake/-/issues/15415#note_333595
# ~> Adding the include directories directly as a (hopefully preliminary) workaround
