#include "logging.hpp"

#include <gtest/gtest.h>

namespace csdb {
  void init();
}

int main(int argc, char** argv)
{
  DEFAULT_LOGGING;
  DEFAULT_LOGGER_SEVERITY(logging::Severity::trace);
  TRACE << "Trace mode is on";

  ::testing::InitGoogleTest(&argc, argv);
  csdb::init();
  return RUN_ALL_TESTS();
}
