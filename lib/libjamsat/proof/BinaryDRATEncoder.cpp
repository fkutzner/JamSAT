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

#include <libjamsat/proof/BinaryDRATEncoder.h>

#include <libjamsat/utils/Assert.h>

namespace jamsat {
namespace {
template <typename OutIter>
auto EncodeBinaryDRAT(CNFLit literal, OutIter outIter) noexcept -> OutIter
{
  CNFLit::RawLiteral rawValue = literal.getRawValue();
  rawValue ^= 1; // Flip the sign bit: DRAT has LSB 0 for positive literals

  do {
    uint8_t leastSignificant = rawValue & 0x7F;
    uint8_t msbMask = ((rawValue >> 7) != 0) ? 0x80 : 0x00;
    uint8_t output = leastSignificant | msbMask;
    *outIter = output;

    rawValue = rawValue >> 7;
    ++outIter;
  } while (rawValue != 0);

  return outIter;
}
}

auto EncodeBinaryDRAT(gsl::span<CNFLit const> literals, gsl::span<unsigned char> target) noexcept
    -> std::size_t
{
  JAM_ASSERT(target.size() >= literals.size() * 5, "Encoding target has insufficient space");
  auto outputIter = target.begin();
  for (CNFLit lit : literals) {
    outputIter = EncodeBinaryDRAT(lit, outputIter);
  }

  return static_cast<std::size_t>(std::distance(target.begin(), outputIter));
}
}
