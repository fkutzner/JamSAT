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

#include "IpasirSolver.h"

#include <stdexcept>
#include <string>

namespace jamsat {
class CNFParserError : public std::runtime_error {
public:
    explicit CNFParserError(std::string const& what);
    virtual ~CNFParserError();
};

/**
 * \brief Reads a CNF problem instance from the given (possibly gz-compressed)
 * file and adds it to an IPASIR solver.
 *
 * \ingroup JamSAT_Frontend
 *
 * \param[in,out] solver            The IPASIR solver receiving the problem.
 * \param[in] location              The location of the problem instance file.
 *                                  If \p location equals "-", the problem instance
 *                                  is read from the standard input.
 * \param[in] msgStream             The stream to which logging messages shall be
 *                                  printed.
 *
 * \throws CNFParserError           An I/O or parsing error has occured while
 *                                  reading \p file.
 */
void readProblem(IpasirSolver& solver, std::string const& location, std::ostream& msgStream);
}
