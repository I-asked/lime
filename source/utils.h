#pragma once

#include "macros.h"

#include <algorithm>
#include <bit>
#include <compare>
#include <cstddef>
#include <limits>

_inline constexpr const unsigned char *operator "" _ucs(const char *str, size_t len) {
  (void)len;
  return std::bit_cast<const unsigned char *>(str);
}

template <typename T>
_inline T _clamp(T v, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
  return std::min(max, std::max(min, v));
}

template <typename T = float>
_inline T _clampf(T v, T min = .0, T max = 1.) {
  return std::min(max, std::max(min, v));
}

constexpr auto _clampd = _clampf<double>;
