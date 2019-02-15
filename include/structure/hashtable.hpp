#pragma once
#include <functional>
#include <cstdint>
#include <memory>

#include "logging.hpp"

namespace structure {
  namespace hashtable {

    template <typename hash_t, size_t size>
    class Storage {
      std::unique_ptr<hash_t[]> _array;
    public:
      Storage() :
        _array(std::make_unique<hash_t>(size))
      {
        TRACE << "DB Storage of " << size << " created";
      }
    };

    constexpr size_t storage_len = 1 << 14;

    template <auto hash, class Ret = std::decay_t<decltype(hash)> >
    class HashTable {
      std::function<Ret(const uint8_t * data, size_t size,
                        const Ret init)> _hash;
      Storage<Ret, storage_len> _storage;
    public:
      HashTable() : _hash(hash) {};
      ~HashTable() {
        DEBUG << "Destroying hash table";
      };
    };
  }
}
