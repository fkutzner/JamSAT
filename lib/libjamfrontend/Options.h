/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace jamsat {

struct JamSATOptions {
  std::string m_problemFilename;
  bool m_printVersion = false;
  bool m_waitForUserInput = false;
  std::optional<std::chrono::seconds> m_timeout;
  std::vector<std::string> m_backendOptions;
  bool m_verbose = false;
  bool m_quit = false;
};

/**
 * \brief Parses the JamSAT command-line options.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param argc      The number of arguments to be parsed.
 * \param argv      An array of arguments to be parsed, with the first
 *                  argument (ie. JamSAT's binary name) getting ignored.
 * \return A representation of the options given via \p argc and \p argv.
 *
 * \throws std::invalid_argument    argv represents invalid JamSAT arguments.
 */
auto parseOptions(int argc, char const* const* argv) -> JamSATOptions;
}
