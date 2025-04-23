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

struct Transform {
  glm::vec3 pos{0.0f};
  glm::vec3 scale{1.0f};
  glm::vec3 rotation{0.0f};
};