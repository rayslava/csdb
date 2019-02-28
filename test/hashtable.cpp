#include "structure/hashtable.hpp"

#include "algo/crc32.hpp"
#include "algo/crc64.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

namespace structure::hashtable {
  using namespace algo::hash;
  const char* key = "test";
  const char* key2 = "test2";

  template class HashTable<std::string, int, crc64>;
  template class HashTable<std::string, int, crc32>;
  template class Node<std::string, int>;
  template class Bucket<std::string, int>;
  template class Storage<std::string, int, storage_len>;

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
  }

  TEST (hashtable, get)
  {
    EXPECT_THROW(({
      HashTable<const char *, int, crc64> ht{};
      const auto& reader = ht;
      const auto& a = reader[key];
    }), std::out_of_range);

    EXPECT_THROW(({
      HashTable<const char *, int, crc64> ht{};
      const auto& a = ht.at(key);
    }), std::out_of_range);
  }

  TEST (hashtable, set)
  {
    HashTable<const char *, int, crc64> ht{};
    ht[key] = 42;
    ASSERT_EQ(ht.at(key), 42);
  }

  uint64_t fake_hash(const uint8_t* s, size_t size, uint64_t init=0) {
    return 1;
  }

  template class HashTable<char const *, int, crc64>;
  template class HashTable<char const *, int, fake_hash>;
  template class Node<char const *, int>;
  template class Bucket<char const *, int>;
  template class Storage<char const *, int, storage_len>;

  TEST (hashtable, linear)
  {
    HashTable<const char *, int, fake_hash> ht{};
    ht[key] = 42;
    ht[key2] = 43;
    ASSERT_EQ(ht.at(key),  42);
    ASSERT_EQ(ht.at(key2), 43);
    ASSERT_EQ(ht.at(key),  42);
  }

  TEST (hashtable, erase)
  {
    HashTable<const char *, int, fake_hash> ht{};
    ht[key] = 42;
    ht[key2] = 43;
    ASSERT_EQ(ht.at(key),  42);
    ASSERT_EQ(ht.at(key2), 43);
    ht.erase(key);
    ASSERT_EQ(ht.at(key2), 43);
    EXPECT_THROW(({
      const auto& reader = ht;
      const auto& a = reader[key];
    }), std::out_of_range);
    ht.erase(key2);
    EXPECT_THROW(({
      const auto& reader = ht;
      const auto& a = reader[key2];
    }), std::out_of_range);
  }
}
