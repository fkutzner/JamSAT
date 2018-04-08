#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/solver/CDCLSatSolver.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

using SolverType = jamsat::CDCLSatSolver<>;

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: SmallRandomSATTestDriver <FILENAME>\n";
        return EXIT_FAILURE;
    }

    std::ifstream cnfFile{argv[1]};
    if (!cnfFile || !cnfFile.is_open()) {
        std::cerr << "Error: could not open file " << argv[1] << "\n";
        return EXIT_FAILURE;
    }

    jamsat::CNFProblem problem;
    cnfFile >> problem;
    if (cnfFile.fail()) {
        std::cerr << "Error: could not parse " << argv[1] << "\n";
        return EXIT_FAILURE;
    }

    typename SolverType::Configuration config;
    config.clauseMemoryLimit = 10495860;


    SolverType solver{config};
    solver.addProblem(problem);
    auto result = solver.solve({});
    if (result.isSatisfiable == jamsat::TBool::TRUE) {
        std::cout << "SATISFIABLE" << std::endl;
    } else if (result.isSatisfiable == jamsat::TBool::FALSE) {
        std::cout << "UNSATISTIABLE" << std::endl;
    } else {
        std::cout << "INDETERMINATE" << std::endl;
    }

    return EXIT_SUCCESS;
}
