#include <iostream>
#include "logging.hpp"

namespace csdb {
  /**
   * Initialization of database engine
   */
#ifndef _UNIT_TEST_BUILD
  __attribute__((constructor))
  static
#endif
  void init() {
#ifdef _UNIT_TEST_BUILD
    DEFAULT_LOGGING;
    DEFAULT_LOGGER_SEVERITY(logging::Severity::trace);
#endif
    DEBUG << "DB init called";
  }
}
