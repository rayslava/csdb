#include <iostream>
#include <functional>
#include <memory>

#include "logging.hpp"
#include "structure/hashtable.hpp"
#include "algo/crc64.hpp"

using namespace structure::hashtable;
using namespace algo::hash;

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
    auto ht = std::unique_ptr<HashTable<std::string, int, crc64> >{};
  }
}
