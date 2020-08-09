/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

/**
 * \file utils/Logger.h
 * \brief Logging-related functions and macros
 */

#pragma once

#include <cstdint>

#if defined(JAM_ENABLE_LOGGING)
#include <boost/log/trivial.hpp>

namespace jamsat {
namespace detail_logger {
extern uint64_t currentEpoch;
extern uint64_t startLoggingEpoch;
}
}
inline void loggingEpochElapsed()
{
  ++(::jamsat::detail_logger::currentEpoch);
}
inline void setLoggingStartEpoch(uint64_t epoch)
{
  ::jamsat::detail_logger::startLoggingEpoch = epoch;
}

#define JAM_LOG(level, category, message)                                                          \
  do {                                                                                             \
    if (::jamsat::detail_logger::currentEpoch >= ::jamsat::detail_logger::startLoggingEpoch) {     \
      BOOST_LOG_TRIVIAL(level) << "[" << category << "] " << message;                              \
    }                                                                                              \
  } while (0)

#else

inline void loggingEpochElapsed()
{
  // Logging disabled, therefore no action required
}
inline void setLoggingStartEpoch(uint64_t epoch)
{
  (void)epoch;
  // Logging disabled, therefore no action required
}

#define JAM_LOG(level, category, message)

#endif
