#pragma once

#include "log.h"

#include <cstdint>
#include <memory>

// Primitive Types
using u32 = uint32_t;
using u16 = uint16_t;

// Memory
template <typename T>
using uptr = std::unique_ptr<T>;

template <typename T>
using sptr = std::shared_ptr<T>;