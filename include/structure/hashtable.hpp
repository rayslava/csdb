#pragma once
#include <functional>
#include <cstdint>
#include <memory>
#include <variant>
#include <exception>
#include <type_traits>
#include <cstring>

#include "logging.hpp"
#include "tools/tmpl.hpp"

namespace structure {
  namespace hashtable {

    template <typename key_t, typename value_t>
    class LinkListNode {
      key_t _key;
      value_t _value;
      LinkListNode<key_t, value_t>* _next;
      LinkListNode<key_t, value_t>* _prev;
    public:
      LinkListNode(const key_t& key, const value_t& value) :
        _key(key),
        _value(value) {};
    };

    template <typename key_t, typename value_t>
    class LinkList {
      LinkListNode<key_t, value_t>* _head;
    public:
      LinkList() {};
      value_t& find (key_t key) {
        throw std::out_of_range(std::string("Not implemented yet for ") + key);
      }
      value_t& find (key_t key) const {
        throw std::out_of_range(std::string("Not implemented yet for ") + key);
      }
    };

    template <typename key_t, typename value_t, size_t size>
    class Storage {

      using cell_type = std::variant<value_t,
                                     LinkList<key_t, value_t>,
                                     std::nullptr_t>;

      std::unique_ptr<std::array<cell_type, size> > _array = std::make_unique<std::array<cell_type, size> > ();
      std::array<cell_type, size> arr = *_array;

      value_t& create_element() {
        return new value_t();
      }

      value_t& insert_empty(size_t idx) {
        arr[idx] = create_element();
        return arr[idx];
      }

    public:
      Storage()
      {
        TRACE << "DB Storage of " << size << " created";
        _array->fill(nullptr);
      }

      value_t& get(size_t idx, key_t key) {
        if (std::holds_alternative<std::nullptr_t>(arr[idx])) {
          throw std::out_of_range("Not found");
        } else if (std::holds_alternative<value_t>(arr[idx])) {
          return std::get<value_t>(arr[idx]);
        } else {
          auto list = std::get<LinkList<key_t, value_t> >(arr[idx]);
          return list.find(key);
        }
      }

      value_t& get(size_t idx, key_t key) const {
        if (std::holds_alternative<std::nullptr_t>(arr[idx])) {
          throw std::out_of_range("Not found");
        } else if (std::holds_alternative<value_t>(arr[idx])) {
          return std::get<value_t>(arr[idx]);
        } else {
          auto list = std::get<LinkList<key_t, value_t> >(arr[idx]);
          return list.find(key);
        }
      }
    };

    constexpr size_t storage_len = 1 << 14;

    // overload ranking
    template <unsigned int N>
    struct rank: rank<N - 1> { };

    template <>
    struct rank<0> { };

    template <typename key_t,
              typename value_t,
              auto hash, class Ret = decltype(tmpl::ret(hash))>
    class HashTable {
      Storage<key_t, value_t, storage_len> _storage;
      std::function<Ret(const uint8_t * data, size_t size,
                        const Ret init)> hash_func = tmpl::toFunction(hash);

      template <typename T>
      Ret do_hash(T key, rank<0>)  {
        const uint8_t* data = &key;
        size_t size = sizeof(key);
        return hash_func(data, size, 0);
      }

      template <typename T,
                std::enable_if_t<std::is_convertible_v<std::decay_t<T>, const char *> > * = nullptr>
      Ret do_hash(T key, rank<1>) {
        auto data = reinterpret_cast<const uint8_t *>(key);
        size_t size = strlen(key);
        return hash_func(data, size, 0);
      }

      template <typename T,
                std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> > * = nullptr>
      Ret do_hash(const T& key, rank<1>) {
        const uint8_t* data = key.c_str();
        size_t size = key.length();
        return hash_func(data, size, 0);
      }

      template <typename T>
      Ret do_hash(T t) {
        return do_hash(t, rank<1>{});
      }

    public:
      HashTable() {};
      ~HashTable() {
        DEBUG << "Destroying hash table";
      };
      value_t& operator[] (const key_t& key) {
        return _storage.get(do_hash(key) % storage_len, key);
      };
      const value_t& operator[] (const key_t& key) const {
        return _storage.get(do_hash(key) % storage_len, key);
      };
    };
  }
}
