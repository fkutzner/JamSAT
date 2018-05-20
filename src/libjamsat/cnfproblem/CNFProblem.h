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

#pragma once

#include "libjamsat/cnfproblem/CNFLiteral.h"
#include <vector>

namespace jamsat {
using CNFClause = std::vector<CNFLit>;

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \class jamsat::CNFProblem
 *
 * \brief A SATISFIABILITY problem instance representation (CNF encoded).
 */
class CNFProblem {
public:
    using size_type = std::vector<CNFClause>::size_type;

    /**
     * \brief Constructs an empty CNFProblem object.
     */
    CNFProblem();

    /**
     * \brief Adds the given clause to the problem instance.
     *
     * \param clause    The clause to be added.
     */
    void addClause(const CNFClause &clause) noexcept;

    /**
     * \brief Adds the given clause to the problem instance, transferring
     * ownership.
     *
     * \param clause    The clause to be added.
     */
    void addClause(CNFClause &&clause) noexcept;

    /**
     * \brief Gets the problem instance's clauses.
     *
     * \returns A vector of clauses.
     */
    const std::vector<CNFClause> &getClauses() const noexcept;

    /**
     * \brief Gets the number of clauses contained in the problem instance.
     *
     * \returns The number of clauses contained in the problem instance.
     */
    size_type getSize() const noexcept;

    /**
     * \brief Determines whether the problem instance is empty.
     *
     * \returns true iff the problem instance does not contain clauses.
     */
    bool isEmpty() const noexcept;

    /**
     * \brief Gets the largest variable occurring in the problem instance.
     *
     * \returns the largest variable occurring in the problem instance if the
     * instance is not empty; CNFVar::getUndefinedVariable() otherwise.
     */
    CNFVar getMaxVar() const noexcept;

    /**
     * \brief Removes all clauses.
     */
    void clear() noexcept;

private:
    std::vector<CNFClause> m_clauses;
    CNFVar m_maxVar;
};

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \brief Prints a CNF-encoded problem, DIMACS-formatted.
 *
 * \param output    The target output stream.
 * \param problem   The problem instance to be printed.
 */
std::ostream &operator<<(std::ostream &output, const CNFProblem &problem);

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \brief Prints a CNF clause, DIMACS-formatted with trailing 0.
 *
 * \param output    The target output stream.
 * \param clause    The clause to be printed.
 */
std::ostream &operator<<(std::ostream &output, const CNFClause &clause);

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \brief Reads a DIMACS-formatted CNF problem instance from the given stream.
 *
 * If reading the problem fails, \p problem is empty when this method returns,
 * and the fail bit of \p input is set.
 *
 * \param input    The input stream from which the DIMACS problem should be
 * read.
 * \param problem   The empty problem instance to which the problem instance's
 * clauses should be added.
 */
std::istream &operator>>(std::istream &input, CNFProblem &problem);

/**
 * \ingroup JamSAT_CNFProblem
 *
 * \brief Reads a DIMACS clause from the given stream.
 *
 * If reading the clause fails, \p clause is reset to the state it had when this
 * method was invoked, and fail bit of \p input is set.
 *
 * \param input    The input stream from which the DIMACS clause should be
 * read.
 * \param clause    The clause where the literals should be appended.
 */
std::istream &operator>>(std::istream &input, CNFClause &clause);
}
