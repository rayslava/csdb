#pragma once

#include <cstdint>
#include <cstddef>

namespace algo {
  namespace hash {
    uint64_t crc64(const uint8_t* s, size_t size, uint64_t init=0);
  }
}
