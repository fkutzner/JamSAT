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

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/cnfproblem/CNFProblem.h>

#include <cstdint>
#include <string>
#include <vector>

namespace jamsat {
/**
 * \defgroup JamSAT_TestInfrastructure  JamSAT testing infrastructure
 */

/**
 * \ingroup JamSAT_TestInfrastructure
 *
 * \brief CNF Encoder for determining whether a given state can be reached in a Rule 110 automaton
 *        within N steps, starting from an under-specified start state.
 *
 * Problem: Given partially specified states B and E of a Rule 110 automaton with exactly N
 *          consecutive cells, can state E be reached from state B with exactly N intermediate
 *          states in a simulation of Rule 110?
 *
 * Rule 110 is an interesting elementary cellular automaton with the following transition function:
 *   current pattern:            111    110    101    100    011    010    001    000
 *   new state for center cell:   0      1      1      0      1      1      1      0
 * For more information about Rule 110, see e.g.: https://en.wikipedia.org/wiki/Rule_110
 *
 * This class serves to encode the problem states above as a SAT problem in CNF. Rule 110 states are
 * given as strings consisting only of the character '1', '0' and 'x' (with 'x' denoting don't-care
 * values). Both the left neightbour of the leftmost cell and the right neighbour of the rightmost
 * cell constantly have the value '0'.
 *
 * Example: The state E = "1xxx0" denotes a state in a 5-cell Rule 110 automaton. The leftmost cell
 * is set to '1', the rightmost cell is set to '0', and all other cell values are left unspecified.
 * Can E be reached from B = "0xx10" with exactly 1 intermediate state? The answer is: yes, with
 * B = "00110" and E = "11000".
 */
class Rule110PredecessorStateProblem {
public:
    struct Rule110Encoding {
        CNFProblem cnfProblem;
        std::vector<CNFLit> freeInputs;
    };

    /**
     * \brief Constructs a Rule110PredecessorStateProblem.
     *
     * \param targetStateSpec   The specification of the source state.
     * \param sourceStateSpec   The specification of the target state. The length of
     *                          \p sourceStateSpec must be equal to the length of
     *                          \p targetStateSpec.
     * \param numberOfIntermediateSteps The number of intermediate Rule 110 steps.
     */
    Rule110PredecessorStateProblem(const std::string &targetStateSpec,
                                   const std::string &sourceStateSpec,
                                   uint32_t numberOfIntermediateSteps);

    /**
     * \brief Encodes the problem instance as a satisfiability problem instance in CNF.
     *
     * \returns a structure containing a CNF SAT problem instance which is satisfiable
     *   iff the target state is reachable from the source state with exactly N
     *   intermediate steps. The structure also contains a vector \p freeInputs constructed
     *   as follows:
     *   For each "x" in \p sourceStateSpec passed to the constructor
     *   of this problem generator, a literal L is added to \p freeInputs
     *   that models the value of the "x" cell: x is set to 1 iff L is
     *   set to "true". The literals added to \p freeInputs are distinct
     *   and model "x" cells in their order of appearance in \p sourceStateSpec.
     */
    auto getCNFEncoding() const -> Rule110Encoding;

private:
    std::string m_targetStateSpec;
    std::string m_sourceStateSpec;
    uint32_t m_numberOfIntermediateSteps;
    uint32_t m_automatonWidth;

    CNFVar getCellVariable(uint32_t step, uint32_t cellIndex) const noexcept;
    std::vector<CNFClause> createConstraints(uint32_t step, uint32_t cellIndex,
                                             std::vector<CNFLit> &freeInputs) const;
};
}
