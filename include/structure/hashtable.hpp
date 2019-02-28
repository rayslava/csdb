#pragma once
#include <functional>
#include <cstdint>
#include <memory>
#include <variant>
#include <optional>
#include <exception>
#include <type_traits>
#include <cstring>

#include "logging.hpp"
#include "tools/tmpl.hpp"

namespace structure {
  namespace hashtable {

    template <typename key_t, typename value_t>
    class Node {
      key_t _key;
      value_t _value = {};
      std::unique_ptr<Node<key_t, value_t> > _next = nullptr;

    public:
      Node(const key_t& key, const value_t& value) :
        _key(key),
        _value(value) {};

      Node(const key_t& key) :
        _key(key) {};

      value_t& operator=(const value_t& value) {
        _value = value;
        return _value;
      }

      operator value_t const& () const {
        return _value;
      }

      operator value_t& () {
        return _value;
      }

      value_t& get() {
        return _value;
      }

      const key_t& key() const {
        return _key;
      }

      std::optional<std::reference_wrapper<value_t> > find(const key_t& key) {
        auto node = this;
        do {
          if (node->_key == key)
            return std::optional<std::reference_wrapper<value_t> >{node->_value};
          node = node->_next.get();
        } while (node);
        return std::nullopt;
      }

      value_t& insert(const key_t& key) {
        auto node = std::unique_ptr<Node<key_t, value_t> >(new Node(key));
        node->_next = std::move(_next);
        _next = std::move(node);
        return _next->get();
      }

      /**
       * Return true when need to destroy bucket
       */
      bool erase(const key_t& key) {
        auto prev = this;
        auto node = this;
        do {
          if (node->_key == key) {
            if (node->_next) {
              node->_key = std::move(node->_next->_key);
              node->_value = std::move(node->_next->_value);
              node->_next = std::move(node->_next->_next);
              return false;
            } else {
              if (prev == node) {
                return true;
              } else {
                prev->_key = std::move(node->_key);
                prev->_value = std::move(node->_value);
                prev->_next = std::move(node->_next);
                return false;
              }
            }
          }
          prev = node;
          node = node->_next.get();
        } while (node);
        return false;
      }
    };

    template <typename key_t, typename value_t>
    class Bucket {
      enum class Type {
        Nothing,
        List
      };

      Type _type = Type::Nothing;
      std::unique_ptr<Node<key_t, value_t> > _first_node = nullptr;
    public:

      Bucket() {}

      value_t& set(const key_t& key) {
        switch (_type) {
        case Type::Nothing:
          _first_node = std::unique_ptr<Node<key_t, value_t> >(new Node<key_t, value_t>(key));
          _type = Type::List;
          return _first_node->get();
        case Type::List:
          if (auto&& val = _first_node->find(key))
            return val->get();
          else
            return _first_node->insert(key);
        default:
          throw std::out_of_range("Not found");
        }
      }

      value_t const& get(const key_t& key) const {
        switch (_type) {
        case Type::Nothing:
          throw std::out_of_range("Not found");
        case Type::List:
          if (auto&& val = _first_node->find(key))
            return val->get();
          else
            throw std::out_of_range("Not found");
        default:
          throw std::out_of_range("Not found");
        }
      }

      void erase(const key_t& key) {
        if (_first_node->erase(key)) {
          _first_node = nullptr;
          _type = Type::Nothing;
        }
      }
    };

    template <typename key_t, typename value_t, size_t size>
    class Storage {
      std::unique_ptr<std::array<Bucket<key_t, value_t>, size> > _array = std::make_unique<std::array<Bucket<key_t, value_t>, size> > ();

    public:
      Storage()
      {
        TRACE << "DB Storage of " << size << " created";
      }

      value_t& set(size_t idx, key_t key) {
        return (*_array)[idx].set(key);
      }

      const value_t& get(size_t idx, key_t key) const {
        return (*_array)[idx].get(key);
      }

      void erase(size_t idx, key_t key) {
        return (*_array)[idx].erase(key);
      }

    };

    constexpr size_t storage_len = 1 << 14;

    template <typename key_t,
              typename value_t,
              auto hash, class Ret = decltype(tmpl::ret(hash))>
    class HashTable {
      Storage<key_t, value_t, storage_len> _storage;
      std::function<Ret(const uint8_t * data,
                        size_t size,
                        const Ret init)> hashFunc = tmpl::toFunction(hash);

      template <typename T>
      Ret doHash(T key, tmpl::rank<0>) const {
        const uint8_t* data = &key;
        size_t size = sizeof(key);
        return hashFunc(data, size, 0);
      }

      template <typename T,
                std::enable_if_t<std::is_convertible_v<std::decay_t<T>, const char *> > * = nullptr>
      Ret doHash(T key, tmpl::rank<1>) const {
        auto data = reinterpret_cast<const uint8_t *>(key);
        size_t size = strlen(key);
        return hashFunc(data, size, 0);
      }

      template <typename T,
                std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> > * = nullptr>
      Ret doHash(const T& key, tmpl::rank<1>) const {
        const uint8_t* data = reinterpret_cast<const uint8_t *>(key.c_str());
        size_t size = key.length();
        return hashFunc(data, size, 0);
      }

      template <typename T>
      Ret doHash(T t) const {
        return doHash(t, tmpl::rank<1>{});
      }

    public:
      HashTable() {}
      ~HashTable() {
        DEBUG << "Destroying hash table";
      }

      operator const HashTable&() const {
        return *this;
      }

      const value_t& operator[] (const key_t& key) const {
        return _storage.get(doHash(key) % storage_len, key);
      }

      const value_t& at(const key_t& key) const {
        return _storage.get(doHash(key) % storage_len, key);
      }

      value_t& operator[] (const key_t& key) {
        return _storage.set(doHash(key) % storage_len, key);
      }

      void erase(const key_t& key) {
        return _storage.erase(doHash(key) % storage_len, key);
      }

    };
  }
}
