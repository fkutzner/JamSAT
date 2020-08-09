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

#include "Options.h"

#include <chrono>
#include <stdexcept>
#include <string>

#include <CLI/CLI.hpp>

namespace jamsat {
auto parseOptions(int argc, char const* const* argv) -> JamSATOptions
{
  CLI::App app{"JamSAT, a SATISFIABILITY problem solver"};

  JamSATOptions result;
  int timeout = 0;
  app.add_flag("-v,--version", result.m_printVersion, "Print the version of JamSAT and exit");
  app.add_flag(
      "-V,--verbose", result.m_verbose, "Periodically print stats during the solving process");
  app.add_option("-t,--timeout", timeout, "Solver timeout in seconds (0 <= N < 2^32)");
  app.add_flag(
      "-w,--wait",
      result.m_waitForUserInput,
      "Wait for the user to press a key before starting the solver (useful for profiling)");
  app.add_option(
         "FILE", result.m_problemFilename, "A file containing a CNF-encoded SAT problem instance")
      ->required();
  app.positionals_at_end();

  try {
    app.parse(argc, argv);
  }
  catch (CLI::CallForHelp const&) {
    std::cout << app.help() << "\n";
    result.m_quit = true;
  }
  catch (CLI::Error const& error) {
    std::cout << error.what() << "\nRe-run with --help to list valid arguments.\n";
    throw std::invalid_argument{"bad program options"};
  }

  if (timeout > 0) {
    result.m_timeout = std::chrono::seconds{timeout};
  }
  return result;
}
}
