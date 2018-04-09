#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/CDCLSatSolver.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

using SolverType = jamsat::CDCLSatSolver<>;

namespace {
void printUsage() {
    std::cerr << "Usage: SmallRandomSATTestDriver (--fail-on-parse-error|--no-fail-on-parse-error) "
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
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printUsage();
        return EXIT_FAILURE;
    }

    std::ifstream cnfFile{argv[2]};
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

    return EXIT_SUCCESS;
}
