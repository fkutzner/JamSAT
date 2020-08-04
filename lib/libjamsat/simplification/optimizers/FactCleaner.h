#pragma once

#include <memory>

#include <libjamsat/simplification/ProblemOptimizer.h>

namespace jamsat {
std::unique_ptr<ProblemOptimizer> createFactCleaner();
}