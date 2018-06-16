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
#include <limits>
#include <sstream>
#include <string>

#if defined(JAMSAT_ENABLE_LOGGING)
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#endif

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/cnfproblem/CNFProblem.h>

namespace jamsat {
namespace {
class SuppressLoggedWarningsRAII {
public:
    SuppressLoggedWarningsRAII() {
#if defined(JAMSAT_ENABLE_LOGGING)
        auto filter = boost::log::trivial::severity > boost::log::trivial::warning;
        boost::log::core::get()->set_filter(filter);
#endif
    }

    ~SuppressLoggedWarningsRAII() {
#if defined(JAMSAT_ENABLE_LOGGING)
        boost::log::core::get()->reset_filter();
#endif
    }
};
}

TEST(UnitCNFProblem, emptyCNFProblemHasSize0) {
    CNFProblem underTest;
    ASSERT_EQ(underTest.getSize(), 0ull);
}

TEST(UnitCNFProblem, emptyCNFProblemIsMarkedEmpty) {
    CNFProblem underTest;
    ASSERT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, emptyCNFProblemMaxVarIsUndefined) {
    CNFProblem underTest;
    ASSERT_EQ(underTest.getMaxVar(), CNFVar::getUndefinedVariable());
}

TEST(UnitCNFProblem, emptyCNFProblemHasNoClauses) {
    CNFProblem underTest;
    ASSERT_TRUE(underTest.getClauses().empty());
}

TEST(UnitCNFProblem, addedClauseCanBeRetrieved) {
    std::vector<CNFLit> clause = {~3_Lit, ~3_Lit};

    CNFProblem underTest;
    underTest.addClause(clause);
    ASSERT_EQ(underTest.getSize(), 1ull);
    ASSERT_EQ(underTest.getClauses()[0], clause);
}

TEST(UnitCNFProblem, cnfProblemWithTwoClausesReportsSize) {
    std::vector<CNFLit> clause1 = {~3_Lit, ~4_Lit};

    std::vector<CNFLit> clause2 = {~5_Lit, 6_Lit};

    CNFProblem underTest;
    underTest.addClause(clause1);
    underTest.addClause(clause2);
    ASSERT_EQ(underTest.getSize(), 2ull);
    ASSERT_FALSE(underTest.isEmpty());
}

TEST(UnitCNFProblem, cnfProblemOrderIsPreserved) {
    std::vector<CNFLit> clause1 = {~3_Lit, ~4_Lit};

    std::vector<CNFLit> clause2 = {~5_Lit, 6_Lit};

    CNFProblem underTest;
    underTest.addClause(clause1);
    underTest.addClause(clause2);
    ASSERT_EQ(underTest.getSize(), 2ull);
    EXPECT_EQ(underTest.getClauses()[0], clause1);
    EXPECT_EQ(underTest.getClauses()[1], clause2);
}

TEST(UnitCNFProblem, cnfProblemReportsMaximumVariable) {
    std::vector<CNFLit> clause1 = {~3_Lit, ~4_Lit};

    std::vector<CNFLit> clause2 = {~5_Lit, 6_Lit};

    CNFProblem underTest;
    underTest.addClause(clause1);
    underTest.addClause(clause2);
    EXPECT_EQ(underTest.getMaxVar(), CNFVar{6});
}

TEST(UnitCNFProblem, printEmptyClauseAsDIMACS) {
    std::stringstream collector;
    CNFClause underTest;
    collector << underTest;

    ASSERT_TRUE(collector);
    std::string result;
    std::getline(collector, result);
    EXPECT_EQ(result, "0");

    collector.get();
    EXPECT_FALSE(collector);
}

TEST(UnitCNFProblem, printBinaryClauseAsDIMACS) {
    std::stringstream collector;
    CNFClause underTest{~1_Lit, 3_Lit};

    collector << underTest;

    ASSERT_TRUE(collector);
    std::string result;
    std::getline(collector, result);
    EXPECT_EQ(result, "-2  4 0");

    collector.get();
    EXPECT_FALSE(collector);
}

TEST(UnitCNFProblem, printEmptyCNFProblemAsDIMACS) {
    std::stringstream collector;
    CNFProblem underTest;
    collector << underTest;

    ASSERT_TRUE(collector);
    std::string result;
    std::getline(collector, result);
    EXPECT_EQ(result, "p cnf 0 0");

    collector.get();
    EXPECT_FALSE(collector);
}

TEST(UnitCNFProblem, printTwoClauseCNFProblemAsDIMACS) {
    std::vector<CNFLit> clause1 = {~3_Lit, ~4_Lit};

    std::vector<CNFLit> clause2 = {~5_Lit, 6_Lit};
    CNFProblem underTest;

    underTest.addClause(clause1);
    underTest.addClause(clause2);

    std::stringstream collector;
    collector << underTest;

    ASSERT_TRUE(collector);
    std::string currentLine;
    std::getline(collector, currentLine);
    EXPECT_EQ(currentLine, "p cnf 7 2");

    ASSERT_TRUE(collector);
    std::getline(collector, currentLine);
    EXPECT_EQ(currentLine, "-4 -5 0");

    ASSERT_TRUE(collector);
    std::getline(collector, currentLine);
    EXPECT_EQ(currentLine, "-6  7 0");

    collector.get();
    EXPECT_FALSE(collector);
}

TEST(UnitCNFProblem, parseEmptyDIMACSClause) {
    std::stringstream conduit;
    conduit << "0";

    CNFClause underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    EXPECT_TRUE(underTest.empty());
}

TEST(UnitCNFProblem, parseSinglePositiveLiteralDIMACSClause) {
    std::stringstream conduit;
    conduit << "1 0";

    CNFClause underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.size(), 1ull);
    CNFLit expectedLiteral{CNFVar{0}, CNFSign::POSITIVE};
    EXPECT_EQ(underTest[0], expectedLiteral);
}

TEST(UnitCNFProblem, parseSingleNegativeLiteralDIMACSClause) {
    std::stringstream conduit;
    conduit << "-2 0";

    CNFClause underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.size(), 1ull);
    CNFLit expectedLiteral{CNFVar{1}, CNFSign::NEGATIVE};
    EXPECT_EQ(underTest[0], expectedLiteral);
}

TEST(UnitCNFProblem, parseSimpleFormattedCNFClause) {
    std::stringstream conduit;
    conduit << "-2 4 1 0";

    CNFClause underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.size(), 3ull);

    CNFClause expected = {
        ~1_Lit,
        3_Lit,
        0_Lit,
    };

    EXPECT_EQ(underTest, expected);
}

TEST(UnitCNFProblem, inputStreamPointsJustBeyondClauseAfterParsing) {
    std::stringstream conduit;
    conduit << "-2 4 1 0 ok";

    CNFClause dummy;
    conduit >> dummy;
    ASSERT_FALSE(conduit.fail());

    std::string dataBeyondClause;
    conduit >> dataBeyondClause;

    ASSERT_FALSE(conduit.fail());
    EXPECT_EQ(dataBeyondClause, "ok");
}

TEST(UnitCNFProblem, parseMultilineCNFClause) {
    std::stringstream conduit;
    conduit << "-2 4" << std::endl << "1 0";

    CNFClause underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.size(), 3ull);

    CNFClause expected = {
        ~1_Lit,
        3_Lit,
        0_Lit,
    };

    EXPECT_EQ(underTest, expected);
}

TEST(UnitCNFProblem, parseCommentContainingCNFClause) {
    std::stringstream conduit;
    conduit << "-2 4 c this is a comment" << std::endl << "1 0";

    CNFClause underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.size(), 3ull);

    CNFClause expected = {
        ~1_Lit,
        3_Lit,
        0_Lit,
    };

    EXPECT_EQ(underTest, expected);
}

TEST(UnitCNFProblem, parseGarbageContainingCNFClauseFails) {
    SuppressLoggedWarningsRAII suppressWarnings;

    std::stringstream conduit;
    conduit << "-2 4 this is garbage" << std::endl << "1 0";

    CNFClause underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.empty());
}

TEST(UnitCNFProblem, parseUnterminatedCNFClauseFails) {
    SuppressLoggedWarningsRAII suppressWarnings;

    std::stringstream conduit;
    conduit << "-2 4";

    CNFClause underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.empty());
}

TEST(UnitCNFProblem, parseEmptyDIMACSProblemInputFails) {
    SuppressLoggedWarningsRAII suppressWarnings;

    std::string testData = " ";
    std::stringstream conduit{testData};
    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseCommentOnlyDIMACSProblemFails) {
    SuppressLoggedWarningsRAII suppressWarnings;

    std::stringstream conduit;
    conduit << "c Foo" << std::endl;
    conduit << "c" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseEmptyDIMACSProblem) {
    std::stringstream conduit;
    conduit << "p cnf 0 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseSingleClauseDIMACSProblem) {
    std::stringstream conduit;
    conduit << "p cnf 5 1" << std::endl;
    conduit << "1 2 -3 4 -5 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.getSize(), 1ull);

    CNFClause expected = {
        0_Lit,
        1_Lit,
        ~2_Lit,
        3_Lit,
        ~4_Lit,
    };

    EXPECT_EQ(underTest.getClauses()[0], expected);
    EXPECT_EQ(underTest.getMaxVar(), CNFVar{4});
}

TEST(UnitCNFProblem, parseMultipleClauseDIMACSProblem) {
    std::stringstream conduit;
    conduit << "p cnf 6 2" << std::endl;
    conduit << "1 2 0" << std::endl;
    conduit << "5 6 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.getSize(), 2ull);

    CNFClause expected1 = {
        0_Lit,
        1_Lit,
    };

    CNFClause expected2 = {
        4_Lit,
        5_Lit,
    };

    EXPECT_EQ(underTest.getClauses()[0], expected1);
    EXPECT_EQ(underTest.getClauses()[1], expected2);
    EXPECT_EQ(underTest.getMaxVar(), CNFVar{5});
}

TEST(UnitCNFProblem, parseDIMACSProblemRecognizingMaxVarInFirstClause) {
    std::stringstream conduit;
    conduit << "p cnf 6 2" << std::endl;
    conduit << "6 2 0" << std::endl;
    conduit << "2 4 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.getSize(), 2ull);
    EXPECT_EQ(underTest.getMaxVar(), CNFVar{5});
}

TEST(UnitCNFProblem, parseDIMACSProblemWithBadClauseFails) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf 6 2" << std::endl;
    conduit << "1 2 0" << std::endl;
    conduit << "1 X 0" << std::endl;
    conduit << "5 6 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseDIMACSProblemWithCommentsAndWhitespace) {
    std::stringstream conduit;
    conduit << "c cnf 5 1" << std::endl;
    conduit << "\t p cnf 6 2 c Foobar" << std::endl;
    conduit << "1 2 0" << std::endl;
    conduit << "c Baz" << std::endl;
    conduit << "5 6 0 c Bam" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    ASSERT_FALSE(conduit.fail());
    ASSERT_EQ(underTest.getSize(), 2ull);
    ASSERT_EQ(underTest.getClauses()[0].size(), 2ull);
    ASSERT_EQ(underTest.getClauses()[1].size(), 2ull);
}

TEST(UnitCNFProblem, parseIllegalDIMACSHeaderCNFFails) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p illegal 0 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseIllegalDIMACSHeaderVarCountFails) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf illegal 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseIllegalDIMACSHeaderClauseCountCountFails) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf 0 illegal" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, parseCNFProblemWithIllegallyHighVariableFails) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf 6 2" << std::endl;
    conduit << "1 2 0" << std::endl;
    conduit << "1 9 0" << std::endl;
    conduit << "5 7 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, rejectsCNFProblemWithUnstorableVariableCount) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf " << std::numeric_limits<CNFVar::RawVariable>::max() << "0";
    conduit << " 1 " << std::endl << "1 2 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, rejectsCNFProblemWithReservedVariable) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf " << std::numeric_limits<CNFVar::RawVariable>::max();
    conduit << " 1 " << std::endl << "1 4 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, rejectsCNFProblemWithMinimalReservedVariable) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf " << (CNFVar::getMaxRawValue() + 1);
    conduit << " 1 " << std::endl << "1 4 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}

TEST(UnitCNFProblem, rejectsCNFProblemWithLargestNegativeLiteral) {
    SuppressLoggedWarningsRAII suppressWarnings;
    std::stringstream conduit;
    conduit << "p cnf " << (CNFVar::getMaxRawValue()) << " 1 " << std::endl;
    conduit << "1 " << std::numeric_limits<int>::min();
    conduit << " 4 0" << std::endl;

    CNFProblem underTest;
    conduit >> underTest;
    EXPECT_TRUE(conduit.fail());
    EXPECT_TRUE(underTest.isEmpty());
}
}
