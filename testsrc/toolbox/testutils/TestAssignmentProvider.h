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

#pragma once

#include <unordered_map>
#include <vector>

#include <boost/range.hpp>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/utils/BoundedStack.h>
#include <libjamsat/utils/Truth.h>

#include <vector>

namespace jamsat {

class TestAssignmentProviderClause : public std::vector<CNFLit> {
public:
    enum class Flag : uint32_t { SCHEDULED_FOR_DELETION = 1 };

    TestAssignmentProviderClause() : std::vector<CNFLit>{}, m_flags(0) {}
    TestAssignmentProviderClause(std::initializer_list<CNFLit> lits)
      : std::vector<CNFLit>{lits}, m_flags(0) {}

    void setFlag(Flag flag) noexcept { m_flags |= static_cast<std::underlying_type_t<Flag>>(flag); }

    void clearFlag(Flag flag) noexcept {
        m_flags &= ~(static_cast<std::underlying_type_t<Flag>>(flag));
    }

    bool getFlag(Flag flag) const noexcept {
        return (m_flags & static_cast<std::underlying_type_t<Flag>>(flag)) != 0;
    }

private:
    uint32_t m_flags;
};

class TestAssignmentProvider {
public:
    using DecisionLevel = size_t;
    using size_type = BoundedStack<CNFLit>::size_type;
    using Clause = TestAssignmentProviderClause;

    TestAssignmentProvider();

    TBool getAssignment(CNFVar variable) const noexcept;
    TBool getAssignment(CNFLit literal) const noexcept;
    void addAssignment(CNFLit literal) noexcept;
    void addAssignment(CNFLit literal, Clause const &clause) noexcept;
    void popLiteral() noexcept;
    size_t getNumberOfAssignments() const noexcept;

    boost::iterator_range<std::vector<CNFLit>::const_iterator> getAssignments(size_t index) const
        noexcept;

    boost::iterator_range<std::vector<CNFLit>::const_iterator>
    getDecisionLevelAssignments(DecisionLevel level) const noexcept;

    DecisionLevel getAssignmentDecisionLevel(CNFVar variable) const noexcept;
    void setAssignmentDecisionLevel(CNFVar variable, DecisionLevel level) noexcept;
    DecisionLevel getCurrentDecisionLevel() const noexcept;
    void setCurrentDecisionLevel(DecisionLevel level) noexcept;

    auto getAssignmentReason(CNFVar variable) const noexcept -> Clause const *;
    void setAssignmentReason(CNFVar variable, Clause const *reason) noexcept;

    struct DecisionLevelKey {
        using Type = DecisionLevel;
        static size_t getIndex(DecisionLevel variable) { return static_cast<size_t>(variable); }
    };

private:
    std::unordered_map<CNFVar, TBool> m_assignments;
    std::unordered_map<CNFVar, DecisionLevel> m_decisionLevels;
    DecisionLevel m_currentLevel;
    BoundedStack<CNFLit> m_trail;
    std::unordered_map<CNFVar, Clause const *> m_reasons;
};
}