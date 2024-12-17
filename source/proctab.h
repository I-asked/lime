#pragma once

#include "lime.h"

#include <compare>
#include <cstring>

struct proctab_entry {
  const char *name;
  void (*proc)();

  auto operator<=>(const proctab_entry &other) {
    return strcmp(name, other.name);
  }

  auto operator<=>(const char *const &other) {
    return strcmp(name, other) == 0;
  }
};
