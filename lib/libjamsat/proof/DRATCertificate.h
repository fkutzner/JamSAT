/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

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
 * \file DRATCertificate.h
 * \brief Data structure for collecting CNF problem DRAT unsatisfiability proofs
 */

#pragma once

#include <exception>
#include <memory>

#include <gsl/span>

#include <libjamsat/cnfproblem/CNFLiteral.h>

namespace jamsat {

class FileIOError : public std::exception {};

/**
 * \ingroup JamSAT_Proof
 *
 * \brief A DRAT (Delete Resolution Asymmetric Tautology) proof for
 *        unsatisfiability of a CNF problem instance.
 *
 * See https://github.com/marijnheule/drat-trim for details about DRAT proofs.
 */
class DRATCertificate {
public:
    /**
     * \brief Adds a resolution asymmetric tautology clause to the proof.
     * 
     * \param clause    A clause.
     * \param pivotIdx  The index of the pivot literal within \p{clause}.
     * 
     * \throw FileIOError           on file i/o errors
     */
    virtual void addRATClause(gsl::span<CNFLit const> clause, size_t pivotIdx) = 0;

    /**
     * \brief Adds an asymmetric tautology clause to the proof.
     * 
     * \param clause    A clause.
     * 
     * \throw FileIOError           on file i/o errors
     */
    virtual void addATClause(gsl::span<CNFLit const> clause) = 0;

    /**
     * \brief Adds a clause deletion to the proof.
     *
     * \param clause                A clause.
     * 
     * \throw FileIOError           on file i/o errors
     */
    virtual void deleteClause(gsl::span<CNFLit const> clause) = 0;


    /**
     * \brief Flushes the proof to its target.
     *
     * \throw FileIOError           on file i/o errors
     */
    virtual void flush() = 0;

    DRATCertificate& operator=(const DRATCertificate& other) = delete;
    DRATCertificate& operator=(DRATCertificate&& other) = delete;
    DRATCertificate(const DRATCertificate& other) = delete;
    DRATCertificate(DRATCertificate&& other) = delete;

    DRATCertificate() = default;
    virtual ~DRATCertificate() = default;
};

/**
 * \ingroup JamSAT_Proof
 * 
 * \brief Creates a file-based DRATCertificate
 * 
 * The proof is emitted in the binary DRAT format.
 * 
 * \throw FileIOError when the file could not be opened
 * 
 * TODO: use std::filesystem even though it's not supported
 *   in VS2017 and macOS 10.13?
 */
auto createFileDRATCertificate(std::string const& path) -> std::unique_ptr<DRATCertificate>;

}
