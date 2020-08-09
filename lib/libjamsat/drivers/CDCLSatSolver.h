/* Copyright (c) 2017, 2018, 2019 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/proof/Model.h>
#include <libjamsat/utils/Truth.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

/**
 * \file CDCLSatSolver.h
 * \brief Generic interface and factories for SAT solvers using CDCL search
 */


namespace jamsat {

class DRATCertificate;

/**
 * \ingroup JamSAT_Drivers
 *
 * \brief Representation of a SAT solving result
 */
class SolvingResult {
public:
    /**
     * \brief Returns the problem's satisfiability status.
     *
     * \returns If resource limits have been exceeded or `stop()` has been called
     *          during the execution of `solve()`, `TBools::INDETERMINATE` is
     *          returned. Otherwise, TBools::TRUE rsp. TBools::FALSE is returned if
     *          the CNF problem instance is satsifiable rsp. unsatisfiable with
     *          respect to the setting of the assumptions.
     */
    virtual auto isProblemSatisfiable() const noexcept -> TBool = 0;


    /**
     * \brief Returns a model for the problem instance.
     *
     * This method may not be called after `takeModel()` has been called.
     *
     * \returns If the problem instance has been determined to be satisfiable, a reference to
     * a satisfying assignment ("model") is returned. Otherwise, nothing is returned.
     */
    virtual auto getModel() const noexcept
        -> std::optional<std::reference_wrapper<Model const>> = 0;

    /**
     * \brief Returns a list of assumed facts that have been used to obtain the UNSAT result.
     *
     * If the problem instance has not been detected to be unsatisfiable, the
     * result is an empty vector. If the problem instance has been detected to be unsatisfiable
     * and the result of this method is empty, the problem instance is unsatisfiable
     * regardless of the assumed fact setting.
     *
     * \returns a list of assumed facts that have been used to obtain the UNSAT result.
     */
    virtual auto getFailedAssumptions() const noexcept -> std::vector<CNFLit> const& = 0;

    virtual ~SolvingResult();
};

/**
 * \ingroup JamSAT_Drivers
 *
 * \brief CDCL-based SAT solver
 */
class CDCLSatSolver {
public:
    /**
     * \brief Adds the clauses of the given CNF problem instance to be solved to the
     *        solver.
     *
     * \param problem   The CNF problem instance to be added.
     *
     * \throws std::bad_alloc   The clause database does not have enough memory to
     *                          hold \p problem
     */
    virtual void addProblem(CNFProblem const& problem) = 0;

    /**
     * \brief Adds a clause of the CNF problem instance to be solved to the solver.
     *
     * \param clause    A CNF clause.
     *
     * \throws std::bad_alloc   The clause database does not have enough memory to
     *                          hold \p clause
     */
    virtual void addClause(CNFClause const& clause) = 0;

    /**
     * \brief Determines whether the CNF problem specified via the methods
     *        `addProblem()` rsp. `addClause()` is satisfiable.
     *
     * \param assumedFacts   A collection of literals which the solver shall
     *                       assume to have the value "true".
     *
     * \return the solving result.
     *
     * \throws std::bad_alloc   The solver is out of memory and cannot recover
     *   from that condition. No resources are leaked. On further calls, the solver
     *   will either throw a `bad_alloc` exception or return an INDETERMINATE result,
     *   but will not produce a wrong result (basic exception guarantee).
     * 
     * \throws FileIOError      The solver failed to write to disk and the solver
     *   cannot recover from that condition. On further calls, the solver will
     *   return INDETERMINATE. This exception can only be thrown if UNSAT certificate
     *   generation is enabled.
     */
    virtual auto solve(std::vector<CNFLit> const& assumedFacts)
        -> std::unique_ptr<SolvingResult> = 0;

    /**
     * \brief Asynchronously instructs the solver to stop solving.
     *
     * This method may be called while `solve()` is being executed. When `solve()`
     * is being executed and this method is called, the solver will stop execution
     * in a timely manner. Calling this method while `solve()` is not being executed
     * has no effect.
     */
    virtual void stop() noexcept = 0;

    using LoggerFn = std::function<void(std::string const&)>;

    /**
     * \brief Sets a logger function.
     * 
     * Sets a function periodically receiving details about the solving process.
     */
    virtual void setLogger(LoggerFn loggerFunction) = 0;


    /**
     * \brief Sets the object receiving the DRUP certificate clauses.
     *
     * When this object is set, DRAT certificate generation is activated.
     *
     * Caveat: when solving with assumptions, the generated proof relates
     * to the original problem with the assumptions added as facts.
     * 
     * Also caveat: when using proofs in incremental mode, you need to
     * specify a proof object before each solve call.
     *
     * \param cert      The DRUP certificate object. The referenced object
     *                  must be valid until the destruction of the solver
     *                  or until another DRUP certificate object is set,
     *                  whichever happens sooner.
     */
    virtual void setDRATCertificate(DRATCertificate& cert) noexcept = 0;

    virtual ~CDCLSatSolver();
};

/**
 * Creates a CDCL SAT solver.
 *
 * \returns a new CDCL SAT solver.
 */
auto createCDCLSatSolver() -> std::unique_ptr<CDCLSatSolver>;
}