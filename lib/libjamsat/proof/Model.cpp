/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/proof/Model.h>
#include <libjamsat/utils/BoundedMap.h>

namespace jamsat {

Model::Model() {}

Model::~Model() {}

class ModelImpl : public Model {
public:
  explicit ModelImpl(CNFVar maxVar);

  void setAssignment(CNFVar variable, TBool value) noexcept override;
  TBool getAssignment(CNFVar variable) const noexcept override;

  TBool check(const CNFProblem& problem) const noexcept override;

  // TODO: implement getAssignments()

  ModelImpl& operator=(const ModelImpl& other) = delete;
  ModelImpl& operator=(const ModelImpl&& other) = delete;
  ModelImpl(const Model& other) = delete;
  ModelImpl(const Model&& other) = delete;

  virtual ~ModelImpl();

private:
  BoundedMap<CNFVar, TBool> m_assignments;
  CNFVar m_currentMaxVar;
};

ModelImpl::ModelImpl(CNFVar maxVar)
  : Model(), m_assignments(maxVar, TBools::INDETERMINATE), m_currentMaxVar(maxVar)
{
}

ModelImpl::~ModelImpl() {}

void ModelImpl::setAssignment(CNFVar variable, TBool value) noexcept
{
  if (variable > m_currentMaxVar) {
    m_currentMaxVar = variable;
    m_assignments.increaseSizeTo(variable);
  }
  m_assignments[variable] = value;
}

TBool ModelImpl::getAssignment(CNFVar variable) const noexcept
{
  if (variable <= m_currentMaxVar) {
    return m_assignments[variable];
  }
  return TBools::INDETERMINATE;
}

TBool ModelImpl::check(const jamsat::CNFProblem& problem) const noexcept
{
  for (auto& clause : problem.getClauses()) {
    bool satisfied = false;
    for (auto lit : clause) {
      TBool expectedValue = (lit.getSign() == CNFSign::POSITIVE) ? TBools::TRUE : TBools::FALSE;
      satisfied = satisfied || (getAssignment(lit.getVariable()) == expectedValue);
    }
    if (!satisfied) {
      return TBools::FALSE;
    }
  }
  return TBools::TRUE;
}

std::unique_ptr<Model> createModel(CNFVar maxVar)
{
  return std::make_unique<ModelImpl>(maxVar);
}
}
