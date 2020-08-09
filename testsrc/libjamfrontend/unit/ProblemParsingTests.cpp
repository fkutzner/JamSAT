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

#include <libjamfrontend/IpasirSolver.h>
#include <libjamfrontend/Parser.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// TODO: test the parser more thoroughly, esp. chunking

namespace jamsat {

namespace {
auto fileExists(std::string const& file) -> bool
{
  std::ifstream stream{file};
  return static_cast<bool>(stream);
}

/**
 * \brief An IpasirSolver implementation recording added clauses
 */
class ClauseRecordingIpasirSolver : public IpasirSolver {
public:
  ClauseRecordingIpasirSolver() {}

  virtual ~ClauseRecordingIpasirSolver() {}

  void addClause(std::vector<int> const& literals) noexcept override
  {
    m_addedClauses.push_back(literals);
  }

  auto getClauses() const noexcept -> std::vector<std::vector<int>> const&
  {
    return m_addedClauses;
  }

  auto solve(std::vector<int> const&) noexcept -> Result override { return Result::INDETERMINATE; }

  auto getValue(int) noexcept -> Value override { return Value::DONTCARE; }

  auto isFailed(int) noexcept -> bool override { return false; }

  void setTerminateFn(void*, int (*)(void* state)) noexcept override {}

  void setLearnFn(void*, int, void (*)(void* state, int* clause)) noexcept override {}

  void enableLogging(std::ostream&) noexcept override {}

private:
  std::vector<std::vector<int>> m_addedClauses;
};
}

TEST(UnitFrontendParsing, ParsingTestIsExecutedInCorrectDirectory)
{
  ASSERT_TRUE(fileExists("BadLiteral.cnf"))
      << "Test input data could not be found. Is the test executed "
      << "in the correct directory, i.e. the JamSAT directory containing "
      << "BadLiteral.cnf?";
}

TEST(UnitFrontendParsing, FileContainingBadLiteralIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("BadLiteral.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "BadLiteral.cnf", &std::cout), std::runtime_error);
}

TEST(UnitFrontendParsing, FileContainingTooFewClausesIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("TooFewClauses.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "TooFewClauses.cnf", &std::cout), std::runtime_error);
}

TEST(UnitFrontendParsing, FileContainingTooManyClausesIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("TooManyClauses.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "TooManyClauses.cnf", &std::cout), std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithMissingHeaderIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("MissingHeader.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "MissingHeader.cnf", &std::cout), std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithInvalidStringInHeaderIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("InvalidStringInHeader.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "InvalidStringInHeader.cnf", &std::cout),
               std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithLiteralOutOfRangeNegIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("LiteralOutOfRangeNeg.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "LiteralOutOfRangeNeg.cnf", &std::cout),
               std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithLiteralOutOfRangePosIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("LiteralOutOfRangePos.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "LiteralOutOfRangePos.cnf", &std::cout),
               std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithMissingClauseCountIsRejected)
{
  ClauseRecordingIpasirSolver recorder;
  ASSERT_TRUE(fileExists("MissingClauseCountInHeader.cnf"));
  EXPECT_THROW(jamsat::readProblem(recorder, "MissingClauseCountInHeader.cnf", &std::cout),
               std::runtime_error);
}

TEST(UnitFrontendParsing, FileWithMissingCountsInHeaderIsRejected)
{
  ASSERT_TRUE(fileExists("MissingCountsInHeader.cnf"));
  ClauseRecordingIpasirSolver recorder;
  EXPECT_THROW(jamsat::readProblem(recorder, "MissingCountsInHeader.cnf", &std::cout),
               std::runtime_error);
}

TEST(UnitFrontendParsing, ValidFileIsParsedCorrectly)
{
  ClauseRecordingIpasirSolver recorder;
  jamsat::readProblem(recorder, "SmallValidProblem.cnf", &std::cout);
  std::vector<std::vector<int>> expected = {{1, 2, 3}, {3, 4}, {1}};
  EXPECT_EQ(recorder.getClauses(), expected);
}

TEST(UnitFrontendParsing, ValidCompressedFileIsParsedCorrectly)
{
  ClauseRecordingIpasirSolver recorder;
  jamsat::readProblem(recorder, "CompressedSmallValidProblem.cnf.gz", &std::cout);
  std::vector<std::vector<int>> expected = {{1, 2, 3}, {3, 4}, {1}};
  EXPECT_EQ(recorder.getClauses(), expected);
}

TEST(UnitFrontendParsing, ValidHugeFileIsParsedCorrectly)
{
  ClauseRecordingIpasirSolver recorder;
  jamsat::readProblem(recorder, "LargeProblem.cnf.gz", &std::cout);

  std::vector<int> clausesFlat;
  for (std::vector<int> const& clause : recorder.getClauses()) {
    std::copy(clause.begin(), clause.end(), std::back_inserter(clausesFlat));
    clausesFlat.push_back(0);
  }

  // Compare via precomputed hash value:
  int hash = 0;
  int clauseCount = 0;
  for (auto lit : clausesFlat) {
    if (lit == 0) {
      ++clauseCount;
      hash += 27;
    }
    else {
      if (lit < 0) {
        lit = (-lit) << 12;
      }
      hash ^= lit;
    }
  }
  hash ^= clauseCount;

  EXPECT_EQ(hash, 3624315);
}
}
