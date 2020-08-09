/* Copyright (c) 2018 Felix Kutzner (github.com/fkutzner)

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

#pragma once

#include <unordered_map>
#include <vector>

extern const char* IPASIRTestMockSignature;

namespace jamsat {
/**
 * Control structure for the IPASIR mock system
 */
struct IpasirMockContext {
  std::vector<int> m_literals;
  std::vector<int> m_assumptions;
  std::vector<int> m_assumptionsAtLastSolveCall;

  // Configure these during test setup:
  std::unordered_map<int, int> m_cfgLiteralVals;
  std::unordered_map<int, int> m_cfgLiteralFailures;
  int m_cfgSolveResult = 0;
};

/**
 * Gets the current IPASIR mock system control structure.
 *
 * Note: The IPASIR mock system is not thread-safe, since it can safely be assumed that
 * the JamSAT frontend test suite will remain small enough so that executing tests in
 * parallel threads remains unwarranted. Create one mock IPASIR solver at a time.
 */
auto getCurrentIPASIRMockContext() noexcept -> IpasirMockContext&;
}
