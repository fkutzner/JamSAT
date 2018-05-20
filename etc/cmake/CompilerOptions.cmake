# Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Except as contained in this notice, the name(s) of the above copyright holders
# shall not be used in advertising or otherwise to promote the sale, use or
# other dealings in this Software without prior written authorization.

macro(add_general_compile_options OPTION)
  list(APPEND JAMSAT_COMPILE_OPTIONS ${OPTION} ${ARGN})
endmacro()

macro(add_dso_compile_options OPTION)
  list(APPEND JAMSAT_DSO_COMPILE_OPTIONS ${OPTION} ${ARGN})
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

macro(jamsat_configure_dso_target TARGET)
  target_compile_options(${TARGET} PRIVATE ${JAMSAT_COMPILE_OPTIONS})
  target_compile_options(${TARGET} PRIVATE ${JAMSAT_DSO_COMPILE_OPTIONS})

  if(PLATFORM_REQUIRES_EXTRA_PIC_FOR_DSO)
    if(COMPILING_WITH_GNULIKE)
      target_compile_options(${TARGET} PRIVATE -fPIC)
    else()
      message(WARNING "PIC compiler flags unknown for this compiler. Not adding PIC flags for shared libraries.")
    endif()
  endif()
endmacro()
