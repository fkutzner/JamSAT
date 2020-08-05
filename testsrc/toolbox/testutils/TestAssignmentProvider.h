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
#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedStack.h>
#include <libjamsat/utils/Truth.h>

#include <algorithm>
#include <vector>

namespace jamsat {

class TestAssignmentProviderClause : public std::vector<CNFLit> {
public:
    enum class Flag : uint32_t { SCHEDULED_FOR_DELETION = 1, REDUNDANT = 2 };

    TestAssignmentProviderClause() : std::vector<CNFLit>{}, m_flags(0), m_lbd(0) {}
    TestAssignmentProviderClause(std::initializer_list<CNFLit> lits)
      : std::vector<CNFLit>{lits}, m_flags(0), m_lbd(0) {}

    void setFlag(Flag flag) noexcept { m_flags |= static_cast<std::underlying_type_t<Flag>>(flag); }

    void clearFlag(Flag flag) noexcept {
        m_flags &= ~(static_cast<std::underlying_type_t<Flag>>(flag));
    }

    bool getFlag(Flag flag) const noexcept {
        return (m_flags & static_cast<std::underlying_type_t<Flag>>(flag)) != 0;
    }

    bool mightContain(CNFLit lit) const noexcept { return std::find(begin(), end(), lit) != end(); }

    bool mightShareAllVarsWith(TestAssignmentProviderClause const& rhs) const noexcept {
        for (auto lit : *this) {
            if (!rhs.mightContain(lit) && !rhs.mightContain(~lit)) {
                return false;
            }
        }
        return true;
    }

    void clauseUpdated() {}

    template <typename L>
    void setLBD(L lbd) {
        JAM_ASSERT(lbd >= 0, "LBD values cannot be negative");
        m_lbd = lbd;
    }

    template <typename L>
    L getLBD() {
        return m_lbd;
    }

private:
    uint32_t m_flags;
    uint64_t m_lbd;
};

class TestAssignmentProvider {
public:
    using Level = size_t;
    using size_type = BoundedStack<CNFLit>::size_type;
    using Clause = TestAssignmentProviderClause;

    TestAssignmentProvider();

    TBool getAssignment(CNFVar variable) const noexcept;
    TBool getAssignment(CNFLit literal) const noexcept;
    void append(CNFLit literal) noexcept;
    void append(CNFLit literal, Clause& clause) noexcept;
    void popLiteral() noexcept;
    size_t getNumberOfAssignments() const noexcept;

    boost::iterator_range<std::vector<CNFLit>::const_iterator>
    getAssignments(size_t index) const noexcept;

    boost::iterator_range<std::vector<CNFLit>::const_iterator>
    getLevelAssignments(Level level) const noexcept;

    Level getLevel(CNFVar variable) const noexcept;
    void setAssignmentDecisionLevel(CNFVar variable, Level level) noexcept;
    Level getCurrentLevel() const noexcept;
    void setCurrentDecisionLevel(Level level) noexcept;

    auto getReason(CNFVar variable) const noexcept -> Clause const*;
    auto getReason(CNFVar variable) noexcept -> Clause*;
    void set_reason(CNFVar variable, Clause* reason) noexcept;

    struct LevelKey {
        using Type = Level;
        static size_t getIndex(Level variable) { return static_cast<size_t>(variable); }
    };

private:
    std::unordered_map<CNFVar, TBool> m_assignments;
    std::unordered_map<CNFVar, Level> m_decisionLevels;
    Level m_currentLevel;
    BoundedStack<CNFLit> m_trail;
    std::unordered_map<CNFVar, Clause*> m_reasons;
};
}
