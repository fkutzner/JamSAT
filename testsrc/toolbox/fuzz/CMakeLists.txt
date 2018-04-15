SET(JAMSAT_LIBRARY_TEST_INFRASTR_FOLDER_NAME "JamSAT Test Infrastructure")

add_library(jamsatFuzzGenericFuzzAdapter OBJECT
  GenericFuzzAdapter.cpp
)

jamsat_configure_target(jamsatFuzzGenericFuzzAdapter)
set_property(TARGET jamsatFuzzGenericFuzzAdapter
             PROPERTY FOLDER ${JAMSAT_LIBRARY_TEST_INFRASTR_FOLDER_NAME})
