#include "algo/crc32.hpp"

#include <cstdint>
#include <gtest/gtest.h>

TEST(crc32, main)
{
  ASSERT_EQ(0xCBF43926, algo::hash::crc32((uint8_t *) "123456789", 9));
}
