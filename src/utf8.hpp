/// Little UTF-8 Lib, inspired by dmenu but bloated
/// because it is BloatMenu XD
#pragma once

#include <cstdint>

namespace stolen_from_dmenu {

static const uint32_t UTF_INVALID = 0xFFFD;

struct CodePoint {
  std::size_t len;
  uint32_t value;
  bool err;
  const char* ref;
};

// Oh no! We copy 24 bytes of data, how inefficient!
CodePoint utf8decode(const char *s_in);

} // namespace sg
