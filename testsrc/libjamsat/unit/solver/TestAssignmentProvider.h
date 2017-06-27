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
#include <libjamsat/utils/Truth.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>

namespace jamsat {
  class TestAssignmentProvider {
  public:
    using DecisionLevel = size_t;

    TestAssignmentProvider();

    TBool getAssignment(CNFVar variable) const noexcept;
    TBool getAssignment(CNFLit literal) const noexcept;
    void addLiteral(CNFLit literal) noexcept;
    void clearLiteral(CNFLit literal) noexcept;
    size_t getNumberOfAssignments() const noexcept;
    boost::iterator_range<std::vector<CNFLit>::const_iterator>
    getAssignments(size_t index) const noexcept;

    DecisionLevel getDecisionLevel(CNFVar variable) const noexcept;
    void setDecisionLevel(CNFVar variable, DecisionLevel level) noexcept;
    DecisionLevel getCurrentDecisionLevel() const noexcept;
    void setCurrentDecisionLevel(DecisionLevel level) noexcept;

  private:
    std::unordered_map<CNFVar, TBool> m_assignments;
    std::unordered_map<CNFVar, DecisionLevel> m_decisionLevels;
    DecisionLevel m_currentLevel;
    std::vector<CNFLit> m_trail;
  };
}
