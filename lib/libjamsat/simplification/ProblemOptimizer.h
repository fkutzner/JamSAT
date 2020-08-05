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

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Assignment.h>
#include <libjamsat/solver/Statistics.h>
#include <libjamsat/utils/OccurrenceMap.h>

#include <boost/optional.hpp>


namespace jamsat {

class PolymorphicClauseDB final {
public:
    using ClauseRecv = std::function<void(std::vector<Clause*> const&)>;
    using ConstClauseRecv = std::function<void(std::vector<Clause const*> const&)>;

private:
    class Base {
    public:
        virtual auto createClause(std::size_t size) noexcept -> Clause* = 0;
        virtual void compress() noexcept = 0;
        virtual void getClauses(ClauseRecv const& receiver) = 0;

        virtual ~Base() = default;
    };

    template <typename T>
    class Impl : public Base {
    public:
        explicit Impl(T&&) noexcept;

        auto createClause(std::size_t size) noexcept -> Clause* override;
        void compress() noexcept override;
        virtual void getClauses(ClauseRecv const& receiver) override;

        T release() noexcept;

        virtual ~Impl() = default;

    private:
        T m_impl;
    };

public:
    template <typename T>
    PolymorphicClauseDB(T&& clauseDB);

    template <typename T>
    auto release() -> T;

    auto createClause(std::size_t size) noexcept -> Clause*;
    void compress() noexcept;
    void getClauses(ClauseRecv const& receiver);

    auto operator=(PolymorphicClauseDB const&) -> PolymorphicClauseDB& = delete;
    PolymorphicClauseDB(PolymorphicClauseDB const&) = delete;
    auto operator=(PolymorphicClauseDB &&) -> PolymorphicClauseDB& = default;
    PolymorphicClauseDB(PolymorphicClauseDB&&) = default;

private:
    std::unique_ptr<Base> m_impl;
};


class SharedOptimizerState final {
private:
    class ClauseDeletedQuery {
    public:
        auto operator()(Clause const* cl) const noexcept -> bool {
            return cl->getFlag(Clause::Flag::SCHEDULED_FOR_DELETION);
        }
    };

    class ClauseModifiedQuery {
    public:
        auto operator()(Clause const* cl) const noexcept -> bool {
            return cl->getFlag(Clause::Flag::MODIFIED);
        }

        void clearModified(Clause& cl) const noexcept { cl.clearFlag(Clause::Flag::MODIFIED); }
    };

public:
    SharedOptimizerState(std::vector<CNFLit>&& facts,
                         PolymorphicClauseDB&& clauseDB,
                         Assignment&& assignment,
                         CNFVar maxVar) noexcept;


    auto getFacts() noexcept -> std::vector<CNFLit>&;
    auto getFacts() const noexcept -> std::vector<CNFLit> const&;

    auto getClauseDB() noexcept -> PolymorphicClauseDB&;
    auto getClauseDB() const noexcept -> PolymorphicClauseDB const&;

    auto getAssignment() noexcept -> Assignment&;
    auto getAssignment() const noexcept -> Assignment const&;

    using OccMap = OccurrenceMap<Clause, ClauseDeletedQuery, ClauseModifiedQuery>;
    auto getOccurrenceMap() noexcept -> OccMap&;
    auto getOccurrenceMap() const noexcept -> OccMap const&;

    void precomputeOccurrenceMap();
    auto hasPrecomputedOccurrenceMap() const noexcept -> bool;

    auto getMaxVar() const noexcept -> CNFVar;
    void setMaxVar(CNFVar var) noexcept;

    auto hasDetectedUnsat() const noexcept -> bool;
    void setDetectedUnsat() noexcept;

    auto hasBreakingChange() const noexcept -> bool;
    void setBreakingChange() noexcept;

    auto getStats() noexcept -> OptimizationStats&;
    auto getStats() const noexcept -> OptimizationStats const&;

    auto release() noexcept -> std::tuple<std::vector<CNFLit>, PolymorphicClauseDB, Assignment>;

    auto operator=(SharedOptimizerState const&) -> SharedOptimizerState& = delete;
    SharedOptimizerState(SharedOptimizerState const&) = delete;

    auto operator=(SharedOptimizerState&&) noexcept -> SharedOptimizerState&;
    SharedOptimizerState(SharedOptimizerState&&) noexcept;

private:
    std::vector<CNFLit> m_facts;
    PolymorphicClauseDB m_clauseDB;
    Assignment m_assignment;
    CNFVar m_maxVar;

    boost::optional<OccMap> m_occMap;

    bool m_breakingChange;
    bool m_detectedUnsat;

    OptimizationStats m_stats;
};

class ProblemOptimizer {
public:
    virtual auto wantsExecution(StatisticsEra const& currentStats) const noexcept -> bool = 0;
    virtual auto optimize(SharedOptimizerState sharedOptimizerState,
                          StatisticsEra const& currentStats) -> SharedOptimizerState = 0;
    virtual ~ProblemOptimizer() = default;
};


// Inline implementations

template <typename T>
PolymorphicClauseDB::PolymorphicClauseDB(T&& clauseDB)
  : m_impl{std::make_unique<Impl<T>>(std::move(clauseDB))} {}

template <typename T>
auto PolymorphicClauseDB::release() -> T {
    Impl<T>* impl = dynamic_cast<Impl<T>*>(m_impl.get());
    if (impl == nullptr) {
        throw std::invalid_argument{"PolymorphicClauseDB::release(): invalid type"};
    }
    return impl->release();
}

template <typename T>
PolymorphicClauseDB::Impl<T>::Impl(T&& clauseDB) noexcept : m_impl{std::move(clauseDB)} {}


template <typename T>
auto PolymorphicClauseDB::Impl<T>::createClause(std::size_t size) noexcept -> Clause* {
    return m_impl.createClause(size).value_or(nullptr);
}

template <typename T>
void PolymorphicClauseDB::Impl<T>::compress() noexcept {
    m_impl.compress();
}

template <typename T>
void PolymorphicClauseDB::Impl<T>::getClauses(ClauseRecv const& receiver) {
    auto clauses = m_impl.getClauses();
    auto cursor = clauses.begin();

    std::size_t const bufSize = 1024 * 10;
    std::vector<Clause*> buffer;
    buffer.reserve(bufSize);

    while (cursor != clauses.end()) {
        while (cursor != clauses.end() && buffer.size() < bufSize) {
            buffer.push_back(&(*cursor));
            ++cursor;
        }
        receiver(buffer);
        buffer.clear();
    }
}


template <typename T>
auto PolymorphicClauseDB::Impl<T>::release() noexcept -> T {
    return std::move(m_impl);
}

inline auto SharedOptimizerState::getFacts() noexcept -> std::vector<CNFLit>& {
    return m_facts;
}

inline auto SharedOptimizerState::getFacts() const noexcept -> std::vector<CNFLit> const& {
    return m_facts;
}

inline auto SharedOptimizerState::getClauseDB() noexcept -> PolymorphicClauseDB& {
    return m_clauseDB;
}

inline auto SharedOptimizerState::getClauseDB() const noexcept -> PolymorphicClauseDB const& {
    return m_clauseDB;
}

inline auto SharedOptimizerState::getAssignment() noexcept -> Assignment& {
    return m_assignment;
}

inline auto SharedOptimizerState::getAssignment() const noexcept -> Assignment const& {
    return m_assignment;
}

inline auto SharedOptimizerState::getOccurrenceMap() noexcept -> OccMap& {
    if (!m_occMap.has_value()) {
        precomputeOccurrenceMap();
    }
    return *m_occMap;
}

inline auto SharedOptimizerState::getOccurrenceMap() const noexcept -> OccMap const& {
    if (!m_occMap.has_value()) {
        const_cast<SharedOptimizerState*>(this)->precomputeOccurrenceMap();
    }
    return *m_occMap;
}

inline auto SharedOptimizerState::getMaxVar() const noexcept -> CNFVar {
    return m_maxVar;
}

inline void SharedOptimizerState::setMaxVar(CNFVar var) noexcept {
    m_maxVar = var;
}
}