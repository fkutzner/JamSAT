macro(add_general_compile_options OPTION)
  list(APPEND JAMSAT_COMPILE_OPTIONS ${OPTION} ${ARGN})
endmacro()

macro(add_sanitizer_compile_options OPTION)
  list(APPEND JAMSAT_SANITIZER_COMPILE_OPTIONS ${OPTION} ${ARGN})
endmacro()

macro(remove_sanitizer_compile_options OPTION)
  list(REMOVE_ITEM JAMSAT_SANITIZER_COMPILE_OPTIONS ${OPTION} ${ARGN})
endmacro()


macro(jamsat_configure_target TARGET)
  target_compile_options(${TARGET} PRIVATE ${JAMSAT_COMPILE_OPTIONS})
  target_compile_options(${TARGET} PRIVATE ${JAMSAT_SANITIZER_COMPILE_OPTIONS})
endmacro()

macro(jamsat_configure_solib_target TARGET)
  target_compile_options(${TARGET} PRIVATE ${JAMSAT_COMPILE_OPTIONS})
endmacro()
