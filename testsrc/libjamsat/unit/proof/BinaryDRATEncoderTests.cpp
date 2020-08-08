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

#include <unordered_map>
#include <utility>
#include <vector>

#include <libjamsat/proof/BinaryDRATEncoder.h>

#include <libjamsat/cnfproblem/CNFLiteral.h>

namespace jamsat {

// clang-format off
using BinaryDRATEncoderTestsParams = std::tuple<
  std::vector<CNFLit>, // input
  std::vector<unsigned char> //expected
>;
// clang-format on

class BinaryDRATEncoderTests : public ::testing::TestWithParam<BinaryDRATEncoderTestsParams> {
public:
    virtual ~BinaryDRATEncoderTests() = default;
};

TEST_P(BinaryDRATEncoderTests, BinaryDRATEncoderComputesExpectedResult) {
    std::vector<CNFLit> testInput = std::get<0>(GetParam());
    std::vector<unsigned char> expected = std::get<1>(GetParam());

    std::vector<unsigned char> buffer;
    buffer.resize(5 * expected.size());

    std::size_t writtenBytes = EncodeBinaryDRAT(testInput, buffer);

    ASSERT_THAT(writtenBytes, ::testing::Eq(expected.size()));

    buffer.resize(writtenBytes);
    EXPECT_THAT(buffer, ::testing::ContainerEq(expected));
}

namespace {
using LitDratMap = std::unordered_map<CNFLit, std::vector<unsigned char>>;

auto createLitDratMap() -> LitDratMap {
    LitDratMap result;
    result[0_Lit] = {0x00};
    result[~0_Lit] = {0x01};
    result[~63_Lit] = {0x7F};
    result[64_Lit] = {0x80, 0x01};
    result[129_Lit] = {0x82, 0x02};
    result[~8191_Lit] = {0xFF, 0x7F};
    result[~8193_Lit] = {0x83, 0x80, 0x01};
    result[~134217727_Lit] = {0xFF, 0xFF, 0xFF, 0x7F};
    result[~134217731_Lit] = {0x87, 0x80, 0x80, 0x80, 0x01};
    return result;
}

auto createDRATTestParams(std::vector<CNFLit> lits) -> BinaryDRATEncoderTestsParams {
    static const LitDratMap testLitMappings = createLitDratMap();
    std::vector<unsigned char> expected;
    for (CNFLit lit : lits) {
        if (auto drat = testLitMappings.find(lit); drat != testLitMappings.end()) {
            expected.insert(expected.end(), drat->second.begin(), drat->second.end());
        } else {
            throw std::invalid_argument{"Encountered literal with unknown drat representation"};
        }
    }

    return std::make_tuple(lits, expected);
}
}

// clang-format off
INSTANTIATE_TEST_CASE_P(Unit_Proof, BinaryDRATEncoderTests,
  ::testing::Values(
      createDRATTestParams({}),
      createDRATTestParams({0_Lit}),
      createDRATTestParams({~0_Lit}),
      createDRATTestParams({~63_Lit}),
      createDRATTestParams({64_Lit}),
      createDRATTestParams({129_Lit}),
      createDRATTestParams({~8191_Lit}),
      createDRATTestParams({~8193_Lit}),
      createDRATTestParams({~134217727_Lit}),
      createDRATTestParams({~134217731_Lit}),
      createDRATTestParams({~63_Lit, 129_Lit}),
      createDRATTestParams({~63_Lit, 129_Lit, ~8191_Lit})
  )
);
// clang-format on
}
