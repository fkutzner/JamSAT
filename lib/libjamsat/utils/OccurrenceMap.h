/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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
 * \file utils/OccurrenceMap.h
 * \brief Implementation of OccurrenceMap
 */

#pragma once

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/BoundedMap.h>
#include <libjamsat/utils/TraitUtils.h>

#include <boost/range.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace jamsat {
/**
 * \class OccurrenceMap
 *
 * \brief A map-like type for keeping track of element occurrences in containers.
 *
 * \ingroup JamSAT_Utils
 *
 * \tparam ContainerT               A type partially satisfying the STL `Container` concept:
 *                                  For objects `c` of type `ContainerT`, the expressions
 *                                  `c.begin()` and `c.end()` must be valid, with the same
 *                                  semantics as defined by the STL `Container` concept.
 *                                  `Container::value_type` must be the type of the objects
 *                                  contained in instances of `ContainerT`.
 * \tparam ContainerDeletedQuery    A type satisfying the STL UnaryPredicate concept for
 *                                  `Container const*` arguments, with instances of
 *                                  `ContainerDeletedQuery`indicating whether the container
 *                                  has been marked for deletion.
 * \tparam ContainerModifiedQuery   A type satisfying the STL UnaryPredicate concept for
 *                                  `Container const*` arguments, with instances `qz of
 *                                  `ContainerDeletedQuery`indicating whether the container
 *                                  has been modified since its addition or the last call
 *                                  to setModified(). `q.clearModified(c)` must be a valid
 *                                  expression for `Container&` objects `c`. When the
 *                                  occurrence map has been informed about the modification
 *                                  of `c`, `q.clearModified(c)` will eventually be
 *                                  called.
 * \tparam ContainerValueIndex      A type that is a model of the concept `Index` with indexed
 *                                  type `Container::value_type`.
 */
template <typename ContainerT,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex = typename ContainerT::value_type::Index>
class OccurrenceMap {
  static_assert(is_index<ContainerValueIndex, typename ContainerT::value_type>::value,
                "ContainerValueIndex must satisfy Index for ContainerT::value_type, but does not");

public:
  struct OccurrenceMapTypeMarker {
  };

  using Container = ContainerT;
  using ContainerRange = boost::iterator_range<typename std::vector<Container*>::const_iterator>;
  using value_type = typename ContainerT::value_type;

  /**
   * \brief Constructs an OccurrenceMap
   *
   * \param maxElement    The maximum element that may occur in the containers
   *                      added to the OccurrenceMap.
   */
  explicit OccurrenceMap(value_type maxElement);

  /**
   * \brief Constructs an OccurrenceMap, inserting a range of elements
   *
   * \tparam ContainerPtrIt   A type being a model of `Iterator`. The value type of
   *                          `ContainerPtrIt` must be convertible to `Container *const`.
   *
   * \param maxElement    The maximum element that may occur in the containers
   *                      added to the OccurrenceMap.
   * \param begin         The begin of the range of pointers to containers to be added.
   * \param end           The end of the range of pointers to containers to be added.
   */
  template <typename ContainerPtrIt>
  OccurrenceMap(value_type maxElement, ContainerPtrIt begin, ContainerPtrIt end);


  ~OccurrenceMap();

  /**
   * \brief Increases the maximum element that may occur in the containers added
   *        to the OccurrenceMap
   *
   * \param maxElement    The maximum element that may occur in the containers
   *                      added to the OccurrenceMap.
   */
  void increaseMaxElementTo(value_type maxElement);

  /**
   * \brief Adds a container to the occurrence map.
   *
   * \param container     The container to be added.
   */
  void insert(Container& container);

  /**
   * \brief Adds containers to the occurrence map.
   *
   * \tparam ContainerPtrIt   A type being a model of `Iterator`. The value type of
   *                          `ContainerPtrIt` must be convertible to `Container *const`.
   * \param begin     The begin of the range of pointers to containers to be added.
   * \param end       The end of the range of pointers to containers to be added.
   */
  template <typename ContainerPtrIt>
  void insert(ContainerPtrIt begin, ContainerPtrIt end);

  /**
   * \brief Returns a range of pointers to the containers in which \p index occurs.
   *
   * \param value         A value not greater than the current maximum element.
   * \returns             A range of pointers to the containers in which \p index
   *                      occurs.
   */
  auto operator[](value_type value) noexcept -> ContainerRange;

  /**
   * \brief Marks a container as to-be-deleted from the occurrence map.
   *
   * Unless elements have been removed from \p container, \p container will not be
   * returned in future calls to `operator[]()`.
   *
   * Precondition: \p container must be marked as to-be-deleted by all
   * `ContainerDeletedQuery` objects.
   *
   * \param container     The container to be marked as to-be-deleted from the
   *                      occurrence map.
   */
  void remove(Container const& container) noexcept;

  /**
   * \brief Informs the occurrence map that the given container has been modified.
   *
   * Precondition: \p container must be marked as to-be-deleted by all
   * `ContainerModifiedQuery` objects.
   * 
   * \param container     The container to be marked as to-be-deleted from the
   *                      occurrence map.
   * 
   * \param additions     Elements added to \p{container}.
   * 
   * \param removals      Elements removed from \p{container}. \p{removals} and
   *                      \p{additions} must be disjoint.
   * 
   * \tparam IterableA    A type supporting iteration over `value_type` objects
   *                      via begin() and end() methods.
   * \tparam IterableB    A type supporting iteration over `value_type` objects
   *                      via begin() and end() methods.
   */
  template <typename IterableA, typename IterableR>
  void setModified(Container& container, IterableA const& additions, IterableR const& removals);


  /**
   * \brief Resolves all cleanups neccessary due to container modifications and
   *   invokes the rsp. clearModified() function of the `ContainerModifiedQuery`
   *   object for all clauses modified since the last `resolveModifications()`
   *   call.
   */
  void resolveModifications();

  /**
   * \brief Removes all elements from the occurrence map.
   */
  void clear() noexcept;

  OccurrenceMap(OccurrenceMap const& rhs) = delete;
  auto operator=(OccurrenceMap const& rhs) -> OccurrenceMap& = delete;

  OccurrenceMap(OccurrenceMap&& rhs) noexcept = default;
  auto operator=(OccurrenceMap&& rhs) noexcept -> OccurrenceMap& = default;

private:
  void update(value_type const& value) noexcept;

  using OccList = std::vector<Container*>;

  struct OccurrenceListWithFlags {
    bool m_requiresUpdate;
    OccList m_occList;
  };

  BoundedMap<value_type, OccurrenceListWithFlags, ContainerValueIndex> m_occurrences;
  ContainerDeletedQuery m_deletedQuery;
  ContainerModifiedQuery m_modifiedQuery;

  // Container caching elements requiring updates due to modifications of containers.
  // This map is used to clear "modified" flags from containers as soon as possible,
  // and also to avoid iterating over large containers too frequently.
  std::unordered_map<Container*, std::unordered_set<value_type>> m_delModUpdates;
};


/**
 * \brief Metafunction checking whether a given type is a OccurrenceMap<...> specialization supporting
 *        looking up objects of a given type.
 *
 * \ingroup JamSAT_Utils
 *
 * \tparam OccMapT      An arbitrary type.
 * \tparam OccT         An arbitrary type.
 *
 * `is_occurrence_map<OccMapT, OccT>::value` is a `constexpr bool` value which is `true`
 * iff \p OccMapT is a \p OccurrenceMap specialization supporting looking up objects of type `OccT`.
 */
template <typename OccMapT, typename OccT, typename = void>
struct is_occurrence_map : public std::false_type {
};


// clang-format off
template<typename OccMapT, typename OccT>
struct is_occurrence_map<OccMapT,
                         OccT,
                         j_void_t<typename OccMapT::OccurrenceMapTypeMarker,
                                  std::enable_if_t<std::is_same<typename OccMapT::value_type, OccT>::value, void>>>
  : public std::true_type {};
// clang-format on

/********** Implementation ****************************** */

template <typename ContainerT,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
OccurrenceMap<ContainerT, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    OccurrenceMap(value_type maxElement)
  : m_occurrences(maxElement), m_deletedQuery()
{
}


template <typename ContainerT,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
template <typename ContainerPtrIt>
OccurrenceMap<ContainerT, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    OccurrenceMap(value_type maxElement, ContainerPtrIt begin, ContainerPtrIt end)
  : m_occurrences(maxElement), m_deletedQuery()
{
  insert(begin, end);
}

template <typename ContainerT,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
void OccurrenceMap<ContainerT, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    increaseMaxElementTo(value_type maxElement)
{
  m_occurrences.increaseSizeTo(maxElement);
}

template <typename ContainerT,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
void OccurrenceMap<ContainerT, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    insert(Container& container)
{
  if (m_deletedQuery(&container)) {
    return;
  }
  for (auto& element : container) {
    m_occurrences[element].m_occList.push_back(&container);
  }
}

template <typename ContainerT,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
template <typename ContainerPtrIt>
void OccurrenceMap<ContainerT, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    insert(ContainerPtrIt begin, ContainerPtrIt end)
{
  while (begin != end) {
    insert(**begin);
    ++begin;
  }
}

template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    remove(Container const& container) noexcept
{
  JAM_ASSERT(m_deletedQuery(&container),
             "Only remove containers marked for deletion may be deleted");
  for (auto& element : container) {
    m_occurrences[element].m_requiresUpdate = true;
  }
}


template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
template <typename IterableA, typename IterableR>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    setModified(Container& container, IterableA const& additions, IterableR const& removals)
{

  JAM_ASSERT(m_modifiedQuery(&container),
             "Only remove containers marked for modification may be passed to setModified()");

  bool const hasNewRemovals = (std::distance(removals.begin(), removals.end()) > 0);

  auto delModUpdateSetIter = m_delModUpdates.find(&container);
  if (delModUpdateSetIter != m_delModUpdates.end()) {
    // Need to take previous removals into account:
    // - only add to occurrence lists if there is no removal pending on that list
    // - remove pending removals for elements that are added again

    std::unordered_set<value_type>& delModUpdateSet = delModUpdateSetIter->second;
    for (value_type const& element : additions) {
      if (delModUpdateSet.find(element) != delModUpdateSet.end()) {
        delModUpdateSet.erase(element);
      }
      else {
        m_occurrences[element].m_occList.push_back(&container);
      }
    }

    if (delModUpdateSet.empty() && !hasNewRemovals) {
      m_delModUpdates.erase(&container);
      m_modifiedQuery.clearModified(container);
    }
  }
  else {
    for (value_type const& element : additions) {
      m_occurrences[element].m_occList.push_back(&container);
    }
    if (!hasNewRemovals) {
      // Only performing additions ~> clear modified flag
      m_modifiedQuery.clearModified(container);
    }
  }

  if (hasNewRemovals) {
    m_delModUpdates[&container].insert(removals.begin(), removals.end());
    for (auto const& element : removals) {
      m_occurrences[element].m_requiresUpdate = true;
    }
  }
}

template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    resolveModifications()
{

  std::vector<value_type> toUpdate;
  std::vector<Container*> toClear;

  for (auto modUpdateList : m_delModUpdates) {
    for (auto const& element : modUpdateList.second) {
      toUpdate.push_back(element);
    }
    toClear.push_back(modUpdateList.first);
  }

  for (value_type const& element : toUpdate) {
    if (m_occurrences[element].m_requiresUpdate) {
      update(element);
    }
  }

  for (Container* c : toClear) {
    m_modifiedQuery.clearModified(*c);
  }

  JAM_ASSERT(m_delModUpdates.empty(),
             "Elements requiring updates remain after resolution of all modifications");
}

template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
auto OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
operator[](value_type value) noexcept -> ContainerRange
{
  if (m_occurrences[value].m_requiresUpdate) {
    update(value);
  }
  OccList& occList = m_occurrences[value].m_occList;
  return boost::make_iterator_range(occList.cbegin(), occList.cend());
}

template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    clear() noexcept
{
  for (auto& x : m_occurrences.values()) {
    x.m_occList.clear();
  }
}

template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
void OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    update(value_type const& value) noexcept
{
  OccList& occList = m_occurrences[value].m_occList;
  auto const removePred = [this, value](Container* c) {
    if (m_deletedQuery(c)) {
      return true;
    }

    if (m_modifiedQuery(c)) {
      auto delModUpdateSetIter = m_delModUpdates.find(c);
      if (delModUpdateSetIter != m_delModUpdates.end()) {
        std::unordered_set<value_type>& delModUpdateSet = delModUpdateSetIter->second;
        if (delModUpdateSet.find(value) != delModUpdateSet.end()) {
          delModUpdateSet.erase(value);
          if (delModUpdateSet.empty()) {
            m_modifiedQuery.clearModified(*c);
            m_delModUpdates.erase(c);
          }
          return true;
        }
      }
    }

    return false;
  };

  boost::remove_erase_if(occList, removePred);
  m_occurrences[value].m_requiresUpdate = false;
}

template <typename Container,
          typename ContainerDeletedQuery,
          typename ContainerModifiedQuery,
          typename ContainerValueIndex>
OccurrenceMap<Container, ContainerDeletedQuery, ContainerModifiedQuery, ContainerValueIndex>::
    ~OccurrenceMap()
{
  for (auto modUpdateList : m_delModUpdates) {
    m_modifiedQuery.clearModified(*modUpdateList.first);
  }
}
}
