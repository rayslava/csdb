#include "structure/hashtable.hpp"

#include "algo/crc32.hpp"
#include "algo/crc64.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

using namespace structure::hashtable;
using namespace algo::hash;
const char* key = "test";

TEST(hashtable, construct)
{

  EXPECT_NO_THROW(({
    HashTable<std::string, int, crc32> ht{};
  }));
  EXPECT_NO_THROW(({
    HashTable<std::string, int, crc64> ht{};
  }));
  EXPECT_NO_THROW(({
    HashTable<int, std::string, crc64> ht{};
  }));
  EXPECT_NO_THROW(({
    HashTable<const char *, int, crc64> ht{};
    auto a = ht[key];
  }));
}

TEST (hashtable, set)
{
  HashTable<const char *, int, crc64> ht{};
  auto a = ht[key];
  ht[key] = 42;
  ASSERT_EQ(ht[key], 42);
}
