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

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
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
