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

#include <memory>
#include <string>

#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/proof/DRATCertificate.h>

namespace jamsat {

class OnlineDRATChecker : public DRATCertificate {
public:
  virtual auto hasValidatedUnsat() const noexcept -> bool = 0;
  virtual auto hasDetectedInvalidLemma() const noexcept -> bool = 0;
  virtual auto hasDetectedUnsupportedLemma() const noexcept -> bool = 0;

  virtual auto getResultComments() const noexcept -> std::vector<std::string> const& = 0;
};

/**
 * \brief Creates a DRATChecker that checks each lemma
 *
 * This checker lacks all optimizations (clause marking, core-first propagation, ...)
 * and is only intended for acceptance-testing the DRAT certificate generation on
 * problem instance.
 *
 * \ingroup JamSAT_TestInfrastructure
 */
std::unique_ptr<OnlineDRATChecker> createOnlineDRATChecker(CNFProblem const& problem);
}
