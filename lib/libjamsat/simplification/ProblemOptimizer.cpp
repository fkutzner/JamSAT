#include "ProblemOptimizer.h"

namespace jamsat {


auto PolymorphicClauseDB::createClause(std::size_t size) noexcept -> Clause* {
    return m_impl->createClause(size);
}

void PolymorphicClauseDB::compress() noexcept {
    m_impl->compress();
}

void PolymorphicClauseDB::getClauses(ClauseRecv const& receiver) {
    m_impl->getClauses(receiver);
}

ProblemOptimizer& ProblemOptimizer::operator=(ProblemOptimizer&& rhs) noexcept {
    m_impl = std::move(rhs.m_impl);
    return *this;
}

ProblemOptimizer::ProblemOptimizer(ProblemOptimizer&& rhs) noexcept
  : m_impl{std::move(rhs.m_impl)} {}


SharedOptimizerState::SharedOptimizerState(std::vector<CNFLit>&& facts,
                                           PolymorphicClauseDB&& clauseDB,
                                           Assignment&& assignment,
                                           CNFVar maxVar) noexcept
  : m_facts{std::move(facts)}
  , m_clauseDB{std::move(clauseDB)}
  , m_assignment{std::move(assignment)}
  , m_maxVar{maxVar}
  , m_occMap{}
  , m_breakingChange{false}
  , m_detectedUnsat{false} {}

SharedOptimizerState::SharedOptimizerState(SharedOptimizerState&& rhs) noexcept
  : m_facts{std::move(rhs.m_facts)}
  , m_clauseDB{std::move(rhs.m_clauseDB)}
  , m_assignment{std::move(rhs.m_assignment)}
  , m_occMap{std::move(rhs.m_occMap)} {}

auto SharedOptimizerState::operator=(SharedOptimizerState&& rhs) noexcept -> SharedOptimizerState& {
    m_facts = std::move(rhs.m_facts);
    m_clauseDB = std::move(rhs.m_clauseDB);
    m_assignment = std::move(rhs.m_assignment);
    m_occMap = std::move(rhs.m_occMap);
    return *this;
}

void SharedOptimizerState::precomputeOccurrenceMap() {
    m_occMap = OccMap{getMaxLit(m_maxVar)};
    m_clauseDB.getClauses([this](std::vector<Clause*> const& clauses) {
        for (Clause* clause : clauses) {
            m_occMap->insert(*clause);
        }
    });
}

auto SharedOptimizerState::hasPrecomputedOccurrenceMap() const noexcept -> bool {
    return m_occMap.has_value();
}

auto SharedOptimizerState::hasDetectedUnsat() const noexcept -> bool {
    return m_detectedUnsat;
}

void SharedOptimizerState::setDetectedUnsat() noexcept {
    m_detectedUnsat = true;
}

auto SharedOptimizerState::hasBreakingChange() const noexcept -> bool {
    return m_breakingChange;
}

void SharedOptimizerState::setBreakingChange() noexcept {
    m_breakingChange = true;
}

auto SharedOptimizerState::release() noexcept
    -> std::tuple<std::vector<CNFLit>, PolymorphicClauseDB, Assignment> {
    return make_tuple(std::move(m_facts), std::move(m_clauseDB), std::move(m_assignment));
}

}