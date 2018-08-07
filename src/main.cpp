#include <iostream>

namespace csdb {
  /**
   * Initialization of database engine
   */
#ifndef _UNIT_TEST_BUILD
  __attribute__((constructor))
  static
#endif
  void init() {
    std::cout << "DB init called";
  }

}
