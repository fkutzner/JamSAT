/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

#include "Statistics.h"

#include <sstream>
#include <string>

namespace jamsat {
auto OptimizationStats::operator+=(OptimizationStats const& rhs) -> OptimizationStats&
{
  amntFactsDerived += rhs.amntFactsDerived;
  amntLitsRemoved += rhs.amntLitsRemoved;
  amntClausesRemoved += rhs.amntClausesRemoved;
  amntClausesAdded += rhs.amntClausesAdded;
  amntVarsEliminated += rhs.amntVarsEliminated;
  amntVarsAdded += rhs.amntVarsAdded;
  return *this;
}


auto operator<<(std::ostream& output, OptimizationStats const& stats) -> std::ostream&
{
  // clang-format off
    output << "{"
           << "F:" << stats.amntFactsDerived
           << ",LR:" << stats.amntLitsRemoved
           << ",ClR" << stats.amntClausesRemoved
           << ",ClA:" << stats.amntClausesAdded
           << ",VE:" << stats.amntVarsEliminated
           << ",VA:" << stats.amntVarsAdded
           << "}";
           return output;
  // clang-format on
}

auto to_string(OptimizationStats const& stats) -> std::string
{
  std::stringstream result;
  result << stats;
  return result.str();
}
}