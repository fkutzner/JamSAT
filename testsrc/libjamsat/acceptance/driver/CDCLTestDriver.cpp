#include <libjamsat/cnfproblem/CNFProblem.h>
#include <libjamsat/drivers/CDCLSatSolver.h>
#include <libjamsat/utils/Assert.h>

#include <toolbox/testutils/Minisat.h>
#include <toolbox/testutils/OnlineDRATChecker.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
void printUsage()
{
  std::cerr << "Usage: SmallRandomSATTestDriver (--fail-on-parse-error|--no-fail-on-parse-error) "
               "(--check-result|--no-check-result) "
               "<FILENAME>\n"
            << " If <FILENAME> is " - ", the problem is read from stdin.\n";
}

int getParseErrorExitValue(std::string const& parseErrorMode)
{
  if (parseErrorMode == "--fail-on-parse-error") {
    return EXIT_FAILURE;
  }
  if (parseErrorMode == "--no-fail-on-parse-error") {
    return EXIT_SUCCESS;
  }
  std::cerr << "Unknown parse error failure mode " << parseErrorMode << std::endl;
  return EXIT_FAILURE;
}

bool isCheckingResultEnabled(std::string const& checkParameter)
{
  return checkParameter == "--check-result";
}

enum class CheckMinisatResult { MATCH, NO_MATCH, SKIPPED };

CheckMinisatResult checkResultWithMinisat(jamsat::CNFProblem& problem, jamsat::TBool result)
{
  if (!isDeterminate(result)) {
    return CheckMinisatResult::SKIPPED;
  }

  jamsat::TBool satisfiable = jamsat::isSatisfiableViaMinisat(problem);

  JAM_ASSERT(satisfiable == result, "Minisat and JamSAT produced different SAT results");

  if (satisfiable == result) {
    return CheckMinisatResult::MATCH;
  }
  else {
    return CheckMinisatResult::NO_MATCH;
  }
}
}

void printDRATCheckerFailure(jamsat::OnlineDRATChecker const& checker)
{
  std::cout << "DRAT proof error:\n";
  for (std::string const& msg : checker.getResultComments()) {
    std::cout << msg << "\n";
  }
}

int main(int argc, char** argv)
{
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
  }
  else {
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

  std::unique_ptr<jamsat::OnlineDRATChecker> dratChecker;
  bool const checkResultEnabled = isCheckingResultEnabled(argv[2]);
  if (checkResultEnabled) {
    dratChecker = jamsat::createOnlineDRATChecker(problem);
  }

  std::unique_ptr<jamsat::CDCLSatSolver> solver = jamsat::createCDCLSatSolver();
  solver->setDRATCertificate(*dratChecker);
  solver->addProblem(problem);
  auto result = solver->solve({});

  if (checkResultEnabled) {
    if (dratChecker->hasDetectedInvalidLemma() || dratChecker->hasDetectedUnsupportedLemma()) {
      printDRATCheckerFailure(*dratChecker);
      return EXIT_FAILURE;
    }

    bool verified = false;
    if (jamsat::isTrue(result->isProblemSatisfiable())) {
      if (dratChecker->hasValidatedUnsat()) {
        std::cout << "Error: generated an UNSAT certificate for a satisfiable problem\n";
        return EXIT_FAILURE;
      }

      auto optModelRef = result->getModel();
      auto& model = (*optModelRef).get();

      bool problemSatisfied = jamsat::isTrue(model.check(problem));
      JAM_ASSERT(problemSatisfied,
                 "The assignment produced by the solver does not satisfy the formula");
      if (!problemSatisfied) {
        return EXIT_FAILURE;
      }

      // satisfied => don't check again with minisat
      verified = true;
    }
    else {
      if (!dratChecker->hasValidatedUnsat()) {
        std::cout << "Error: failed to generate an UNSAT proof for an unsatisfiable problem\n";
        return EXIT_FAILURE;
      }
    }

    if (!verified && checkResultWithMinisat(problem, result->isProblemSatisfiable()) ==
                         CheckMinisatResult::NO_MATCH) {
      return EXIT_FAILURE;
    }
  }

  if (result->isProblemSatisfiable() == jamsat::TBools::TRUE) {
    std::cout << "Satisfiable:1" << std::endl;
  }
  else if (result->isProblemSatisfiable() == jamsat::TBools::FALSE) {
    std::cout << "Satisfiable:0" << std::endl;
  }
  else {
    std::cout << "Satisfiable:-1" << std::endl;
  }

  return EXIT_SUCCESS;
}
