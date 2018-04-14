SET(JAMSAT_LIBRARY_TEST_INFRASTR_FOLDER_NAME "JamSAT Test Infrastructure")

add_library(jamsatFuzzGenericFuzzAdapter OBJECT
  GenericFuzzAdapter.cpp
)
set_property(TARGET jamsatFuzzGenericFuzzAdapter
             PROPERTY FOLDER ${JAMSAT_LIBRARY_TEST_INFRASTR_FOLDER_NAME})
