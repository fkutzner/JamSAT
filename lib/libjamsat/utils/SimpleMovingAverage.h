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
 * \file utils/SimpleMovingAverage.h
 * \brief Implementation of SimpleMovingAverage
 */

#pragma once

#include <boost/circular_buffer.hpp>

namespace jamsat {

/**
 * \ingroup JamSAT_Utils
 *
 * \class jamsat::SimpleMovingAverage
 *
 * \brief A data structure for computing simple moving averages.
 *
 * \tparam T        The type of the values to be averaged.
 * \tparam Average  The type of the mean value.
 */
template <typename T, typename Average = double>
class SimpleMovingAverage {
private:
    using Storage = boost::circular_buffer<T>;

public:
    using capacity_type = typename Storage::capacity_type;

    /**
     * \brief Constructs a SimpleMovingAverage instance with an empty sequence of
     *        elements.
     *
     * \param horizon   The maximum of elements the constructed instance takes
     *                  into account when computing mean values.
     */
    explicit SimpleMovingAverage(capacity_type horizon);

    /**
     * \brief Adds the given value to the sequence of elements whose mean value
     *        can be computed.
     *
     * \param value     The value to be added.
     */
    void add(T value) noexcept;

    /**
     * \brief Computes the simple moving average of the values previously passed
     *        to \p add() .
     *
     * Computes the unweighted mean of the last N elements passed to \p add() ,
     * where N is the minimum of the total number of calls to \p add() and the \p
     * horizon argument passed to the constructor. The result is computed by
     * summing up these elements, casting the sum to the Average type and finally
     * dividing by the number of elements taken into account.
     *
     * If no values have been passed yet to \p add() , this method returns 0.
     *
     * \returns The mean value as described above.
     */
    Average getAverage() const noexcept;

    /**
     * \brief Removes all elements.
     */
    void clear() noexcept;

    /**
     * \brief Determines whether the amount of elements currently taken into
     *        account has reached the instance's horizon.
     *
     * \returns true iff the amount of elements currently taken into account has
     *          reached the instance's horizon.
     */
    bool isFull() const noexcept;

private:
    Storage m_values;
    T m_currentSum;
};

/********** Implementation ****************************** */

template <typename T, typename Average>
SimpleMovingAverage<T, Average>::SimpleMovingAverage(capacity_type horizon)
  : m_values(horizon), m_currentSum(0) {}

template <typename T, typename Average>
void SimpleMovingAverage<T, Average>::add(T value) noexcept {
    if (m_values.capacity() == 0ull) {
        return;
    }

    if (m_values.full()) {
        m_currentSum -= m_values.front();
    }
    m_values.push_back(value);
    m_currentSum += value;
}

template <typename T, typename Average>
Average SimpleMovingAverage<T, Average>::getAverage() const noexcept {
    if (m_values.empty()) {
        return static_cast<Average>(0);
    }
    return static_cast<Average>(m_currentSum) / static_cast<Average>(m_values.size());
}

template <typename T, typename Average>
void SimpleMovingAverage<T, Average>::clear() noexcept {
    m_values.clear();
    m_currentSum = 0;
}

template <typename T, typename Average>
bool SimpleMovingAverage<T, Average>::isFull() const noexcept {
    return m_values.size() == m_values.capacity();
}
}
