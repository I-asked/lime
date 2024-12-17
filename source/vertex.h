#pragma once

#include "macros.h"
#include <3ds.h>

#include <glm/glm.hpp>
#include <initializer_list>
#include <memory>

#define VERTEX3D_FORMAT

namespace lime {

struct Vertex3D {
  static constexpr auto format =
      (GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 4, GPU_FLOAT) |
       GPU_ATTRIBFMT(2, 4, GPU_FLOAT) | GPU_ATTRIBFMT(3, 4, GPU_FLOAT));

  glm::vec3 m_position;
  glm::vec4 m_color;
  glm::vec4 m_texCoord0;
  glm::vec4 m_texCoord1;
};

struct linearFree_t {
  void operator()(void *ptr) { linearFree(ptr); }
};

template <typename T>
_inline static std::unique_ptr<T, linearFree_t>
reserve_linear(std::initializer_list<T> init) {
  std::unique_ptr<T, linearFree_t> vbo(
      reinterpret_cast<T *>(linearAlloc(init.size() * sizeof(T))));
  int i = 0;
  for (auto &vertex : init) {
    new (&vbo.get()[i++]) T(std::move(vertex));
  }
  return vbo;
}

constexpr auto reserve_linear_vbo = reserve_linear<Vertex3D>;
} // namespace lime
