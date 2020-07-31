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

/**
 * \file Model.h
 * \brief Data structure for CNF problem satisfiability proofs
 */

#pragma once

#include <memory>

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Proof
 *
 * \brief A model for a CNF problem instance (proof for its satisfiability).
 */
class Model {
public:
    /**
     * \brief Sets the assignment of the given variable.
     *
     * \param variable  A regular variable.
     * \param value     The variable's value in the model.
     */
    virtual void setAssignment(CNFVar variable, TBool value) noexcept = 0;

    /**
     * \brief Gets the assignment of the given variable.
     *
     * If the variable has not been assigned via setAssignment(), TBools::INDETERMINATE
     * is returned.
     *
     * \param variable  A regular variable.
     * \returns         The variable's assignment.
     */
    virtual TBool getAssignment(CNFVar variable) const noexcept = 0;

    /**
     * \brief Checks the proof.
     *
     * \param problem       A CNF problem.
     * \returns             TBools::TRUE if the model is a model for \p problem; TBools::FALSE
     *                      otherwise.
     */
    virtual TBool check(const CNFProblem& problem) const noexcept = 0;

    // TODO: add getAssignments() returning a range of assignments

    Model& operator=(const Model& other) = delete;
    Model& operator=(Model&& other) = delete;
    Model(const Model& other) = delete;
    Model(Model&& other) = delete;

    Model();
    virtual ~Model();
};

/**
 * \ingroup JamSAT_Proof
 *
 * \brief Creates a new Model instance.
 *
 * \param initialMaxVar   The maximum variable expected to occur in the model. If assignments are
 *                        stored for greater variables than \p initialMaxVar, the data structure
 *                        is automatically resized appropriately.
 * \returns               A unique_ptr to the new Model instance.
 */
std::unique_ptr<Model> createModel(CNFVar initialMaxVar);
}
