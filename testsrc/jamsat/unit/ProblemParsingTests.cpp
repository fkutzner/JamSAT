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

#include <gtest/gtest.h>

#include <jamsat/Parser.h>
#include <jamsat/ipasirmock/IpasirMock.h>
#include <libjamsat/api/ipasir/JamSatIpasir.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

// TODO: test the parser more thoroughly, esp. chunking

namespace jamsat {

namespace {
bool fileExists(std::string const &file) {
    std::ifstream stream{file};
    return static_cast<bool>(stream);
}

class TestIpasirRAII {
public:
    TestIpasirRAII() { m_solver = ipasir_init(); }

    ~TestIpasirRAII() { ipasir_release(m_solver); }

    void *getSolver() noexcept { return m_solver; }

private:
    void *m_solver;
};
}

TEST(UnitFrontendParsing, ParsingTestIsExecutedInCorrectDirectory) {
    ASSERT_TRUE(fileExists("BadLiteral.cnf"))
        << "Test input data could not be found. Is the test executed "
        << "in the correct directory, i.e. the JamSAT directory containing "
        << "BadLiteral.cnf?";
}

TEST(UnitFrontendParsing, ParsingTestIsLinkedToMockIPASIR) {
    ASSERT_TRUE(ipasir_signature() == IPASIRTestMockSignature);
}

TEST(UnitFrontendParsing, FileContainingBadLiteralIsRejected) {
    TestIpasirRAII mockSolver;
    EXPECT_THROW(jamsat::readProblem(mockSolver.getSolver(), "BadLiteral.cnf"), std::runtime_error);
}

TEST(UnitFrontendParsing, FileContainingTooFewClausesIsRejected) {
    TestIpasirRAII mockSolver;
    EXPECT_THROW(jamsat::readProblem(mockSolver.getSolver(), "TooFewClauses.cnf"),
                 std::runtime_error);
}

TEST(UnitFrontendParsing, FileContainingTooManyClausesIsRejected) {
    TestIpasirRAII mockSolver;
    EXPECT_THROW(jamsat::readProblem(mockSolver.getSolver(), "TooManyClauses.cnf"),
                 std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithMissingHeaderIsRejected) {
    TestIpasirRAII mockSolver;
    EXPECT_THROW(jamsat::readProblem(mockSolver.getSolver(), "MissingHeader.cnf"),
                 std::runtime_error);
}

TEST(UnitFrontendParsing, ValidFileIsParsedCorrectly) {
    TestIpasirRAII mockSolver;
    jamsat::readProblem(mockSolver.getSolver(), "SmallValidProblem.cnf");
    std::vector<int> expected = {1, 2, 3, 0, 3, 4, 0, 1, 0};
    EXPECT_EQ(getIPASIRMockContext(mockSolver.getSolver())->m_literals, expected);
}

TEST(UnitFrontendParsing, ValidCompressedFileIsParsedCorrectly) {
    TestIpasirRAII mockSolver;
    jamsat::readProblem(mockSolver.getSolver(), "CompressedSmallValidProblem.cnf.gz");
    std::vector<int> expected = {1, 2, 3, 0, 3, 4, 0, 1, 0};
    EXPECT_EQ(getIPASIRMockContext(mockSolver.getSolver())->m_literals, expected);
}
}
