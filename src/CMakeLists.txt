add_subdirectory(libjamsat)

if(NOT JAMSAT_DISABLE_FRONTEND)
  add_subdirectory(jamsat)
endif()
