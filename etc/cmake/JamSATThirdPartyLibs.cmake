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

list(APPEND JAMSAT_REQUIRED_BOOST_LIBRARIES filesystem)
if (JAMSAT_ENABLE_LOGGING)
  list(APPEND JAMSAT_REQUIRED_BOOST_LIBRARIES log log_setup)
endif()

if (NOT JAMSAT_DISABLE_BOOST_LINKING_SETUP)
  if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
  else()
    set(Boost_USE_STATIC_LIBS OFF)
  endif()
endif()

find_package(Boost
    1.59.0
    REQUIRED
    COMPONENTS
      ${JAMSAT_REQUIRED_BOOST_LIBRARIES}
)

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

if(Boost_LIBRARIES)
  nm_add_thirdparty_libs(LIBS ${Boost_LIBRARIES})
endif()

if(UNIX AND Boost_USE_MULTITHREADED)
  nm_add_thirdparty_libs(LIBS pthread)
endif()


# See comment in deps/CMakeLists.txt about the usage
# of include_directories() here.

# state_ptr is a header-only library:
include_directories(SYSTEM deps/state_ptr/include)

if (JAMSAT_USE_SYSTEM_GSL)
  find_package(Microsoft.GSL 3.1.0 CONFIG REQUIRED)
  nm_add_thirdparty_libs(LIBS Microsoft.GSL::GSL)
else()
  include_directories(SYSTEM deps/GSL/include)
endif()
