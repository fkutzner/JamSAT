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

#include <libjamsat/utils/OccurrenceMap.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace jamsat {
namespace {
struct TestUIntIndex {
    static size_t getIndex(uint32_t item) { return item; }
};

class TestUIntVec : public std::vector<uint32_t> {
public:
    TestUIntVec() : std::vector<uint32_t>{}, m_deleteFlag{false} {}
    TestUIntVec(std::initializer_list<uint32_t> values)
      : std::vector<uint32_t>{values}, m_deleteFlag{false} {}

    void setDeleted() noexcept { m_deleteFlag = true; }

    bool isDeleted() const noexcept { return m_deleteFlag; }


private:
    bool m_deleteFlag;
};

class TestUIntVecDelPred {
public:
    bool operator()(TestUIntVec const *x) const noexcept { return x->isDeleted(); }
};
}

TEST(UnitUtils, emptyOccurrenceMapContainsNoEntries) {
    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{10};
    for (uint32_t i = 0; i <= 10; ++i) {
        EXPECT_TRUE(underTest[i].empty());
    }
}

namespace {
template <typename OccurrenceMapT, typename OccAnalog>
void expectAnalogousToOccurrenceMap(OccAnalog const &expected, OccurrenceMapT &underTest,
                                    uint32_t maxValue) {
    for (uint32_t i = 0; i <= maxValue; ++i) {
        ASSERT_EQ(underTest[i].size(), expected[i].size())
            << "Expected elements mismatch at index " << i;
        auto occurrences = underTest[i];
        EXPECT_TRUE(
            std::is_permutation(occurrences.begin(), occurrences.end(), expected[i].begin()))
            << "Expected elements mismatch at index " << i;
    }
}
}

TEST(UnitUtils, elementsAreRetrievableFromOccurrenceMap) {
    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{31};
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};

    std::vector<std::vector<TestUIntVec *>> expected;
    expected.resize(32);
    expected[9].push_back(&testData1);
    expected[10].push_back(&testData1);
    expected[10].push_back(&testData2);
    expected[13].push_back(&testData2);
    expected[15].push_back(&testData1);
    expected[22].push_back(&testData2);

    underTest.insert(testData1);
    underTest.insert(testData2);

    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

TEST(UnitUtils, deletedElementsAreRemovedFromOccurrenceMap) {
    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{31};
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};
    TestUIntVec testData3{22, 10};

    std::vector<std::vector<TestUIntVec *>> expected;
    expected.resize(32);
    expected[10].push_back(&testData2);
    expected[10].push_back(&testData3);
    expected[13].push_back(&testData2);
    expected[22].push_back(&testData2);
    expected[22].push_back(&testData3);

    underTest.insert(testData1);
    underTest.insert(testData2);
    underTest.insert(testData3);

    testData1.setDeleted();
    underTest.remove(testData1);

    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

TEST(UnitUtils, OccurrenceMapIsEmptyAfterInsertionOfEmptySequence) {
    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{31};
    std::vector<TestUIntVec *> containers;
    underTest.insert(containers.begin(), containers.end());
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_EQ(underTest[i].size(), 0ULL) << "No container expected for index " << i;
    }
}

TEST(UnitUtils, OccurenceMapIsEmptyAfterConstructionWithEmptySequence) {
    std::vector<TestUIntVec *> containers;
    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{
        31, containers.begin(), containers.end()};
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_EQ(underTest[i].size(), 0ULL) << "No container expected for index " << i;
    }
}

TEST(UnitUtils, OccurrenceMapContainsExpectedContainersAfterRangeInsert) {
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};

    std::vector<std::vector<TestUIntVec *>> expected;
    expected.resize(32);
    expected[9].push_back(&testData1);
    expected[10].push_back(&testData1);
    expected[10].push_back(&testData2);
    expected[13].push_back(&testData2);
    expected[15].push_back(&testData1);
    expected[22].push_back(&testData2);

    std::vector<TestUIntVec *> testDataVec{&testData1, &testData2};

    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{31};
    underTest.insert(testDataVec.begin(), testDataVec.end());
    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

TEST(UnitUtils, OccurrenceMapContainsExpectedContainersAfterRangeConstruction) {
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};

    std::vector<std::vector<TestUIntVec *>> expected;
    expected.resize(32);
    expected[9].push_back(&testData1);
    expected[10].push_back(&testData1);
    expected[10].push_back(&testData2);
    expected[13].push_back(&testData2);
    expected[15].push_back(&testData1);
    expected[22].push_back(&testData2);

    std::vector<TestUIntVec *> testDataVec{&testData1, &testData2};

    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntIndex> underTest{
        31, testDataVec.begin(), testDataVec.end()};
    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}
}
