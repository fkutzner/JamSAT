#include <libjamsat/solver/Assignment.h>

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Casts.h>

namespace jamsat {
assignment::assignment(CNFVar max_var)
  : m_trail{max_var.getRawValue() + 1}
  , m_level_limits{}
  , m_assignments{max_var, TBools::INDETERMINATE}
  , m_phases{max_var, TBools::FALSE}
  , m_current_level{0}
  , m_reasons_and_als{max_var}
  , m_binaryWatchers{max_var}
  , m_watchers{max_var}
  , m_lits_with_required_watcher_update{getMaxLit(max_var)}
  , m_lits_with_required_watcher_update_as_vec{} {
    m_level_limits.push_back(0);
}

void assignment::clear() noexcept {}

void assignment::inc_max_var(CNFVar var) {
    JAM_ASSERT(var.getRawValue() + 1 >= m_assignments.size(), "Decreasing size not allowed");
    auto amnt_new_vars = var.getRawValue() + 1 - m_assignments.size();
    if (amnt_new_vars == 0) {
        return;
    }
    CNFVar const first_new_var{static_cast<CNFVar::RawVariable>(m_assignments.size())};

    m_trail.increaseMaxSizeBy(amnt_new_vars);
    m_assignments.increaseSizeTo(var);
    m_phases.increaseSizeTo(var);
    m_reasons_and_als.increaseSizeTo(var);
    m_binaryWatchers.increaseMaxVarTo(var);
    m_watchers.increaseMaxVarTo(var);
    m_lits_with_required_watcher_update.increaseSizeTo(getMaxLit(var));


    for (CNFVar i = first_new_var; i <= var; i = nextCNFVar(i)) {
        m_assignments[i] = TBools::INDETERMINATE;
        m_reasons_and_als[i].m_level = 0;
        m_reasons_and_als[i].m_reason = nullptr;
        m_phases[i] = TBools::FALSE;
    }
}

auto assignment::append(CNFLit literal) -> Clause* {
    m_trail.push_back(literal);

    TBool value = TBool::fromUnderlyingValue(static_cast<TBool::UnderlyingType>(literal.getSign()));
    m_assignments[literal.getVariable()] = value;
    m_reasons_and_als[literal.getVariable()].m_level = get_current_level();
    m_reasons_and_als[literal.getVariable()].m_reason = nullptr;

    // TODO: propagate assignment
    return nullptr;
}

void assignment::register_new_clause(Clause& clause) {
    // TODO: register with propagator
}

auto assignment::register_new_clause(Clause& clause, CNFLit asserting_lit) -> Clause* {
    // TODO: register with propagator
    return nullptr;
}

void assignment::register_clause_modification(Clause& clause) noexcept {
    // TODO: register with propagator
}

void assignment::new_level() noexcept {
    m_level_limits.push_back(static_checked_cast<level_limit>(m_trail.size()));
    ++m_current_level;
}

void assignment::undo_to_level(level level) noexcept {
    for (auto i = m_trail.begin() + m_level_limits[level + 1]; i != m_trail.end(); ++i) {
        m_phases[i->getVariable()] = m_assignments[(*i).getVariable()];
        m_assignments[i->getVariable()] = TBools::INDETERMINATE;
    }

    m_trail.pop_to(m_level_limits[level + 1]);
    m_level_limits.resize(level + 1);
    m_current_level = level;
}

auto assignment::get_level_assignments(level level) const noexcept -> assignment_range {
    if (level >= m_level_limits.size()) {
        return boost::make_iterator_range(m_trail.end(), m_trail.end());
    }

    auto begin = m_trail.begin() + m_level_limits[level];
    if (level + 1 == m_level_limits.size()) {
        return boost::make_iterator_range(begin, m_trail.end());
    }
    auto end = m_trail.begin() + m_level_limits[level + 1];
    return boost::make_iterator_range(begin, end);
}
}
