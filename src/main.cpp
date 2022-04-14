#include <iostream>
#include <functional>
#include <memory>
#include <thread>


#include "logging.hpp"
#include "structure/hashtable.hpp"
#include "structure/bptree.hpp"
#include "algo/crc64.hpp"

using namespace structure::hashtable;
using namespace structure::bptree;
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
    TRACE << "Trace mode is on";
#endif
    DEBUG << "DB init called";
  }
}
