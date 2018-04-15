macro(add_general_compile_options OPTION)
  list(APPEND JAMSAT_COMPILE_OPTIONS ${OPTION} ${ARGN})
endmacro()

macro(add_solib_compile_options OPTION)
  list(APPEND JAMSAT_SOLIB_COMPILE_OPTIONS ${OPTION} ${ARGN})
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
  target_compile_options(${TARGET} PRIVATE ${JAMSAT_SOLIB_COMPILE_OPTIONS})

  if(PLATFORM_REQUIRES_EXTRA_PIC_FOR_SOLIBS)
    if(COMPILING_WITH_GNULIKE)
      target_compile_options(${TARGET} PRIVATE -fPIC)
    else()
      message(WARNING "PIC compiler flags unknown for this compiler. Not adding PIC flags for shared libraries.")
    endif()
  endif()
endmacro()
