#pragma once

#include <algorithm>

#include <gmock/gmock.h>


namespace jamsat {
MATCHER(range_empty, "Expected size matcher for ranges")
{
  return std::distance(arg.begin(), arg.end()) == 0;
}

MATCHER_P(range_size_is, expected_size, "Expected size matcher for ranges")
{
  using diff_type = typename decltype(arg.begin())::difference_type;
  diff_type expected_size_casted = static_cast<diff_type>(expected_size);
  return std::distance(arg.begin(), arg.end()) == expected_size_casted;
}

MATCHER_P(range_is, expected, "Expected size matcher for ranges")
{
  std::size_t arg_size = static_cast<std::size_t>(std::distance(arg.begin(), arg.end()));
  std::size_t expected_size =
      static_cast<std::size_t>(std::distance(expected.begin(), expected.end()));
  if (arg_size != expected_size) {
    return false;
  }
  else {
    return std::equal(arg.begin(), arg.end(), expected.begin());
  }
}
}