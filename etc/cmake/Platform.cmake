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

# This file sets the following variables:
#
# COMPILING_WITH_GNULIKE                     when compiling with clang or g++
# COMPILING_WITH_CLANG                       when compiling with clang++
# COMPILING_WITH_GXX                         when compiling with g++
# COMPILING_WITH_MSVC                        when compiling with Microsoft Visual Studio
#
# PLATFORM_SUPPORTS_RPATH_LIKE_SO_LOOKUP     when the target platform has RPATH-like
#                                            shared-object-lookup support
# PLATFORM_SUPPORTS_SO_LOOKUP_IN_PATH        when the target platform supports
#                                            lookup up shared objects in $PATH

if(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang"))
  set(COMPILING_WITH_CLANG true)
  set(COMPILING_WITH_GNULIKE true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(COMPILING_WITH_GXX true)
  set(COMPILING_WITH_GNULIKE true)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(COMPILING_WITH_MSVC true)
else()
  message(WARNING "Unknown compiler ${CMAKE_CXX_COMPILER_ID}, compiling with default parameters.")
endif()


if(WIN32 OR CYGWIN)
  set(PLATFORM_SUPPORTS_RPATH_LIKE_SO_LOOKUP OFF)
  set(PLATFORM_SUPPORTS_SO_LOOKUP_IN_PATH ON)
elseif(UNIX)
  set(PLATFORM_SUPPORTS_RPATH_LIKE_SO_LOOKUP ON)
  set(PLATFORM_SUPPORTS_SO_LOOKUP_IN_PATH OFF)
else()
  message(WARNING "Unknown platform.")
  set(PLATFORM_SUPPORTS_RPATH_LIKE_SO_LOOKUP OFF)
  set(PLATFORM_SUPPORTS_SO_LOOKUP_IN_PATH OFF)
endif()

set(PLATFORM_REQUIRES_EXTRA_PIC_FOR_DSO FALSE)
if(UNIX AND (NOT APPLE))
  set(PLATFORM_REQUIRES_EXTRA_PIC_FOR_DSO TRUE)
endif()
