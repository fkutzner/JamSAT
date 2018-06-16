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

#include <toolbox/testutils/Minisat.h>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <minisat/core/Solver.h>
#include <minisat/core/SolverTypes.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace jamsat {
TBool isSatisfiableViaMinisat(CNFProblem const& problem) {
    Minisat::Solver solver;
    for (unsigned int i = 0; i <= problem.getMaxVar().getRawValue(); ++i) {
        solver.newVar();
    }

    for (auto& clause : problem.getClauses()) {
        Minisat::vec<Minisat::Lit> minisatClause;
        for (auto lit : clause) {
            Minisat::Lit minisatLit =
                Minisat::mkLit(static_cast<Minisat::Var>(lit.getVariable().getRawValue()));
            minisatClause.push(lit.getSign() == jamsat::CNFSign::POSITIVE ? minisatLit
                                                                          : ~minisatLit);
        }
        solver.addClause(minisatClause);
    }

    return toTBool(solver.solve());
}
}
