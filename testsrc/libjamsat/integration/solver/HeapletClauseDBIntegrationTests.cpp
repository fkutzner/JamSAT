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

#include <gtest/gtest.h>

#include <vector>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/clausedb/HeapletClauseDB.h>
#include <libjamsat/solver/Propagation.h>
#include <libjamsat/solver/Trail.h>

namespace jamsat {

namespace {
Clause *allocateClause(HeapletClauseDB<Clause> &db, const std::vector<CNFLit> &literals) {
    auto &result = db.allocate(literals.size());
    std::copy(literals.begin(), literals.end(), result.begin());
    return &result;
}
}

TEST(IntegrationSolver, HeapletClauseDB_retainWatchedClauses) {
    HeapletClauseDB<Clause> clauseDB{1048576ull, 10485760ull};
    std::vector<Clause *> clauses = {
        allocateClause(clauseDB,
                       {CNFLit{CNFVar{3}, CNFSign::POSITIVE}, CNFLit{CNFVar{4}, CNFSign::POSITIVE},
                        CNFLit{CNFVar{5}, CNFSign::POSITIVE}}),
        allocateClause(clauseDB,
                       {CNFLit{CNFVar{6}, CNFSign::POSITIVE}, CNFLit{CNFVar{7}, CNFSign::POSITIVE},
                        CNFLit{CNFVar{8}, CNFSign::POSITIVE}}),
        allocateClause(clauseDB,
                       {CNFLit{CNFVar{9}, CNFSign::POSITIVE}, CNFLit{CNFVar{10}, CNFSign::POSITIVE},
                        CNFLit{CNFVar{11}, CNFSign::POSITIVE}})};

    Trail<Clause> trail{CNFVar{100}};
    Propagation<Trail<Clause>, Clause> propagation{CNFVar{100}, trail};

    propagation.registerClause(*clauses[0]);
    propagation.registerClause(*clauses[1]);
    propagation.registerClause(*clauses[2]);

    std::vector<Clause *> relocated;
    using BackInserterType = decltype(std::back_inserter(relocated));

    ASSERT_NO_THROW(
        clauseDB.retain(propagation.getClausesInPropagationOrder(),
                        [](const Clause &c) {
                            (void)c;
                            return false;
                        },
                        {}, boost::optional<BackInserterType>{std::back_inserter(relocated)}));
}
}
