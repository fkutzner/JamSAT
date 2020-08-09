/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include <libjamsat/proof/DRATCertificate.h>
#include <libjamsat/utils/Printers.h>

#include <fstream>
#include <random>
#include <utility>

namespace fs = boost::filesystem;

namespace {
class PathWithDeleter {
public:
  PathWithDeleter(fs::path const& path) : m_path{path} {}

  auto getPath() const noexcept -> fs::path const& { return m_path; }

  ~PathWithDeleter()
  {
    if (!m_path.empty()) {
      fs::remove(m_path);
    }
  }

  PathWithDeleter(PathWithDeleter&& rhs) noexcept
  {
    if (!m_path.empty()) {
      fs::remove(m_path);
    }
    m_path = rhs.m_path;
    rhs.m_path.clear();
  }

  auto operator=(PathWithDeleter&& rhs)
  {
    if (!m_path.empty()) {
      fs::remove(m_path);
    }
    m_path = rhs.m_path;
    rhs.m_path.clear();
  }


  PathWithDeleter(PathWithDeleter const&) = delete;
  auto operator=(PathWithDeleter const&) = delete;

private:
  fs::path m_path;
};

auto createTempFile() -> PathWithDeleter
{
  static std::mt19937 rng{32};
  std::uniform_int_distribution<> numbers;

  fs::path tempDir = fs::temp_directory_path();
  fs::path candidate;
  do {
    candidate = tempDir / ("jamsat-tmp-" + std::to_string(numbers(rng)));
  } while (fs::exists(candidate));

  std::ofstream create{candidate.string()};
  return PathWithDeleter{candidate};
}

auto slurpFile(fs::path const& path) -> std::vector<unsigned char>
{
  std::ifstream file(path.string());
  std::vector<unsigned char> result((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
  return result;
}
}

namespace jamsat {
TEST(UnitProof, FileDRATCertificate_WhenFileCannotBeCreated_ThenExceptionIsThrown)
{
  std::string const path = "/highly/unlikely/existing/folder/proof.drat";
  EXPECT_THROW(createFileDRATCertificate(path), FileIOError);
}


struct DRATClauseBase {
  std::vector<CNFLit> literals;
};

struct ATClause : public DRATClauseBase {
};

struct RATClause : public DRATClauseBase {
  std::size_t pivot = 0;
};

struct DeleteClause : public DRATClauseBase {
};

using ProofClause = boost::variant<ATClause, RATClause, DeleteClause>;


// clang-format off
using ProofClauseAndResult = std::tuple<
  std::vector<ProofClause>, // input clauses
  std::vector<unsigned char> // expected proof contents
>;
//clang-format on

auto operator<<(std::ostream& stream, ATClause const& toPrint) -> std::ostream& {
    stream << "AT clause: (" << toString(toPrint.literals.begin(), toPrint.literals.end()) << ")";
    return stream;
}

auto operator<<(std::ostream& stream, RATClause const& toPrint) -> std::ostream& {
    stream << "RAT clause: (" << toString(toPrint.literals.begin(), toPrint.literals.end());
    stream << ", pivot: " << toPrint.pivot << ")";
    return stream;
}

auto operator<<(std::ostream& stream, DeleteClause const& toPrint) -> std::ostream& {
    stream << "Remove clause: (" << toString(toPrint.literals.begin(), toPrint.literals.end());
    return stream;
}



class FileDRATCertificateSerializationTests : public ::testing::TestWithParam<ProofClauseAndResult> {
public:
virtual ~FileDRATCertificateSerializationTests() = default;
};

TEST_P(FileDRATCertificateSerializationTests, SerializationTest) {
    PathWithDeleter tempFile = createTempFile();
    std::unique_ptr<DRATCertificate> underTest = createFileDRATCertificate(tempFile.getPath().string());

    for(ProofClause const& clause : std::get<0>(GetParam())) {
        if (ATClause const* cl = boost::get<ATClause>(&clause); cl != nullptr) {
            underTest->addATClause(cl->literals);
        }
        else if (RATClause const* cl = boost::get<RATClause>(&clause); cl != nullptr) {
            underTest->addRATClause(cl->literals, cl->pivot);
        }
        else if (DeleteClause const* cl = boost::get<DeleteClause>(&clause); cl != nullptr) {
            underTest->deleteClause(cl->literals);
        }
    }
    underTest->flush();
    underTest.reset(nullptr);

    std::vector<unsigned char> result = slurpFile(tempFile.getPath());

    EXPECT_THAT(result, ::testing::ContainerEq(std::get<1>(GetParam())));
}

namespace {
auto createATClause(std::vector<CNFLit> const& lits) -> ATClause {
    return ATClause{{lits}};
}

auto createRATClause(std::vector<CNFLit> const& lits, std::size_t pivot) -> RATClause {
    return RATClause{{lits}, pivot};
}

auto createDeleteClause(std::vector<CNFLit> const& lits) -> DeleteClause {
    return DeleteClause{{lits}};
}

constexpr unsigned char CL_ADD = 0x61;
constexpr unsigned char CL_DEL = 0x64;

}

// clang-format off
INSTANTIATE_TEST_CASE_P(UnitProof, FileDRATCertificateSerializationTests,
  ::testing::Values(
    ProofClauseAndResult{{}, {}},
    ProofClauseAndResult{{ATClause{}}, {CL_ADD, 0x00}},
    ProofClauseAndResult{{createATClause({0_Lit})}, {CL_ADD, 0x02, 0x00}},
    ProofClauseAndResult{{createATClause({0_Lit, 1_Lit, 128_Lit})}, {CL_ADD, 0x02, 0x04, 0x82, 0x02, 0x00}},
    ProofClauseAndResult{{createRATClause({0_Lit}, 0)}, {CL_ADD, 0x02, 0x00}},
    ProofClauseAndResult{{createRATClause({0_Lit, 1_Lit, 128_Lit}, 0)}, {CL_ADD, 0x02, 0x04, 0x82, 0x02, 0x00}},
    ProofClauseAndResult{{createRATClause({0_Lit, 1_Lit, 128_Lit}, 1)}, {CL_ADD, 0x04, 0x02, 0x82, 0x02, 0x00}},
    ProofClauseAndResult{{createRATClause({0_Lit, 1_Lit, 128_Lit}, 2)}, {CL_ADD, 0x82, 0x02, 0x02, 0x04, 0x00}},
    ProofClauseAndResult{{createDeleteClause({0_Lit, 1_Lit, 128_Lit})}, {CL_DEL, 0x02, 0x04, 0x82, 0x02, 0x00}},
    ProofClauseAndResult{
        {
            createATClause({0_Lit}),
            createDeleteClause({~0_Lit, 1_Lit}),
            createRATClause({~0_Lit, 1_Lit, 128_Lit}, 1),
        },
        {CL_ADD, 0x02, 0x00, CL_DEL, 0x03, 0x04, 0x00, CL_ADD, 0x04, 0x03, 0x82, 0x02, 0x00}}
  )
);
// clang-format on


}
