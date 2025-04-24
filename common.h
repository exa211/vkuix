#pragma once

#include <cstdint>
#include <memory>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"

#include "log.h"

// Primitive Types
using u32 = uint32_t;
using u16 = uint16_t;

// Memory
template <typename T>
using uptr = std::unique_ptr<T>;

template <typename T>
using sptr = std::shared_ptr<T>;

namespace VKUIX {

  struct Dim {
    u32 width;
    u32 height;
  };

  struct Transform {
    glm::vec3 pos{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 rotation{0.0f};
  };

  struct Color {
    float r;
    float g;
    float b;
    float a;

    Color toDecimal() {
      r = r/255.0f;
      g = g/255.0f;
      b = b/255.0f;
      a = a/255.0f;
      return *this;
    }

    [[nodiscard]] glm::vec4 glmDecimal() const {
      return {r/255.0f, g/255.0f, b/255.0f, a/255.0f};
    }

  };

}