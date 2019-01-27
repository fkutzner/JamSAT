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

#pragma once

#include <boost/range/algorithm_ext/erase.hpp>
#include <vector>

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Logger.h>
#include <libjamsat/utils/OverApproximatingSet.h>

#if defined(JAM_ENABLE_MINIMIZER_LOGGING)
#define JAM_LOG_MINIMIZER(x, y) JAM_LOG(x, "minmiz", y)
#else
#define JAM_LOG_MINIMIZER(x, y)
#endif

namespace jamsat {

/**
 * \defgroup JamSAT_Simplification_Minimizer  Lemma Minimization
 * \ingroup JamSAT_Simplification
 *
 * This module contains functions for simplifying lemmas just after
 * the learning process, before they are added to the problem instance
 * in the form of regular clauses.
 */

/**
 * \ingroup JamSAT_Simplification_Minimizer
 *
 * \brief Erases redundant literals from the given clause.
 *
 * Erases literals from \p literals which are redundant wrt. reason clauses
 * given  via \p reasonProvider .
 *
 * A literal l is said to be <i>redundant</i> if l has an assigned value, and
 * either
 *  - occurs on decision level 0 or
 *  - l is not a decision literal and every false-assigned literal in l's reason
 * is either contained in \p clause or is redundant.
 *
 * [Knuth, The Art of Computer Programming, chapter 7.2.2.2, exercise 257]
 *
 * Literals on the current decision level are not checked for being redundant.
 * (Note that if a clause has been learnt via first-UIP clause learning, it
 * contains a single literal on the current decision level, and that literal
 * cannot be redundant.) Literals occurring on other decision levels than the
 * current one must currently be assignmened to FALSE.
 *
 * Usage example: Remove redundant literals from a conflicting clause returned
 * by first-UIP conflict analysis, using Propagation as an assignment reason
 * provider and Trail as a decision level provider.
 *
 *
 * \param[in,out] literals    The container of CNFLits in which redundant
 * literals should be erased.
 * \param[in] reasonProvider  A reason provider.
 * \param[in] dlProvider      A decision level provider.
 * \param[in,out] tempStamps  a clean StampMap supporting stamping CNFVar values
 * occuring in \p literals and any reason clause in \p reasonProvider . When
 * this function returns, \p tempStamps is clean.
 *
 * \tparam ReasonProvider        A type satisfying the \ref ReasonProvider concept.
 * \tparam DecisionLevelProvider A type satisfying the \ref DecisionLevelProvider concept.
 * TODO: document other template parameters
 *
 * TODO: document that LiteralContainer must support erasing
 */
template <class LiteralContainer,
          class ReasonProvider,
          class DecisionLevelProvider,
          class StampMapT>
void eraseRedundantLiterals(LiteralContainer& literals,
                            const ReasonProvider& reasonProvider,
                            const DecisionLevelProvider& dlProvider,
                            StampMapT& tempStamps) noexcept;

/**
 * \ingroup JamSAT_Simplification_Minimizer
 *
 * \brief Erases literals from the given clause which can be removed via
 * resolution with binary clauses.
 *
 * Example: Given a clause \p literals =  (a, b, c, d) as \p literals and \p
 * resolveAt = d, removes literals a and b from \p literals if there are binary
 * clauses (c, -a) and (c, -b).
 *
 * Usage example: Use this function to minimize conflicting clauses (e.g. with
 * \p resolveAt being the asserting literal) before using these clause as learnt
 * clauses.
 *
 * \param[in,out] literals      The container of CNFLit values in which literals
 * removable via resolution with \p resolveAt should be removed.
 * \param[in] binaryClauses     A map mapping CNFLit values l to containers of
 * CNFLit m1, ..., mN values representing the binary clauses (l, m1), ..., (l,
 * mN). The map must not contain the binary any of the clauses (l, l) and (l,
 * -l).
 * \param[in] resolveAt         The literal at which resolution should be
 * performed. \p resolveAt must be contained in \p literals .
 * \param[in,out] tempStamps    a clean StampMap supporting stamping CNFVar
 * values occuring in \p literals and any reason clause in \p reasonProvider .
 * When this function returns, \p tempStamps is clean.
 *
 * \tparam LiteralContainer         TODO
 * \tparam BinaryClausesProvider    TODO
 * \tparam StampMapT                TODO
 */
template <class LiteralContainer, class BinaryClausesProvider, class StampMapT>
void resolveWithBinaries(LiteralContainer& literals,
                         BinaryClausesProvider& binaryClauses,
                         CNFLit resolveAt,
                         StampMapT& tempStamps) noexcept;

/********** Implementation ****************************** */

namespace erl_detail {

// LiteralRedundancyChecker allows reusing m_work and m_stampCleanup by the isRedundant
// function, saving allocations which were prone to become a bottleneck.
class LiteralRedundancyChecker {
public:
    template <class ReasonProvider, class DecisionLevelProvider, class StampMapT, class DLSet>
    bool isRedundant(CNFLit literal,
                     const ReasonProvider& reasonProvider,
                     const DecisionLevelProvider& dlProvider,
                     StampMapT& tempStamps,
                     typename StampMapT::Stamp currentStamp,
                     DLSet decisionLevelsInLemma) {

        if (dlProvider.getAssignmentDecisionLevel(literal.getVariable()) ==
            dlProvider.getCurrentDecisionLevel()) {
            return false;
        }

        m_work.clear();
        m_work.push_back(literal.getVariable());

        // If the redundancy check fails, all stamps added here must be removed. Collect them in
        // m_stampCleanup:
        m_stampCleanup.clear();

        while (!m_work.empty()) {
            CNFVar workItem = m_work.back();
            m_work.pop_back();

            auto clausePtr = reasonProvider.getAssignmentReason(workItem);
            JAM_LOG_MINIMIZER(info,
                              "  Checking if lits with variable "
                                  << workItem << " and reason " << clausePtr << " are redundant.");
            JAM_ASSERT(clausePtr != nullptr, "Can't determine redundancy of reasonless literals");


            decisionLevelsInLemma.insert(0);
            for (CNFLit lit : *clausePtr) {
                CNFVar var = lit.getVariable();
                auto varLevel = dlProvider.getAssignmentDecisionLevel(var);

                if (!decisionLevelsInLemma.mightContain(varLevel)) {
                    // The reason of lit will contain at least two literals on varLevel.
                    // Thus, if there is definitely no literal on varLevel in the lemma,
                    // lit cannot be redundant.
                    for (auto stampedVar : m_stampCleanup) {
                        tempStamps.setStamped(stampedVar, currentStamp, false);
                    }
                    return false;
                }

                if (varLevel == 0 || tempStamps.isStamped(var, currentStamp)) {
                    JAM_LOG_MINIMIZER(
                        info, "    Reason lit " << lit << " is on level 0 or has been visited");
                    continue;
                }

                if (reasonProvider.getAssignmentReason(var) != nullptr) {
                    JAM_LOG_MINIMIZER(
                        info, "    Reason lit " << lit << " not checked yet, adding to queue");
                    tempStamps.setStamped(var, currentStamp, true);
                    m_work.push_back(var);
                    m_stampCleanup.push_back(var);
                } else {
                    JAM_LOG_MINIMIZER(info, "    lit " << lit << " is not redundant");
                    for (auto stampedVar : m_stampCleanup) {
                        tempStamps.setStamped(stampedVar, currentStamp, false);
                    }
                    return false;
                }
            }
        }

        JAM_LOG_MINIMIZER(info, "Literal " << literal << " is redundant");
        return true;
    }

private:
    // no information contained in m_work and m_stampCleanup is reused by isRedundant()
    // - these member variables purely exist to save allocations.
    std::vector<CNFVar> m_work;
    std::vector<CNFVar> m_stampCleanup;
};
}

template <class LiteralContainer,
          class ReasonProvider,
          class DecisionLevelProvider,
          class StampMapT>
void eraseRedundantLiterals(LiteralContainer& literals,
                            const ReasonProvider& reasonProvider,
                            const DecisionLevelProvider& dlProvider,
                            StampMapT& tempStamps) noexcept {
    const auto stampContext = tempStamps.createContext();
    const auto stamp = stampContext.getStamp();

    OverApproximatingSet<64, typename DecisionLevelProvider::DecisionLevelKey> decisionLevels;

    for (auto literal : literals) {
        tempStamps.setStamped(literal.getVariable(), stamp, true);
        decisionLevels.insert(dlProvider.getAssignmentDecisionLevel(literal.getVariable()));
    }

    erl_detail::LiteralRedundancyChecker redundancyChecker;

    auto isRedundant =
        [&reasonProvider, &dlProvider, &tempStamps, &stamp, &redundancyChecker, decisionLevels](
            CNFLit literal) {
            auto reason = reasonProvider.getAssignmentReason(literal.getVariable());
            if (reason != nullptr) {
                JAM_LOG_MINIMIZER(info,
                                  "Checking if lit " << literal << " with reason " << reason
                                                     << " is redundant.");
                return redundancyChecker.isRedundant(
                    literal, reasonProvider, dlProvider, tempStamps, stamp, decisionLevels);
            }
            return dlProvider.getAssignmentDecisionLevel(literal.getVariable()) == 0;
        };

    boost::remove_erase_if(literals, isRedundant);
}

template <class LiteralContainer, class BinaryClausesProvider, class StampMapT>
void resolveWithBinaries(LiteralContainer& literals,
                         BinaryClausesProvider& binaryClauses,
                         CNFLit resolveAt,
                         StampMapT& tempStamps) noexcept {
    const auto stampContext = tempStamps.createContext();
    const auto stamp = stampContext.getStamp();

    if (binaryClauses[resolveAt].empty()) {
        return;
    }

    for (auto secondLiteral : binaryClauses[resolveAt]) {
        tempStamps.setStamped(secondLiteral, stamp, true);
    }

    auto mayRemoveByResolution = [&tempStamps, stamp](CNFLit literal) {
        return tempStamps.isStamped(~literal, stamp);
    };

    boost::remove_erase_if(literals, mayRemoveByResolution);
}
}
