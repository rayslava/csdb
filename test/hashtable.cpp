#include "structure/hashtable.hpp"

#include "algo/crc32.hpp"
#include "algo/crc64.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

using namespace structure::hashtable;
using namespace algo::hash;

TEST(hashtable, construct)
{
  EXPECT_NO_THROW(({
    HashTable<crc32> ht();
  }));
  EXPECT_NO_THROW(({
    HashTable<crc64> ht();
  }));
}
