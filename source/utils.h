#pragma once

#include "macros.h"

#include <bit>
#include <compare>
#include <cstddef>

_inline constexpr const unsigned char *operator "" _ucs(const char *str, size_t len) {
  (void)len;
  return std::bit_cast<const unsigned char *>(str);
}
