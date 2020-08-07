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

#include "IpasirSolver.h"
#include "Options.h"
#include "Parser.h"
#include "Timeout.h"

#include <cassert>
#include <cstdio>
#include <ostream>
#include <stdexcept>
#include <string>

namespace jamsat {

namespace {
void printVersion(std::ostream& stream) noexcept {
    stream << ipasir_signature() << "\n";
}

void printErrorMessage(std::string const& message, std::ostream& errStream) noexcept {
    errStream << "Error: " << message << "\n";
}

auto solve(IpasirSolver& solver, std::ostream& outStream) noexcept -> int {
    IpasirSolver::Result result = solver.solve({});
    switch (result) {
    case IpasirSolver::Result::SATISFIABLE:
        outStream << "SATISFIABLE\n";
        return 10;
    case IpasirSolver::Result::UNSATISFIABLE:
        outStream << "UNSATISFIABLE\n";
        return 20;
    default: // also handles case IpasirSolver::Result::INDETERMINATE
        outStream << "INDETERMINATE\n";
        return 0;
    }
}
}

auto jamsatMain(int argc, char** argv, std::ostream& outStream, std::ostream& errStream) noexcept
    -> int {
    JamSATOptions options;
    try {
        options = parseOptions(argc, argv);
    } catch (std::invalid_argument&) {
        return EXIT_FAILURE;
    }

    if (options.m_quit) {
        return EXIT_SUCCESS;
    }

    if (options.m_printVersion) {
        printVersion(outStream);
        return EXIT_SUCCESS;
    }

    if (options.m_waitForUserInput) {
        outStream << "Press any key to start the solver.\n";
        std::getc(stdin);
    }

    try {
        std::unique_ptr<IpasirSolver> wrappedSolver = createIpasirSolver();
        if (options.m_timeout) {
            configureTimeout(*wrappedSolver, options.m_timeout.get());
        }

        if (options.m_verbose) {
            wrappedSolver->enableLogging(outStream);
        }

        std::ostream* logStream = options.m_verbose ? &outStream : nullptr;
        readProblem(*wrappedSolver, options.m_problemFilename, logStream);
        return solve(*wrappedSolver, outStream);
    } catch (std::exception& e) {
        printErrorMessage(e.what(), errStream);
        return EXIT_FAILURE;
    }
}
}
