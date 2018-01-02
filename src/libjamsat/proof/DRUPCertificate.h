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

#include <array>
#include <memory>
#include <ostream>

#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/utils/Truth.h>

namespace jamsat {

/**
 * \ingroup JamSAT_Proof
 *
 * \brief A DRUP (Delete Reverse Unit Propagation) certificate.
 *
 * This class is a generic interface for DRUP proofs. Implementations may emit the proof
 * formatted as plaintext DRUP or binary DRUP. See e.g.:
 * https://baldur.iti.kit.edu/sat-competition-2016/index.php?cat=certificates
 */
class DRUPCertificate {
public:
    /**
     * \brief Adds a clause to the proof satisfying the RUP property.
     *
     * \tparam LiteralForwardRange  A forward range with iterators over CNFLit.
     * \param clause                A clause.
     */
    template <typename LiteralForwardRange>
    void addRUPClause(const LiteralForwardRange &clause);

    /**
     * \brief Adds a clause deletion to the proof.
     *
     * \tparam LiteralForwardRange  A forward range with iterators over CNFLit.
     * \param clause                A clause.
     */
    template <typename LiteralForwardRange>
    void deleteClause(const LiteralForwardRange &clause);

    /**
     * \brief Closes the UNSAT proof.
     *
     * Call this method only when the problem is unsatisfiable.
     */
    virtual void closeProof() = 0;

    DRUPCertificate &operator=(const DRUPCertificate &other) = delete;
    DRUPCertificate &operator=(DRUPCertificate &&other) = delete;
    DRUPCertificate(const DRUPCertificate &other) = delete;
    DRUPCertificate(DRUPCertificate &&other) = delete;

    virtual ~DRUPCertificate();

protected:
    DRUPCertificate() = default;

    template <typename LiteralForwardRange>
    void addClause(const LiteralForwardRange &clause);

    virtual void beginDeletedClause() = 0;
    virtual void addLiteral(CNFLit lit) = 0;
    virtual void endClause() = 0;
};

/********** Implementation ****************************** */

template <typename LiteralForwardRange>
void DRUPCertificate::addClause(const LiteralForwardRange &clause) {
    for (CNFLit lit : clause) {
        addLiteral(lit);
    }
    endClause();
}

template <typename LiteralForwardRange>
void DRUPCertificate::addRUPClause(const LiteralForwardRange &clause) {
    addClause(clause);
}

template <typename LiteralForwardRange>
void DRUPCertificate::deleteClause(const LiteralForwardRange &clause) {
    beginDeletedClause();
    addClause(clause);
}

std::unique_ptr<DRUPCertificate> createPlainDRUPCertificate(std::ostream &outputStream);
}
