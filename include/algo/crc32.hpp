#pragma once

#include <cstdint>
#include <cstddef>

namespace algo {
  namespace hash {
    uint32_t crc32 (const unsigned char* s, size_t size, uint32_t init=~0);
  }
}
