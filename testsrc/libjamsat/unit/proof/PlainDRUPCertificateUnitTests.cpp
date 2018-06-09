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

#include <sstream>
#include <vector>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/proof/DRUPCertificate.h>

namespace jamsat {
TEST(UnitProof, PlainDRUPCertificate_trivialProofContainsEmptyClause) {
    std::stringstream buffer;
    auto underTest = createPlainDRUPCertificate(buffer);
    underTest->closeProof();

    std::string proof = buffer.str();
    std::string expected = "0\n";
    ASSERT_EQ(proof, expected);
}

TEST(UnitProof, PlainDRUPCertificate_singleRUPClauseWrittenToProof) {
    std::stringstream buffer;
    auto underTest = createPlainDRUPCertificate(buffer);
    std::vector<CNFLit> clause{
        0_Lit,
        ~4_Lit,
        ~2_Lit,
    };
    underTest->addRUPClause(clause);

    std::string proof = buffer.str();
    std::string expected = " 1 -5 -3 0\n";
    ASSERT_EQ(proof, expected);
}

TEST(UnitProof, PlainDRUPCertificate_multipleRUPClausesWrittenToProof) {
    std::stringstream buffer;
    auto underTest = createPlainDRUPCertificate(buffer);
    std::vector<CNFLit> clause1{
        0_Lit,
        ~4_Lit,
        ~2_Lit,
    };
    underTest->addRUPClause(clause1);

    std::vector<CNFLit> clause2{
        1_Lit,
        ~2_Lit,
        ~3_Lit,
    };
    underTest->addRUPClause(clause2);

    std::string proof = buffer.str();
    std::string expected = " 1 -5 -3 0\n 2 -3 -4 0\n";
    ASSERT_EQ(proof, expected);
}

TEST(UnitProof, PlainDRUPCertificate_deletedClauseWrittenToProof) {
    std::stringstream buffer;
    auto underTest = createPlainDRUPCertificate(buffer);
    std::vector<CNFLit> clause{
        0_Lit,
        ~4_Lit,
        ~2_Lit,
    };
    underTest->deleteClause(clause);

    std::string proof = buffer.str();
    std::string expected = "d  1 -5 -3 0\n";
    ASSERT_EQ(proof, expected);
}
}
