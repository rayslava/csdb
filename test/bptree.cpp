#include "structure/bptree.hpp"

#include "algo/crc32.hpp"
#include "algo/crc64.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

namespace structure::bptree {
  const char* key = "test";
  const char* key2 = "test2";
  const char* key3 = "test3";

  TEST(bptree, construct)
  {
    BPTree<uint64_t, int, 3> bt{};
    auto& newval = bt.insert(42, 43);

    EXPECT_NO_THROW(({
      BPTree<uint64_t, int, 3> bt{};
      auto& newval = bt.insert(42, 54);
    }));
  }
}
