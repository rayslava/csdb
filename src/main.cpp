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
    //    auto ht = std::make_unique<HashTable<std::string, int, crc64> >();
        auto ind = std::make_unique<BPTNode<uint64_t, int, 3>>(true);
	for (int i = 20; i > 0; i--) {
	  ind->insert(i * 10, i);
	  std::this_thread::sleep_for(std::chrono::milliseconds(200));
	  TRACE << *ind.get();
	}
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    auto something = ind->find(7*10);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    TRACE << "Finished: " << something.has_value();
    TRACE << something.value();
  }
}
