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

#include <jamsat/Parser.h>
#include <libjamsat/api/ipasir/JamSatIpasir.h>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

namespace jamsat {

namespace {
void printVersion() noexcept {
    std::cout << ipasir_signature() << "\n";
}

void printUsage() noexcept {
    std::cerr << "Usage: jamsat <FILE>\n"
              << "  Solves the SATISFIABILITY problem instance given in <FILE>.\n"
              << "  <FILE> is required to be formatted as described in Sec. 2.1 of\n"
              << "  http://www.cs.ubc.ca/~hoos/SATLIB/Benchmarks/SAT/satformat.ps\n"
              << "  If <FILE> is -, the problem is read from the standard input.\n";
}

void printErrorMessage(std::string const &message) noexcept {
    std::cerr << "Error: " << message << "\n";
}

int solve(void *solver) {
    int result = ipasir_solve(solver);
    assert(result == 0 || result == 10 || result == 20);
    if (result == 0) {
        std::cout << "INDETERMINATE\n";
    } else if (result == 10) {
        std::cout << "SATISFIABLE\n";
    } else if (result == 20) {
        std::cout << "UNSATISFIABLE\n";
    }
    return result;
}

class IpasirRAII {
public:
    IpasirRAII() { m_solver = ipasir_init(); }

    ~IpasirRAII() { ipasir_release(m_solver); }

    void *getSolver() noexcept { return m_solver; }

private:
    void *m_solver;
};
}

int jamsatMain(int argc, char **argv) noexcept {
    if (argc != 2) {
        printUsage();
        return EXIT_FAILURE;
    }

    try {
        IpasirRAII wrappedSolver;
        readProblem(wrappedSolver.getSolver(), argv[argc - 1]);
        return solve(wrappedSolver.getSolver());
    } catch (std::exception &e) {
        printErrorMessage(e.what());
        return EXIT_FAILURE;
    }
}
}
