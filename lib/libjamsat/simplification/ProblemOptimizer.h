#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <libjamsat/clausedb/Clause.h>
#include <libjamsat/cnfproblem/CNFLiteral.h>
#include <libjamsat/solver/Assignment.h>
#include <libjamsat/utils/OccurrenceMap.h>

#include <boost/optional.hpp>

namespace jamsat {

class SharedOptimizerState {
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
                         std::vector<Clause*>&& clauses,
                         Assignment&& assignment,
                         CNFVar maxVar) noexcept;


    auto getFacts() noexcept -> std::vector<CNFLit>&;
    auto getFacts() const noexcept -> std::vector<CNFLit> const&;

    auto getClauses() noexcept -> std::vector<Clause*>&;
    auto getClauses() const noexcept -> std::vector<Clause*> const&;

    auto getAssignment() noexcept -> Assignment&;
    auto getAssignment() const noexcept -> Assignment const&;

    using OccMap = OccurrenceMap<Clause, ClauseDeletedQuery, ClauseModifiedQuery>;
    auto getOccurrenceMap() noexcept -> OccMap&;
    auto getOccurrenceMap() const noexcept -> OccMap const&;

    void precomputeOccurrenceMap() const;
    auto hasPrecomputedOccurrenceMap() const noexcept -> bool;

    auto getMaxVar() const noexcept -> CNFVar;
    void setMaxVar(CNFVar var) noexcept;

    auto hasDetectedUnsat() const noexcept -> bool;
    void setDetectedUnsat() noexcept;

    auto hasBreakingChange() const noexcept -> bool;
    void setBreakingChange() noexcept;

    auto release() noexcept -> std::tuple<std::vector<CNFLit>, std::vector<Clause*>, Assignment>;

    auto operator=(SharedOptimizerState const&) -> SharedOptimizerState& = delete;
    SharedOptimizerState(SharedOptimizerState const&) = delete;

    auto operator=(SharedOptimizerState&&) noexcept -> SharedOptimizerState&;
    SharedOptimizerState(SharedOptimizerState&&) noexcept;

private:
    std::vector<CNFLit> m_facts;
    std::vector<Clause*> m_clauses;
    Assignment m_assignment;
    CNFVar m_maxVar;

    mutable boost::optional<OccMap> m_occMap;

    bool m_breakingChange;
    bool m_detectedUnsat;
};

class ProblemOptimizer {
private:
    class Base {
    public:
        virtual auto wantsExecution(uint64_t conflictsSinceInvocation) const noexcept -> bool = 0;
        virtual auto optimize(SharedOptimizerState sharedOptimizerState)
            -> SharedOptimizerState = 0;
        virtual ~Base() = default;
    };

    template <typename T>
    class Impl final : public Base {
    public:
        Impl(T&& t);
        auto wantsExecution(uint64_t conflictsSinceInvocation) const noexcept -> bool override;
        auto optimize(SharedOptimizerState sharedOptimizerState) -> SharedOptimizerState override;

        T m_impl;
    };

public:
    template <typename T>
    ProblemOptimizer(T&& optimizer);

    auto wantsExecution(uint64_t conflictsSinceInvocation) const noexcept -> bool;
    auto optimize(SharedOptimizerState sharedOptimizerState) -> SharedOptimizerState;

    auto operator=(ProblemOptimizer const&) -> ProblemOptimizer& = delete;
    ProblemOptimizer(ProblemOptimizer const&) = delete;

    auto operator=(ProblemOptimizer&&) noexcept -> ProblemOptimizer&;
    ProblemOptimizer(ProblemOptimizer&&) noexcept;

private:
    std::unique_ptr<Base> m_impl;
};


// Inline implementations

template <typename T>
ProblemOptimizer::ProblemOptimizer(T&& optimizer)
  : m_impl{std::make_unique<Impl<T>>(std::move(optimizer))} {}

template <typename T>
ProblemOptimizer::Impl<T>::Impl(T&& t) : m_impl{std::move(t)} {};

template <typename T>
auto ProblemOptimizer::Impl<T>::wantsExecution(uint64_t conflictsSinceInvocation) const noexcept
    -> bool {
    return m_impl.wantsExecution(conflictsSinceInvocation);
}

template <typename T>
auto ProblemOptimizer::Impl<T>::optimize(SharedOptimizerState sharedOptimizerState)
    -> SharedOptimizerState {
    return m_impl.optimize(std::move(sharedOptimizerState));
}

inline auto ProblemOptimizer::wantsExecution(uint64_t conflictsSinceInvocation) const noexcept
    -> bool {
    return m_impl->wantsExecution(conflictsSinceInvocation);
}

inline auto ProblemOptimizer::optimize(SharedOptimizerState sharedOptimizerState)
    -> SharedOptimizerState {
    return m_impl->optimize(std::move(sharedOptimizerState));
}


inline auto SharedOptimizerState::getFacts() noexcept -> std::vector<CNFLit>& {
    return m_facts;
}

inline auto SharedOptimizerState::getFacts() const noexcept -> std::vector<CNFLit> const& {
    return m_facts;
}

inline auto SharedOptimizerState::getClauses() noexcept -> std::vector<Clause*>& {
    return m_clauses;
}

inline auto SharedOptimizerState::getClauses() const noexcept -> std::vector<Clause*> const& {
    return m_clauses;
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
        precomputeOccurrenceMap();
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