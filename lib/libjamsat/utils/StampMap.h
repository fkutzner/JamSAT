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

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Concepts.h>
#include <libjamsat/utils/ControlFlow.h>
#include <libjamsat/utils/TraitUtils.h>

#include <limits>
#include <type_traits>
#include <vector>

namespace jamsat {
/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::StampMapBase
 *
 * \brief The base class template for StampMap.
 *
 * Implementation details common to all StampMap specializations/instantiations
 * are defined in this class.
 *
 * \tparam T    An integral type used as the internal index type, e.g. uint32_t.
 *              Stamped elements are mapped to elements of type T, which in turn is used as
 *              an internal index to the stamping data storage. Using narrower types T leads to
 *              improved cache efficiency, but also requires the internal stamping data
 *              storage to be cleaned completely more frequently.
 */
template <typename T>
class StampMapBase {
public:
    using size_type = typename std::vector<T>::size_type;

    class StampingContext;

    /**
     * \class Stamp
     *
     * \brief The stamp data type.
     *
     * Usage example: mark elements using a StampMap and a stamp.
     */
    struct Stamp {
        T value;
    };

    /**
     * \class StampingContext
     *
     * \brief RAII-style context for StampMaps, on destruction clearing the stamps
     *        made since creating the context.
     *
     * Usage example: create a stamping context before beginning to use a stamp
     * map, stamp elements using the stamp obtained from the context, and have the
     * stamps cleared when the context is destroyed.
     */
    class StampingContext {
    public:
        /**
         * \brief Returns the context's stamp.
         *
         * \returns the context's stamp.
         */
        Stamp getStamp() const noexcept { return m_stamp; }

        StampingContext& operator=(const StampingContext& other) = delete;
        StampingContext& operator=(StampingContext&& other) = default;
        StampingContext(const StampingContext& other) = delete;
        StampingContext(StampingContext&& other) = default;

    private:
        friend class StampMapBase<T>;
        StampingContext(StampMapBase& m_origin, Stamp stamp) noexcept
          : m_clearStamps([&m_origin]() { m_origin.clear(); }), m_stamp(stamp) {}

        OnExitScope m_clearStamps;
        Stamp m_stamp;
    };

    /**
     * \brief Creates a stamping context.
     *
     * A stamping context provides a stamp with which items can be stamped, and
     * takes care of clearing the stamps set by the user. For each stamp map, at
     * most one stamping context may exist at a time.
     *
     * \returns a stamping context.
     */
    StampingContext createContext() noexcept;

    /**
     * \brief Increases the maximum internal index which can be stored in the stamp map.
     *
     * \param maxIdx    The new maximum internal index as described above. Must at least be as large
     *                  as the current maximum internal index.
     */
    void increaseSizeTo(size_type maxIdx);

protected:
    /**
     * \brief Constructs a StampMapBase instance.
     *
     * \param maxIdx    The maximum internal index which can be stored in the stamp
     * map.
     */
    explicit StampMapBase(size_type maxIdx);

    void clear() noexcept;

    std::vector<T> m_stamps;
    T m_currentStamp;
    bool m_contextActive;
};

template <typename T, typename... Ks>
class StampMap;

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::StampMap<T>
 *
 * \brief The vacuous StampMap<T, K...> template base case class.
 */
template <typename T>
class StampMap<T> : public StampMapBase<T> {
public:
    /**
     * \brief Constructs a StampMap<T> instance.
     *
     * \param maxIdx    The maximum internal index which can be stored in the stamp
     *                  map.
     */
    explicit StampMap<T>(typename StampMapBase<T>::size_type maxIdx) : StampMapBase<T>(maxIdx) {}

    /**
     * \brief The StampMap's internal index type.
     */
    using InternalIndexType = T;

protected:
    // TODO: refactor the inheritance hierarchy to remove these method declarations
    void setStamped();
    bool isStamped();
};

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::StampMap<T, K...>
 *
 * \brief A map for flagging values ("stamping"), with an efficient flag
 *        clearing mechanism.
 *
 * StampMap allows type-safe stamping of objects of different type.
 *
 * Usage example: Mark object in an algorithm with O(1) operations using a
 * StampMap, clearing the markers after the algorithm executed efficiently,
 * allowing the StampMap to be reused later.
 *
 * \tparam T    An integral type used as the internal index type, e.g. uint32_t.
 *              Stamped elements are mapped to elements of type T, which in turn is used as
 *              an internal index to the stamping data storage. Using narrower types T leads to
 *              improved cache efficiency, but also requires the internal stamping data
 *              storage to be cleaned completely more frequently.
 * \tparam K    A type satisfying the concept `Index`
 * \tparam Ks   A sequence of types satisfying the concept `Index`
 */
template <typename T, typename K, typename... Ks>
class StampMap<T, K, Ks...> : public StampMap<T, Ks...> {
    static_assert(is_index<K, typename K::Type>::value, "K must satisfy Index, but does not");

public:
    /**
     * \brief Constructs a StampMap<T, K, Ks...> instance.
     *
     * \param maxIdx    The maximum internal index which can be stored in the
     *                  stamp map.
     */
    explicit StampMap<T, K, Ks...>(typename StampMapBase<T>::size_type maxIdx)
      : StampMap<T, Ks...>(maxIdx) {}

    /**
     * \brief Stamps or unstamps a given object.
     *
     * \param obj       The object to be marked as stamped/not-stamped. \p
     *                  K::getIndex(obj) must not be greater than the maximum index \p maxIdx
     *                  passed to the constructor of the stamp map.
     * \param stamp     The current stamp (obtained from a StampingContext instance).
     * \param stamped   true iff \p obj should be marked as stamped.
     */
    void setStamped(const typename K::Type& obj,
                    typename StampMapBase<T>::Stamp stamp,
                    bool stamped) noexcept;

    using StampMap<T, Ks...>::setStamped;

    /**
     * \brief Determines if the given object is stamped.
     *
     * \param obj       An object. \p K::getIndex(obj) must not be greater than the
     *                  maximum index \p maxIdx passed to the constructor of the stamp map.
     * \param stamp     The current stamp (obtained from a StampingContext instance).
     * \returns         true iff \p index is marked as stamped.
     */
    bool isStamped(const typename K::Type& obj, const typename StampMapBase<T>::Stamp stamp) const
        noexcept;

    using StampMap<T, Ks...>::isStamped;
};

/**
 * \brief Metafunction checking whether a given type is a StampMap<...> specialization supporting
 *        stamping objects of a given type.
 *
 * \ingroup JamSAT_Utils
 *
 * \tparam StampMapT    An arbitrary type.
 * \tparam S            An arbitrary type.
 *
 * `is_stamp_map<StampMapT, S>::value` is a `constexpr bool` value which is `true`
 * iff \p StampMapT is a \p StampMap specialization supporting stamping objects of type \p S.
 */
template <typename StampMapT, typename S, typename = j_void_t<>>
struct is_stamp_map : public std::false_type {};

// clang-format off
template<typename StampMapT, typename S>
struct is_stamp_map<StampMapT, S, j_void_t<
        std::is_base_of<StampMapBase<typename StampMapT::InternalIndexType>, StampMapT>,
        JAM_REQUIRE_EXPR(std::declval<StampMapT>().setStamped(std::declval<S>(),
                                                              std::declval<typename StampMapT::Stamp>(),
                                                              std::declval<bool>()),
                         void),
        JAM_REQUIRE_EXPR(std::declval<StampMapT>().isStamped(std::declval<S>(),
                                                             std::declval<typename StampMapT::Stamp>()),
                         bool)
        >> : public std::true_type {};

// clang-format on

/********** Implementation ****************************** */

template <typename T>
StampMapBase<T>::StampMapBase(typename StampMapBase<T>::size_type maxIdx)
  : m_stamps(maxIdx + 1), m_currentStamp(T{} + 1), m_contextActive(false) {}

template <typename T>
typename StampMapBase<T>::StampingContext StampMapBase<T>::createContext() noexcept {
    JAM_ASSERT(!m_contextActive, "StampMap does not support concurrent contexts");
    m_contextActive = true;
    return StampingContext(*this, StampMapBase<T>::Stamp{m_currentStamp});
}

template <typename T>
void StampMapBase<T>::increaseSizeTo(typename StampMapBase<T>::size_type maxIdx) {
    JAM_ASSERT((maxIdx + 1) >= m_stamps.size(), "The size of StampMaps can only be increased");
    m_stamps.resize(maxIdx + 1, std::numeric_limits<T>::min());
}

template <typename T>
void StampMapBase<T>::clear() noexcept {
    if (m_currentStamp == std::numeric_limits<T>::max()) {
        auto clearValue = std::numeric_limits<T>::min();
        for (auto& x : m_stamps) {
            x = clearValue;
        }
        m_currentStamp = clearValue;
    }
    ++m_currentStamp;
    m_contextActive = false;
}

template <typename T, typename K, typename... Ks>
void StampMap<T, K, Ks...>::setStamped(const typename K::Type& obj,
                                       typename StampMapBase<T>::Stamp stamp,
                                       bool stamped) noexcept {
    JAM_ASSERT(stamp.value == StampMapBase<T>::m_currentStamp, "Invalid stamp");
    auto index = K::getIndex(obj);
    JAM_ASSERT(index < StampMapBase<T>::m_stamps.size(), "Index out of bounds");
    auto minStamp = std::numeric_limits<T>::min();
    StampMapBase<T>::m_stamps[index] = stamped ? stamp.value : minStamp;
}

template <typename T, typename K, typename... Ks>
bool StampMap<T, K, Ks...>::isStamped(const typename K::Type& obj,
                                      const typename StampMapBase<T>::Stamp stamp) const noexcept {
    JAM_ASSERT(stamp.value == StampMapBase<T>::m_currentStamp, "Invalid stamp");
    auto index = K::getIndex(obj);
    JAM_ASSERT(index < StampMapBase<T>::m_stamps.size(), "Index out of bounds");
    return StampMapBase<T>::m_stamps[index] == stamp.value;
}
}
