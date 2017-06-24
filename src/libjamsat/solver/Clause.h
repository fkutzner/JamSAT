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

#include <cstdint>
#include <memory>

#include <libjamsat/cnfproblem/CNFLiteral.h>

namespace jamsat {
/**
 * \ingroup JamSAT_Solver
 *
 * \class jamsat::Clause
 *
 * \brief The internal clause data structure
 */
class Clause {
public:
  using size_type = size_t;
  using iterator = CNFLit *;
  using const_iterator = const CNFLit *;

  /**
   * \brief Returns a reference to a literal within the clause.
   *
   * \param index The index of the target literal. \p index must be smaller than
   * the clause size.
   * \returns A reference to the literal with index \p index.
   */
  CNFLit &operator[](size_type index) noexcept;

  /**
   * \brief Returns the clause's size.
   *
   * \returns The clause's size.
   */
  size_type getSize() const noexcept;

  /**
   * \brief Reduces the length of the clause to the given size.
   *
   * \param newSize  The clause's new size, which must not be larger than the
   * current size.
   */
  void shrink(size_type newSize) noexcept;

  /**
   * \brief Gets the begin random-access iterator for the literal list.
   *
   * \returns The begin iterator for the literal list.
   */
  iterator begin() noexcept;

  /**
   * \brief Gets the end random-access iterator for the literal list.
   *
   * \returns The end iterator for the literal list. This iterator may not be
   * dereferenced.
   */
  iterator end() noexcept;

  /**
   * \brief Gets the begin random-access const iterator for the literal list.
   *
   * \returns The begin const iterator for the literal list.
   */
  const_iterator begin() const noexcept;

  /**
   * \brief Gets the end random-access const iterator for the literal list.
   *
   * \returns The end const iterator for the literal list. This iterator may not
   * be dereferenced.
   */
  const_iterator end() const noexcept;

  friend std::unique_ptr<Clause> createHeapClause(size_type size);

private:
  /**
   * \brief Constructs a clause object of the given size.
   *
   * Note that objects are expected to be constructed within a preallocated
   * memory buffer of sufficient size.
   *
   * \param size  The clause's size.
   */
  Clause(size_type size) noexcept;

  uint32_t m_size;
  CNFLit m_anchor;
};

/**
 * \ingroup JamSAT_Solver
 *
 * \brief Computes the size of a non-empty Clause object.
 *
 * \param clauseSize The nonzero length of the clause.
 * \returns The size of a Clause object, in bytes.
 */
size_t getClauseAllocationSize(Clause::size_type clauseSize);

/**
 * \ingroup JamSAT_Solver
 *
 * \brief Allocates a clause of the given size on the heap.
 *
 * \param size The clause's size.
 * \returns A pointer to a new clause of size \p size, allocated on the heap.
 * Ownership of this object is transferred to the caller.
 */
std::unique_ptr<Clause> createHeapClause(Clause::size_type size);
}
