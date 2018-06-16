/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)

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

#include <boost/range.hpp>
#include <vector>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/cnfproblem/CNFProblem.h>

namespace jamsat {
/**
 * \ingroup JamSAT_TestInfrastructure
 *
 * \brief Encodes an AND gate as a set of clauses.
 *
 * Only the variables occuring in \p inputs and \p output are used for encoding the gate.
 *
 * \param[in] inputs        The AND gate's input literals.
 * \param[in] output        The AND gate's output literal.
 * \param[in,out] target    The CNFProblem instance into which the clauses encoding the gates are
 *                          inserted.
 */
void insertAND(const std::vector<CNFLit> inputs, CNFLit output, CNFProblem& target);

/**
 * \ingroup JamSAT_TestInfrastructure
 *
 * \brief Encodes an OR gate as a set of clauses.
 *
 * Only the variables occuring in \p inputs and \p output are used for encoding the gate.
 *
 * \param[in] inputs        The OR gate's input literals.
 * \param[in] output        The OR gate's output literal.
 * \param[in,out] target    The CNFProblem instance into which the clauses encoding the gates are
 *                          inserted.
 */
void insertOR(const std::vector<CNFLit> inputs, CNFLit output, CNFProblem& target);

/**
 * \ingroup JamSAT_TestInfrastructure
 *
 * \brief Encodes an XOR gate as a set of clauses.
 *
 * Only the variables occuring in \p inputs and \p output are used for encoding the gate.
 *
 * \param[in] inputs        The OR gate's input literals.
 * \param[in] output        The OR gate's output literal.
 * \param[in,out] target    The CNFProblem instance into which the clauses encoding the gates are
 *                          inserted.
 */
void insertXOR(const std::vector<CNFLit> inputs, CNFLit output, CNFProblem& target);
}
