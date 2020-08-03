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
#include <array>
#include <cstdint>
#include <vector>

namespace jamsat {
namespace {
struct TestUIntIndex {
    using Type = uint32_t;
    static size_t getIndex(uint32_t item) { return item; }
};

class TestUIntVec : public std::vector<uint32_t> {
public:
    TestUIntVec() : std::vector<uint32_t>{}, m_deleteFlag{false} {}
    TestUIntVec(std::initializer_list<uint32_t> values)
      : std::vector<uint32_t>{values}, m_deleteFlag{false}, m_modifiedFlag{false} {}

    void setDeleted() noexcept { m_deleteFlag = true; }
    void setModified() noexcept { m_modifiedFlag = true; }
    void clearModified() noexcept { m_modifiedFlag = false; }

    bool isDeleted() const noexcept { return m_deleteFlag; }
    bool isModified() const noexcept { return m_modifiedFlag; }


private:
    bool m_deleteFlag;
    bool m_modifiedFlag;
};

class TestUIntVecDelPred {
public:
    bool operator()(TestUIntVec const* x) const noexcept { return x->isDeleted(); }
};

class TestUIntVecModPred {
public:
    bool operator()(TestUIntVec const* x) const noexcept { return x->isModified(); }
    void clearModified(TestUIntVec& x) noexcept { x.clearModified(); }
};
}


using TestOccMap =
    jamsat::OccurrenceMap<TestUIntVec, TestUIntVecDelPred, TestUIntVecModPred, TestUIntIndex>;

TEST(UnitUtils, emptyOccurrenceMapContainsNoEntries) {
    TestOccMap underTest{10};
    for (uint32_t i = 0; i <= 10; ++i) {
        EXPECT_TRUE(underTest[i].empty());
    }
}


namespace {
using ExpectedTestUIntVecOccMap = std::vector<std::vector<TestUIntVec*>>;

ExpectedTestUIntVecOccMap createExpectedOccMap(size_t maxElement,
                                               std::vector<TestUIntVec*> const& containers) {
    ExpectedTestUIntVecOccMap result;
    result.resize(maxElement);
    for (TestUIntVec* container : containers) {
        for (auto const& element : *container) {
            result[element].push_back(container);
        }
    }
    return result;
}

template <typename OccurrenceMapT, typename OccAnalog>
void expectAnalogousToOccurrenceMap(OccAnalog const& expected,
                                    OccurrenceMapT& underTest,
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
    TestOccMap underTest{31};
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};

    std::vector<std::vector<TestUIntVec*>> expected;
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
    TestOccMap underTest{31};
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};
    TestUIntVec testData3{22, 10};

    std::vector<std::vector<TestUIntVec*>> expected;
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
    TestOccMap underTest{31};
    std::vector<TestUIntVec*> containers;
    underTest.insert(containers.begin(), containers.end());
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_EQ(underTest[i].size(), 0ULL) << "No container expected for index " << i;
    }
}

TEST(UnitUtils, OccurenceMapIsEmptyAfterConstructionWithEmptySequence) {
    std::vector<TestUIntVec*> containers;
    TestOccMap underTest{31, containers.begin(), containers.end()};
    for (uint32_t i = 0; i < 32; ++i) {
        EXPECT_EQ(underTest[i].size(), 0ULL) << "No container expected for index " << i;
    }
}

TEST(UnitUtils, OccurrenceMapContainsExpectedContainersAfterRangeInsert) {
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};

    std::vector<std::vector<TestUIntVec*>> expected;
    expected.resize(32);
    expected[9].push_back(&testData1);
    expected[10].push_back(&testData1);
    expected[10].push_back(&testData2);
    expected[13].push_back(&testData2);
    expected[15].push_back(&testData1);
    expected[22].push_back(&testData2);

    std::vector<TestUIntVec*> testDataVec{&testData1, &testData2};

    TestOccMap underTest{31};
    underTest.insert(testDataVec.begin(), testDataVec.end());
    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

TEST(UnitUtils, OccurrenceMapContainsExpectedContainersAfterRangeConstruction) {
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};

    std::vector<std::vector<TestUIntVec*>> expected;
    expected.resize(32);
    expected[9].push_back(&testData1);
    expected[10].push_back(&testData1);
    expected[10].push_back(&testData2);
    expected[13].push_back(&testData2);
    expected[15].push_back(&testData1);
    expected[22].push_back(&testData2);

    std::vector<TestUIntVec*> testDataVec{&testData1, &testData2};

    TestOccMap underTest{31, testDataVec.begin(), testDataVec.end()};
    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

TEST(UnitUtils, OccurrenceMapContainsNoElementsAfterClear) {
    TestOccMap underTest{31};
    TestUIntVec testData1{9, 10, 15};
    TestUIntVec testData2{22, 10, 13};
    TestUIntVec testData3{22, 10};

    underTest.insert(testData1);
    underTest.insert(testData2);
    underTest.insert(testData3);

    std::vector<std::vector<TestUIntVec*>> expected;
    expected.resize(32);

    underTest.clear();
    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

TEST(UnitUtils, OccurrenceMapDoesNotAddDeletedElements) {
    TestOccMap underTest{31};
    TestUIntVec testData1{9, 10, 15};
    testData1.setDeleted();
    underTest.insert(testData1);
    std::vector<std::vector<TestUIntVec*>> expected;
    expected.resize(32);
    expectAnalogousToOccurrenceMap(expected, underTest, 31);
}

namespace {
template <typename... Ts>
constexpr auto createUIntArray(Ts&&... ts) -> std::array<uint32_t, sizeof...(Ts)> {
    return {static_cast<uint32_t>(ts)...};
}

template <typename T>
auto ptrVec(std::vector<T>& vec) -> std::vector<T*> {
    std::vector<T*> result;
    for (T& t : vec) {
        result.push_back(&t);
    }
    return result;
}

void addAllToOccMap(std::vector<TestUIntVec>& testVecs, TestOccMap& target) {
    for (TestUIntVec& vec : testVecs) {
        target.insert(vec);
    }
}
}

////////////////////////////////////////////////////////////////////////////////
// Occurrence map tests for modifications of the containers referenced by the
// map
//
// These tests are parametrizeed with a set of test input containers and
// a modification function. An occurrence map is created with the given
// containers. After that, the modification function is used to modify the
// containers as well as the occurrence map - e.g. modifying a container
// and updating the occurrence map accordingly. The modification function
// returns a vector of pointers to containers that should still exist in
// the occurrence map, which is used to compute the expected result.
////////////////////////////////////////////////////////////////////////////////

using OccMapModificationFn =
    std::function<std::vector<TestUIntVec*>(std::vector<TestUIntVec*> const&, TestOccMap&)>;

// clang-format off
using OccMapModificationTestParams = std::tuple<
  std::string, // description
  std::vector<TestUIntVec>, // containers
  OccMapModificationFn // modification function
>;
// clang-format on

class UnitUtils_OccurrenceMapModificationTest
  : public ::testing::TestWithParam<OccMapModificationTestParams> {

public:
    void RunTest(bool resolveModificationsBeforeAccess) {
        std::vector<TestUIntVec> testData = std::get<1>(GetParam());
        std::vector<TestUIntVec*> testDataPtrs = ptrVec(testData);

        TestOccMap underTest{31};
        addAllToOccMap(testData, underTest);

        std::vector<TestUIntVec*> expectedRemainingVecs =
            std::get<2>(GetParam())(testDataPtrs, underTest);

        std::vector<std::vector<TestUIntVec*>> expected =
            createExpectedOccMap(32, expectedRemainingVecs);

        if (resolveModificationsBeforeAccess) {
            underTest.resolveModifications();
            expectAnalogousToOccurrenceMap(expected, underTest, 31);
        } else {
            expectAnalogousToOccurrenceMap(expected, underTest, 31);
            underTest.resolveModifications();
        }

        for (TestUIntVec& vec : testData) {
            EXPECT_FALSE(vec.isModified());
        }
    }
};

TEST_P(UnitUtils_OccurrenceMapModificationTest, TestSuiteWithResolveModificationsAfterAccess) {
    RunTest(false);
}

TEST_P(UnitUtils_OccurrenceMapModificationTest, TestSuiteWithResolveModificationsBeforeAccess) {
    RunTest(true);
}

TEST_P(UnitUtils_OccurrenceMapModificationTest,
       TestSuiteWhenOccMapIsDestroyedThenModifiedFlagsAreCleared) {
    std::vector<TestUIntVec> testData = std::get<1>(GetParam());
    std::vector<TestUIntVec*> testDataPtrs = ptrVec(testData);

    {
        TestOccMap underTest{31};
        addAllToOccMap(testData, underTest);
        std::get<2>(GetParam())(testDataPtrs, underTest);
    }

    for (TestUIntVec& vec : testData) {
        EXPECT_FALSE(vec.isModified());
    }
}

// clang-format off

INSTANTIATE_TEST_CASE_P(UnitUtils_OccurrenceMapModificationTest, UnitUtils_OccurrenceMapModificationTest,
  ::testing::Values(
    std::make_tuple("Container modification: drops references to modified containers",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[0]->resize(1);
            testData[0]->setModified();
            underTest.setModified(*testData[0], createUIntArray(), createUIntArray(10, 15));
            return testData;
        }
    ),
    std::make_tuple("Container modification: drops references to empty containers",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[0]->resize(0);
            testData[0]->setModified();
            underTest.setModified(*testData[0], createUIntArray(), createUIntArray(9, 10, 15));
            return testData;
        }
    ),
    std::make_tuple("Container modification: retains references to unmodified containers",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[0]->setModified();
            underTest.setModified(*testData[0], createUIntArray(), createUIntArray());
            return testData;
        }
    ),
    std::make_tuple("Container modification: adds references for new values in container (1 element)",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[0]->push_back(20);
            testData[0]->setModified();
            underTest.setModified(*testData[0], createUIntArray(20), createUIntArray());
            return testData;
        }
    ),
    std::make_tuple("Container modification: adds references for new values in container (2 elements)",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[0]->push_back(20);
            testData[0]->push_back(21);
            testData[0]->setModified();
            underTest.setModified(*testData[0], createUIntArray(20, 21), createUIntArray());
            return testData;
        }
    ),
    std::make_tuple("Container modification: adds references for new values in container together with removing references",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[0]->back() = 20;
            testData[0]->setModified();
            underTest.setModified(*testData[0], createUIntArray(20), createUIntArray(15));
            return testData;
        }
    ),
    std::make_tuple("Container modification: adds/removes references after multiple modified calls",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            testData[1]->pop_back();
            testData[1]->setModified();
            underTest.setModified(*testData[1], createUIntArray(), createUIntArray(13));

            testData[1]->back() = 11;
            testData[1]->push_back(20);
            testData[1]->setModified();
            underTest.setModified(*testData[1], createUIntArray(11, 20), createUIntArray(10));

            return testData;
        }
    ),
    std::make_tuple("Container modification: adding elements to containers after removing them",
        std::vector<TestUIntVec>{{9, 10, 15}, {22, 10, 13}},
        [](std::vector<TestUIntVec*> const& testData, TestOccMap& underTest){
            uint32_t removed = testData[1]->back();
            testData[1]->pop_back();
            testData[1]->setModified();
            underTest.setModified(*testData[1], createUIntArray(), createUIntArray(removed));

            testData[1]->push_back(removed);
            testData[1]->setModified();
            underTest.setModified(*testData[1], createUIntArray(removed), createUIntArray());

            return testData;
        }
    )
  )
);

// clang-format on
}
