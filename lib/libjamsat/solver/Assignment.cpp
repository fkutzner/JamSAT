#include <libjamsat/solver/Assignment.h>

#include <libjamsat/utils/Assert.h>
#include <libjamsat/utils/Casts.h>

#if defined(JAM_ENABLE_ASSIGNMENT_LOGGING)
#define JAM_LOG_ASSIGN(x, y) JAM_LOG(x, "assign", y)
#else
#define JAM_LOG_ASSIGN(x, y)
#endif

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

void assignment::clear_clauses() noexcept {
    m_watchers.clear();
    m_binaryWatchers.clear();
}

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

void assignment::assign(CNFLit literal, Clause* reason) {
    m_trail.push_back(literal);

    TBool const value =
        TBool::fromUnderlyingValue(static_cast<TBool::UnderlyingType>(literal.getSign()));
    m_assignments[literal.getVariable()] = value;
    m_reasons_and_als[literal.getVariable()].m_level = get_current_level();
    m_reasons_and_als[literal.getVariable()].m_reason = reason;
}

auto assignment::append(CNFLit literal, up_mode mode) -> Clause* {
    assign(literal, nullptr);
    return propagate_until_fixpoint(literal, mode);
}

void assignment::register_clause(Clause& clause) {
    JAM_ASSERT(clause.size() >= 2ull, "Illegally small clause argument");
    JAM_LOG_ASSIGN(info,
                   "Registering clause " << &clause << " ("
                                         << toString(clause.begin(), clause.end())
                                         << ") for propagation.");

    bool const isRedundant = clause.getFlag(Clause::Flag::REDUNDANT);
    CNFLit const lit0 = clause[0];
    CNFLit const lit1 = clause[1];
    detail_propagation::Watcher<Clause> watcher1{clause, lit0, 1, isRedundant};
    detail_propagation::Watcher<Clause> watcher2{clause, lit1, 0, isRedundant};

    auto& target_watch_list = (clause.size() <= 2 ? m_binaryWatchers : m_watchers);
    target_watch_list.addWatcher(lit0, watcher2);
    target_watch_list.addWatcher(lit1, watcher1);
}

auto assignment::register_lemma(Clause& clause) -> Clause* {
    register_clause(clause);

    JAM_EXPENSIVE_ASSERT(
        std::all_of(clause.begin() + 1, clause.end(), [this](CNFLit l) { return isFalse(get(l)); }),
        "Added a clause requiring first-literal propagation which does not actually "
        "force the first literal");
    JAM_LOG_ASSIGN(info, "Propagating first literal of registered clause.");
    CNFLit const asserting_lit = clause[0];
    assign(asserting_lit, &clause);
    return propagate_until_fixpoint(asserting_lit, up_mode::include_lemmas);
}


void assignment::register_clause_modification(Clause& clause) noexcept {
    JAM_LOG_ASSIGN(info,
                   "About to modify clause: " << std::addressof(clause) << " ("
                                              << toString(clause.begin(), clause.end()) << ")");
    JAM_ASSERT(clause.size() >= 2, "Can't modify clauses with size <= 1");
    JAM_ASSERT(!is_reason(clause), "Can't modify reason clauses");
    if (m_lits_with_required_watcher_update[clause[0]] != 1) {
        m_lits_with_required_watcher_update[clause[0]] = 1;
        m_lits_with_required_watcher_update_as_vec.push_back(clause[0]);
    }
    if (m_lits_with_required_watcher_update[clause[1]] != 1) {
        m_lits_with_required_watcher_update[clause[1]] = 1;
        m_lits_with_required_watcher_update_as_vec.push_back(clause[1]);
    }
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

auto assignment::propagate_until_fixpoint(CNFLit to_propagate, up_mode mode) -> Clause* {
    JAM_LOG_ASSIGN(info, "Propagating assignment until fixpoint: " << toPropagate);

    if (is_watcher_cleanup_required()) {
        cleanup_watchers();
    }

    auto prop_queue_begin = m_trail.begin() + m_trail.size();

    size_t amnt_new_facts = 0;
    Clause* conflicting_clause = nullptr;
    if (mode == up_mode::exclude_lemmas) {
        conflicting_clause = propagate<up_mode::exclude_lemmas>(to_propagate, amnt_new_facts);
    } else {
        conflicting_clause = propagate<up_mode::include_lemmas>(to_propagate, amnt_new_facts);
    }

    if (conflicting_clause) {
        return conflicting_clause;
    }

    auto prop_queue_end = prop_queue_begin + amnt_new_facts;

    // Propagate all forced assignments. New assignments are added to the
    // assignment provider by propagate, and therefore are also added to the
    // propagation queue.

    while (prop_queue_begin != prop_queue_end) {
        JAM_LOG_ASSIGN(
            info, "  Propagating until fixpoint: " << amnt_new_facts << " assignments pending");
        size_t local_new_facts = 0;
        if (mode == up_mode::exclude_lemmas) {
            conflicting_clause =
                propagate<up_mode::exclude_lemmas>(*prop_queue_begin, local_new_facts);
        } else {
            conflicting_clause =
                propagate<up_mode::include_lemmas>(*prop_queue_begin, local_new_facts);
        }
        prop_queue_end += local_new_facts;
        if (conflicting_clause) {
            return conflicting_clause;
        }
        ++prop_queue_begin;
    }

    JAM_LOG_ASSIGN(info, "  Done propagating to fixpoint.");
    // No more forced assignments can be propagated => fixpoint reached.
    return nullptr;
}

template <assignment::up_mode mode>
auto assignment::propagate(CNFLit to_propagate, size_t& amnt_new_facts) -> Clause* {
    // Caution: this method is on the solver's hottest path.

    JAM_LOG_ASSIGN(info, "  Propagating assignment: " << toPropagate);
    amnt_new_facts = 0;

    if (Clause* conflict = propagate_binaries(to_propagate, amnt_new_facts)) {
        return conflict;
    }

    CNFLit negated_to_prop = ~to_propagate;

    // Traverse all watchers referencing clauses containing ~toPropagate to find
    // new forced assignments.
    auto watcher_list_traversal = m_watchers.getWatchers(negated_to_prop);
    while (!watcher_list_traversal.hasFinishedTraversal()) {
        auto current_watcher = *watcher_list_traversal;

        if (mode == up_mode::exclude_lemmas && current_watcher.isClauseRedundant()) {
            ++watcher_list_traversal;
            continue;
        }

        CNFLit other_watched_lit = current_watcher.getOtherWatchedLiteral();
        TBool assignment = get(other_watched_lit);

        if (isTrue(assignment)) {
            // The clause is already satisfied and can be ignored for propagation.
            ++watcher_list_traversal;
            continue;
        }

        auto& clause = current_watcher.getClause();

        // other_watched_lit might not actually be the other watched literal due to
        // the swap at (*), so restore it
        other_watched_lit = clause[1 - current_watcher.getIndex()];
        assignment = get(other_watched_lit);
        if (isTrue(assignment)) {
            // The clause is already satisfied and can be ignored for propagation.
            ++watcher_list_traversal;
            continue;
        }

        // Invariant: both watchers pointing to the clause have an other watched
        // literal pointing either to clause[0] or clause[1], but not to the literal
        // which is their index in m_watchers.

        for (typename Clause::size_type i = 2; i < clause.size(); ++i) {
            CNFLit current_lit = clause[i];
            if (!isFalse(get(current_lit))) {
                // The FALSE literal is moved into the unwatched of the clause here,
                // such that an INDETERMINATE or TRUE literal gets watched.
                //
                // If otherLit is INDETERMINATE, this clause does not force anything,
                // and we can skip propagation.
                //
                // Since FALSE literals are moved into the non-watched part of the
                // clause as much as possible, otherLit can only be FALSE due to
                // a forced assignment which has not been propagated yet (but will
                // still be propagated in the future, causing a possible conflict
                // or propagation to be detected).
                std::swap(clause[current_watcher.getIndex()], clause[i]); // (*, see above)
                m_watchers.addWatcher(current_lit, current_watcher);
                watcher_list_traversal.removeCurrent();

                // No action is forced: skip to outer_continue to save some branches
                // Unfortunately, clang doesn't seem to jump there automatically if
                // some flag is set here and tested later :(
                goto outer_continue;
            }
        }

        // An action is forced: otherwise, the jump to outer_continue would have
        // been taken in the loop above

        // Invariant holding here: all literals in the clause beyond the second
        // literal have the value FALSE.
        if (isFalse(assignment)) {
            // Conflict case: all literals are FALSE. Return the conflicting clause.
            watcher_list_traversal.finishedTraversal();
            JAM_LOG_ASSIGN(info,
                           "  Current assignment is conflicting at clause " << &clause << ".");
            return &clause;
        } else {
            // Propagation case: other_watched_lit is the only remaining unassigned
            // literal
            ++amnt_new_facts;
            JAM_LOG_ASSIGN(info,
                           "  Forced assignment: " << other_watched_lit << " Reason: " << &clause);
            assign(other_watched_lit, &clause);
        }

        // Only advancing the traversal if an action is forced, since otherwise
        // the current watcher has been removed via removeCurrent() and
        // watcher_list_traversal already points to the next watcher.
        ++watcher_list_traversal;

    outer_continue:
        (void)0;
    }

    watcher_list_traversal.finishedTraversal();
    return nullptr;
}

auto assignment::propagate_binaries(CNFLit to_propagate, size_t& amnt_new_facts) -> Clause* {
    CNFLit negated_to_prop = ~to_propagate;
    auto watcher_list_traversal = m_binaryWatchers.getWatchers(negated_to_prop);
    while (!watcher_list_traversal.hasFinishedTraversal()) {
        auto& current_watcher = *watcher_list_traversal;
        CNFLit secondLit = current_watcher.getOtherWatchedLiteral();
        TBool assignment = get(secondLit);

        if (isFalse(assignment)) {
            // conflict case:
            JAM_LOG_ASSIGN(info,
                           "  Current assignment is conflicting at clause "
                               << &current_watcher.getClause() << ".");
            return &current_watcher.getClause();
        } else if (!isDeterminate(assignment)) {
            // propagation case:
            ++amnt_new_facts;
            Clause& reason = current_watcher.getClause();
            JAM_LOG_ASSIGN(info, "  Forced assignment: " << secondLit << " Reason: " << &reason);
            assign(secondLit, &reason);

            ++watcher_list_traversal;
            continue;
        }

        ++watcher_list_traversal;
    }
    watcher_list_traversal.finishedTraversal();
    return nullptr;
}

auto assignment::is_reason(Clause& clause) noexcept -> bool {
    JAM_ASSERT(clause.size() >= 2, "Argument clause must at have a size of 2");

    if (get_num_assignments() == 0) {
        // Special case for decision level 0, to avoid erroneously marking
        // clauses having been reasons for implied facts as reasons even
        // after backtracking:
        return false;
    }

    for (auto var : {clause[0].getVariable(), clause[1].getVariable()}) {
        if (get_reason(var) != &clause) {
            continue;
        }

        // The reason pointers do not neccessarily get cleared eagerly during backtracking
        auto decisionLevel = get_level(var);
        if (decisionLevel <= get_current_level()) {
            return true;
        }
    }
    return false;
}


void assignment::cleanup_watchers() {
    for (CNFLit dirty_lit : m_lits_with_required_watcher_update_as_vec) {
        cleanup_watchers(dirty_lit);
    }
    m_lits_with_required_watcher_update_as_vec.clear();
}

auto assignment::is_watcher_cleanup_required() const noexcept -> bool {
    return !m_lits_with_required_watcher_update_as_vec.empty();
}

void assignment::cleanup_watchers(CNFLit lit) {
    // This is not implemented as a detail of the watcher data structure
    // since watchers may be moved from the "regular" watchers to the binary
    // ones.
    // Since notifyClauseModificationAhead() may not be called for binary
    // clauses, it is sufficient to traverse the non-binary watchers.

    using WatcherType = detail_propagation::Watcher<Clause>;

    auto watcher_list_traversal = m_watchers.getWatchers(lit);
    while (!watcher_list_traversal.hasFinishedTraversal()) {
        WatcherType current_watcher = *watcher_list_traversal;
        Clause& clause = current_watcher.getClause();

        if (clause.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) == true) {
            watcher_list_traversal.removeCurrent();
            continue;
        }
        JAM_ASSERT(clause.size() >= 2,
                   "Clauses shrinked to size 1 must be removed from propagation");

        if (clause.size() == 2) {
            // The clause has become a binary clause ~> move to binary watchers
            current_watcher.setOtherWatchedLiteral(clause[1 - current_watcher.getIndex()]);

            // When a clause becomes binary, it may also lose its redundancy status.
            // However, the redundancy is not relevant for binary clauses wrt. propagation,
            // so just clear the flag:
            current_watcher.setClauseRedundant(false);

            m_binaryWatchers.addWatcher(clause[current_watcher.getIndex()], current_watcher);
            watcher_list_traversal.removeCurrent();
        } else if (clause[current_watcher.getIndex()] != lit) {
            // The clause has been modified externally and this watcher watches
            // the wrong literal ~> move the watcher
            current_watcher.setOtherWatchedLiteral(clause[1 - current_watcher.getIndex()]);

            // Optimizations (e.g. subsumption) may promote redundant clauses to
            // non-redundant clauses, so update the redundancy flag:
            current_watcher.setClauseRedundant(clause.getFlag(Clause::Flag::REDUNDANT));

            m_watchers.addWatcher(clause[current_watcher.getIndex()], current_watcher);
            watcher_list_traversal.removeCurrent();
        } else {
            ++watcher_list_traversal;
        }
    }
    watcher_list_traversal.finishedTraversal();

    auto binarywatcher_list_traversal = m_binaryWatchers.getWatchers(lit);
    while (!binarywatcher_list_traversal.hasFinishedTraversal()) {
        WatcherType current_watcher = *binarywatcher_list_traversal;
        Clause& clause = current_watcher.getClause();

        if (clause.getFlag(Clause::Flag::SCHEDULED_FOR_DELETION) == true) {
            binarywatcher_list_traversal.removeCurrent();
        } else {
            JAM_ASSERT(clause.size() >= 2,
                       "Clauses shrinked to size 1 must be removed from propagation");
            ++binarywatcher_list_traversal;
        }
    }
    binarywatcher_list_traversal.finishedTraversal();
    m_lits_with_required_watcher_update[lit] = 0;
}
}
