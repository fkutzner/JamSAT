#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/CDCLSatSolver.h>
#include <libjamsat/utils/Assert.h>

#include <toolbox/testutils/Minisat.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

using SolverType = jamsat::CDCLSatSolver<>;

namespace {
void printUsage() {
    std::cerr << "Usage: SmallRandomSATTestDriver (--fail-on-parse-error|--no-fail-on-parse-error) "
                 "(--check-result|--no-check-result) "
                 "<FILENAME>\n"
              << " If <FILENAME> is " - ", the problem is read from stdin.\n";
}

int getParseErrorExitValue(std::string const& parseErrorMode) {
    if (parseErrorMode == "--fail-on-parse-error") {
        return EXIT_FAILURE;
    }
    if (parseErrorMode == "--no-fail-on-parse-error") {
        return EXIT_SUCCESS;
    }
    std::cerr << "Unknown parse error failure mode " << parseErrorMode << std::endl;
    return EXIT_FAILURE;
}

bool isCheckingResultEnabled(std::string const& checkParameter) {
    return checkParameter == "--check-result";
}

enum class CheckMinisatResult { MATCH, NO_MATCH, SKIPPED };

CheckMinisatResult checkResultWithMinisat(jamsat::CNFProblem& problem, jamsat::TBool result) {
    if (!isDeterminate(result)) {
        return CheckMinisatResult::SKIPPED;
    }

    jamsat::TBool satisfiable = jamsat::isSatisfiableViaMinisat(problem);

    JAM_ASSERT(satisfiable == result, "Minisat and JamSAT produced different SAT results");

    if (satisfiable == result) {
        return CheckMinisatResult::MATCH;
    } else {
        return CheckMinisatResult::NO_MATCH;
    }
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printUsage();
        return EXIT_FAILURE;
    }

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::istream* cnfFile;
    std::ifstream cnfPhysFile;

    if (std::string{argv[3]} == "-") {
        cnfFile = &std::cin;
    } else {
        cnfPhysFile = std::ifstream{argv[3]};
        if (!cnfPhysFile || !cnfPhysFile.is_open()) {
            std::cerr << "Error: could not open file " << argv[1] << "\n";
            return EXIT_FAILURE;
        }
        cnfFile = &cnfPhysFile;
    }

    jamsat::CNFProblem problem;
    *cnfFile >> problem;
    if (cnfFile->fail()) {
        std::cerr << "Error: could not parse " << argv[1] << "\n";
        return getParseErrorExitValue(argv[1]);
    }

    typename SolverType::Configuration config;
    config.clauseMemoryLimit = 1048576ull * 1024ull;


    SolverType solver{config};
    solver.addProblem(problem);
    auto result = solver.solve({});

    bool checkResultEnabled = isCheckingResultEnabled(argv[2]);
    if (checkResultEnabled) {
        bool verified = false;
        if (jamsat::isTrue(result.isSatisfiable)) {
            bool problemSatisfied = jamsat::isTrue(result.model->check(problem));
            JAM_ASSERT(problemSatisfied,
                       "The assignment produced by the solver does not satisfy the formula");
            if (!problemSatisfied) {
                return EXIT_FAILURE;
            }

            // satisfied => don't check again with minisat
            verified = true;
        }
        if (!verified &&
            checkResultWithMinisat(problem, result.isSatisfiable) == CheckMinisatResult::NO_MATCH) {
            return EXIT_FAILURE;
        }
    }

    if (result.isSatisfiable == jamsat::TBools::TRUE) {
        std::cout << "Satisfiable:1" << std::endl;
    } else if (result.isSatisfiable == jamsat::TBools::FALSE) {
        std::cout << "Satisfiable:0" << std::endl;
    } else {
        std::cout << "Satisfiable:-1" << std::endl;
    }

    return EXIT_SUCCESS;
}
