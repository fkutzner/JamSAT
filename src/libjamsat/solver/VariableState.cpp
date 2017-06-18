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

#include "VariableState.h"

namespace jamsat {
VariableState::VariableState(CNFVar maxVar)
    : m_eliminatedVariables({}), m_reasons({}) {
  m_eliminatedVariables.resize(maxVar.getRawValue() + 1);
  m_reasons.resize(maxVar.getRawValue() + 1);
}

bool VariableState::isEliminated(CNFVar variable) const noexcept {
  JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariableType>(
                                          m_eliminatedVariables.size()),
             "Variable out of bounds");
  return toRawBool(m_eliminatedVariables[variable.getRawValue()]);
}

void VariableState::setEliminated(CNFVar variable) noexcept {
  JAM_ASSERT(variable.getRawValue() < static_cast<CNFVar::RawVariableType>(
                                          m_eliminatedVariables.size()),
             "Variable out of bounds");
  m_eliminatedVariables[variable.getRawValue()] = Bool::TRUE;
}

const Clause *VariableState::getAssignmentReason(CNFVar variable) const
    noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Variable out of bounds");
  return m_reasons[variable.getRawValue()];
}

void VariableState::setAssignmentReason(CNFVar variable,
                                        Clause *reason) noexcept {
  JAM_ASSERT(variable.getRawValue() <
                 static_cast<CNFVar::RawVariableType>(m_reasons.size()),
             "Variable out of bounds");
  m_reasons[variable.getRawValue()] = reason;
}
}
