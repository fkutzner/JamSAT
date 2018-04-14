#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/CDCLSatSolver.h>
#include <libjamsat/utils/Assert.h>

#include <minisat/core/Solver.h>
#include <minisat/core/SolverTypes.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

using SolverType = jamsat::CDCLSatSolver<>;

namespace {
void printUsage() {
    std::cerr << "Usage: SmallRandomSATTestDriver (--fail-on-parse-error|--no-fail-on-parse-error) "
                 "(--check-result|--no-check-result) "
                 "<FILENAME>\n";
}

int getParseErrorExitValue(std::string const &parseErrorMode) {
    if (parseErrorMode == "--fail-on-parse-error") {
        return EXIT_FAILURE;
    }
    if (parseErrorMode == "--no-fail-on-parse-error") {
        return EXIT_SUCCESS;
    }
    std::cerr << "Unknown parse error failure mode " << parseErrorMode << std::endl;
    return EXIT_FAILURE;
}

bool isCheckingWithMinisatEnabled(std::string const &checkParameter) {
    return checkParameter == "--check-result";
}

enum class CheckMinisatResult { MATCH, NO_MATCH, SKIPPED };

CheckMinisatResult checkResultWithMinisat(jamsat::CNFProblem &problem, jamsat::TBool result) {
    if (result == jamsat::TBool::INDETERMINATE) {
        return CheckMinisatResult::SKIPPED;
    }

    Minisat::Solver solver;
    for (unsigned int i = 0; i <= problem.getMaxVar().getRawValue(); ++i) {
        solver.newVar();
    }

    for (auto &clause : problem.getClauses()) {
        Minisat::vec<Minisat::Lit> minisatClause;
        for (auto lit : clause) {
            Minisat::Lit minisatLit =
                Minisat::mkLit(static_cast<Minisat::Var>(lit.getVariable().getRawValue()));
            minisatClause.push(lit.getSign() == jamsat::CNFSign::POSITIVE ? minisatLit
                                                                          : ~minisatLit);
        }
        solver.addClause(minisatClause);
    }

    bool satisfiable = solver.solve();
    bool equalSatResults = (satisfiable == (result == jamsat::TBool::TRUE));

    JAM_ASSERT(equalSatResults, "Minisat and JamSAT produced different SAT results");

    if (equalSatResults) {
        return CheckMinisatResult::MATCH;
    } else {
        return CheckMinisatResult::NO_MATCH;
    }
}
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printUsage();
        return EXIT_FAILURE;
    }

    std::ifstream cnfFile{argv[3]};
    if (!cnfFile || !cnfFile.is_open()) {
        std::cerr << "Error: could not open file " << argv[1] << "\n";
        return EXIT_FAILURE;
    }

    jamsat::CNFProblem problem;
    cnfFile >> problem;
    if (cnfFile.fail()) {
        std::cerr << "Error: could not parse " << argv[1] << "\n";
        return getParseErrorExitValue(argv[1]);
    }

    typename SolverType::Configuration config;
    config.clauseMemoryLimit = 10485760;


    SolverType solver{config};
    solver.addProblem(problem);
    auto result = solver.solve({});
    if (result.isSatisfiable == jamsat::TBool::TRUE) {
        std::cout << "Satisfiable:1" << std::endl;
    } else if (result.isSatisfiable == jamsat::TBool::FALSE) {
        std::cout << "Satisfiable:0" << std::endl;
    } else {
        std::cout << "Satisfiable:-1" << std::endl;
    }

    bool checkWithMinisat = isCheckingWithMinisatEnabled(argv[2]);

    if (checkWithMinisat &&
        checkResultWithMinisat(problem, result.isSatisfiable) == CheckMinisatResult::NO_MATCH) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
