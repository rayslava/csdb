#include "structure/bptree.hpp"

#include "algo/crc32.hpp"
#include "algo/crc64.hpp"

#include <cstdint>
#include <variant>
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

namespace structure::bptree {
  const char* key = "test";
  const char* key2 = "test2";
  const char* key3 = "test3";

  using testnode_t = BPTNode<uint64_t, int, 3>;

  TEST(leaf, construct) {
    testnode_t leaf{true};
    auto result = leaf.find(42);
    ASSERT_FALSE(result.has_value());
  }

  TEST(node, construct3) {
    testnode_t node{false};
    testnode_t* leafl = new testnode_t(true);
    testnode_t* leafc = new testnode_t(true);
    testnode_t* leafr = new testnode_t(true);
    node.keys[0] = 40;
    node.keys[1] = 43;
    node.values[0] = leafl;
    node.values[1] = leafc;
    node.values[2] = leafr;
    node.status.size = 2;
    leafl->keys[0] = 39;
    leafl->values[0] = 123;
    leafl->status.size = 1;
    leafc->keys[0] = 42;
    leafc->values[0] = 321;
    leafc->status.size = 1;
    leafr->keys[0] = 44;
    leafr->values[0] = 213;
    leafr->status.size = 1;
    auto result = node.find(42);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 321);
    result = node.find(44);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 213);
    result = node.find(39);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 123);
  }

  TEST(node, insert_plain) {
    testnode_t leaf{true};
    leaf.insert(42, 321);
    auto result = leaf.find(42);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 321);
    leaf.insert(41, 123);
    result = leaf.find(41);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 123);
    leaf.insert(40, 213);
    result = leaf.find(40);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 213);
    result = leaf.find(41);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 123);
    result = leaf.find(42);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result, 321);
  }

  TEST(node, small_split) {
    testnode_t node{false};
    testnode_t* leafl = new testnode_t(true, &node);
    testnode_t* leafr = new testnode_t(true, &node);
    node.keys[0] = 40;
    node.values[0] = leafl;
    node.values[1] = leafr;
    node.status.size = 1;
    node.status.isEmpty = false;

    leafl->keys[0] = 30;
    leafl->values[0] = 1;
    leafl->keys[1] = 33;
    leafl->values[1] = 2;
    leafl->keys[2] = 35;
    leafl->values[2] = 3;
    leafl->status.size = 3;
    leafl->values[3] = leafr;

    leafr->keys[0] = 41;
    leafr->values[0] = 1;
    leafr->keys[1] = 43;
    leafr->values[1] = 2;
    leafr->keys[2] = 45;
    leafr->values[2] = 3;
    leafr->status.size = 3;
    leafr->values[3] = nullptr;

    leafl->insert(32, 4);

    ASSERT_EQ(node.status.size, 2);
    ASSERT_TRUE((std::holds_alternative<testnode_t *>(node.values[0])));
    ASSERT_TRUE((std::holds_alternative<testnode_t *>(node.values[1])));
    ASSERT_TRUE((std::holds_alternative<testnode_t *>(node.values[2])));
    auto newleaf1 = std::get<testnode_t *>(node.values[0]);
    auto newleaf2 = std::get<testnode_t *>(node.values[1]);
    auto newleaf3 = std::get<testnode_t *>(node.values[2]);

    std::string leafline1{node};
    std::string leafline2{*leafl};
    std::string leafline3{*leafr};

    std::string nleafline1{*newleaf1};
    std::string nleafline2{*newleaf2};
    std::string nleafline3{*newleaf3};

    ASSERT_EQ(newleaf1->status.size, 2);
    ASSERT_EQ(newleaf2->status.size, 2);
    ASSERT_EQ(newleaf3->status.size, 3);

    ASSERT_TRUE(newleaf1->status.isLeaf);
    ASSERT_TRUE(newleaf2->status.isLeaf);
    ASSERT_TRUE(newleaf3->status.isLeaf);
    ASSERT_FALSE(node.status.isLeaf);

    ASSERT_EQ(newleaf1->keys[0],		  30);
    ASSERT_EQ(newleaf1->keys[1],		  32);
    ASSERT_EQ(std::get<int>(newleaf1->values[0]), 1);
    ASSERT_EQ(std::get<int>(newleaf1->values[1]), 4);

    ASSERT_EQ(newleaf2->keys[0],		  33);
    ASSERT_EQ(newleaf2->keys[1],		  35);
    ASSERT_EQ(std::get<int>(newleaf2->values[0]), 2);
    ASSERT_EQ(std::get<int>(newleaf2->values[1]), 3);

    ASSERT_EQ(newleaf3->keys[0],		  41);
    ASSERT_EQ(newleaf3->keys[1],		  43);
    ASSERT_EQ(newleaf3->keys[2],		  45);
    ASSERT_EQ(std::get<int>(newleaf3->values[0]), 1);
    ASSERT_EQ(std::get<int>(newleaf3->values[1]), 2);
    ASSERT_EQ(std::get<int>(newleaf3->values[2]), 3);

    ASSERT_EQ((std::get<testnode_t *>(newleaf1->values[newleaf1->status.size])),
              newleaf2);
    ASSERT_EQ((std::get<testnode_t *>(newleaf2->values[newleaf2->status.size])),
              newleaf3);
    ASSERT_EQ((std::get<testnode_t *>(newleaf3->values[newleaf3->status.size])),
              nullptr);

    auto result = node.find(32);
    ASSERT_EQ(result, 4);
  }

  TEST(node, root_split) {
    testnode_t node{true};

    node.keys[0] = 10;
    node.keys[1] = 20;
    node.keys[2] = 30;
    node.values[0] = 1;
    node.values[1] = 2;
    node.values[2] = 3;
    node.status.size = 3;

    node.insert(15, 4);
    auto newleaf1 = std::get<testnode_t *>(node.values[0]);
    auto newleaf2 = std::get<testnode_t *>(node.values[1]);
    ASSERT_EQ(newleaf1->status.size, 2);
    ASSERT_EQ(newleaf2->status.size, 2);

    ASSERT_TRUE(newleaf1->status.isLeaf);
    ASSERT_TRUE(newleaf2->status.isLeaf);
    ASSERT_FALSE(node.status.isLeaf);

    ASSERT_EQ(newleaf1->keys[0],		  10);
    ASSERT_EQ(newleaf1->keys[1],		  15);
    ASSERT_EQ(std::get<int>(newleaf1->values[0]), 1);
    ASSERT_EQ(std::get<int>(newleaf1->values[1]), 4);

    ASSERT_EQ(newleaf2->keys[0],		  20);
    ASSERT_EQ(newleaf2->keys[1],		  30);
    ASSERT_EQ(std::get<int>(newleaf2->values[0]), 2);
    ASSERT_EQ(std::get<int>(newleaf2->values[1]), 3);
    ASSERT_EQ((std::get<testnode_t *>(newleaf1->values[newleaf1->status.size])),
              newleaf2);
  }

  TEST(node, random_insert) {
    testnode_t node{true};
    node.insert(20, 2);
    node.insert(10, 1);
    node.insert(30, 3);
    ASSERT_TRUE(node.status.isLeaf);
    ASSERT_EQ(node.status.size, 3);

    node.insert(5, 10);
    ASSERT_FALSE(node.status.isLeaf);
    ASSERT_EQ(node.status.size, 1);
    ASSERT_EQ(node.keys[0],	20);

    auto newleaf1 = std::get<testnode_t *>(node.values[0]);
    auto newleaf2 = std::get<testnode_t *>(node.values[1]);

    ASSERT_TRUE(newleaf1->status.isLeaf);
    ASSERT_EQ(newleaf1->status.size,		  2);
    ASSERT_EQ(newleaf1->keys[0],		  5);
    ASSERT_EQ(newleaf1->keys[1],		  10);
    ASSERT_EQ(std::get<int>(newleaf1->values[0]), 10);
    ASSERT_EQ(std::get<int>(newleaf1->values[1]), 1);

    ASSERT_TRUE(newleaf2->status.isLeaf);
    ASSERT_EQ(newleaf2->status.size,		  2);
    ASSERT_EQ(newleaf2->keys[0],		  20);
    ASSERT_EQ(newleaf2->keys[1],		  30);
    ASSERT_EQ(std::get<int>(newleaf2->values[0]), 2);
    ASSERT_EQ(std::get<int>(newleaf2->values[1]), 3);

    ASSERT_EQ((std::get<testnode_t *>(newleaf1->values[newleaf1->status.size])),
              newleaf2);

  }

  TEST(node, overfulling_insert) {
    DEFAULT_LOGGING;
    DEFAULT_LOGGER_SEVERITY(logging::Severity::trace);
    {
      testnode_t node{true};
      for (int i = 1; i < 20; i++)
        node.insert(i * 10, i);
      for (int i = 1; i < 20; i++) {
        auto something = node.find(i * 10);
        ASSERT_TRUE(something.has_value());
        ASSERT_EQ(something, i);
      }
    }
    {
      testnode_t node{true};
      for (int i = 20; i > 0; i--) {
        node.insert(i * 13, i);
      }
      for (int i = 1; i <= 20; i++) {
        auto something = node.find(i * 13);
        std::cout << "finding " << i * 13 << std::endl;
        ASSERT_TRUE(something.has_value());
        ASSERT_EQ(something, i);
      }
    }
  }

  TEST(node, notfound) {
    testnode_t node{true};

    node.insert(10, 1);
    node.insert(20, 2);
    node.insert(30, 3);
    EXPECT_NO_THROW({
      auto nothing = node.find(5);
      ASSERT_FALSE(nothing.has_value());
    });
  }

  TEST(node, notfound_then_found) {
    testnode_t node{true};

    node.insert(20, 2);
    node.insert(10, 1);
    node.insert(30, 3);
    EXPECT_NO_THROW({
      auto nothing = node.find(5);
      ASSERT_FALSE(nothing.has_value());
    });
    node.insert(5, 10);
    EXPECT_NO_THROW({
      auto something = node.find(5);
      ASSERT_TRUE(something.has_value());
      ASSERT_EQ(something, 10);
    });
    auto something = node.find(5);
    ASSERT_EQ(something, 10);
    something = node.find(20);
    ASSERT_EQ(something, 2);
    something = node.find(30);
    ASSERT_EQ(something, 3);
    something = node.find(10);
    ASSERT_EQ(something, 1);
  }
}
