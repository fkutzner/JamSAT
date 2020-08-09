/* Copyright (c) 2019 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/JamSatIpasir.h>

#include <memory>
#include <vector>

namespace jamsat {

/**
 * \brief Interface for IPASIR API wrappers
 *
 * \ingroup JamSAT_Frontend
 */
class IpasirSolver {
public:
  IpasirSolver();
  virtual ~IpasirSolver();

  /**
   * \brief Adds a clause to the solver.
   *
   * \param literals      The clause to add.
   */
  virtual void addClause(std::vector<int> const& literals) noexcept = 0;

  enum class Result { INDETERMINATE = 0, SATISFIABLE = 10, UNSATISFIABLE = 20 };

  /**
   * \brief Invokes the SAT solver.
   *
   * \param assumedFacts  The vector of literals that shall be assumed to be true
   *                      during the solver invocation.
   *
   * \returns the solving result.
   */
  virtual auto solve(std::vector<int> const& assumedFacts) noexcept -> Result = 0;

  enum class Value { TRUE, FALSE, DONTCARE };

  /**
   * \brief Gets the value of a literal.
   *
   * This function may only be called if the last `solve()` call returned
   * Result::SATISFIABLE and `addClause()` has not been called since the last
   * call to `solve()`.
   *
   * \param literal       A literal. `literal` must not be 0.
   *
   * \returns the value of the literal.
   */
  virtual auto getValue(int literal) noexcept -> Value = 0;

  /**
   * \brief Determines whether an assumed fact has been used to prove unsatisfiability.
   *
   * This function may only be called if the last `solve()` call returned
   * Result::UNSATISFIABLE and `addClause()` has not been called since the last
   * call to `solve()`.
   *
   * \param literal       A literal. `literal` must not be 0.
   *
   * \returns `true` if the literal is an assumed fact that has been used to prove
   *          unsatisfiability.
   */
  virtual auto isFailed(int literal) noexcept -> bool = 0;

  /**
   * \brief Wrapper for ipasir_set_terminate()
   *
   * See the documentation of ipasir_set_terminate().
   */
  virtual void setTerminateFn(void* state, int (*terminate)(void* state)) noexcept = 0;

  /**
   * \brief Wrapper for ipasir_set_learn()
   *
   * See the documentation of ipasir_set_learn().
   */
  virtual void
  setLearnFn(void* state, int max_length, void (*learn)(void* state, int* clause)) noexcept = 0;

  /**
   * \brief Wrapper for jamsat_ipasir_set_logger()
   *
   * Prints the log messages obtained from JamSAT to the given stream.
   * 
   * \param targetStream      An ostream. The argument must reference an object that remains
   *                          valid until the destruction of the IpasirSolver object.
   */
  virtual void enableLogging(std::ostream& targetStream) noexcept = 0;

  IpasirSolver(IpasirSolver const& rhs) = delete;
  IpasirSolver(IpasirSolver&& rhs) = delete;
  auto operator=(IpasirSolver const& rhs) = delete;
  auto operator=(IpasirSolver&& rhs) = delete;
};

/**
 * \brief Creates a solver using the IPASIR API.
 *
 * \ingroup JamSAT_Frontend
 *
 * \returns a solver using the IPASIR API.
 */
auto createIpasirSolver() -> std::unique_ptr<IpasirSolver>;
}
