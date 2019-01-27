#include "algo/crc64.hpp"

#include <cstdint>
#include <gtest/gtest.h>

TEST(crc64, main)
{
  ASSERT_EQ(0xe9c6d914c4b8d9ca, algo::hash::crc64((uint8_t *) "123456789", 9));
}
