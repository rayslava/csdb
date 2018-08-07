#include <gtest/gtest.h>

namespace csdb {
  void init();
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  csdb::init();
  return RUN_ALL_TESTS();
}
