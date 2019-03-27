#pragma once
#include <cstdint>
#include <array>
#include <type_traits>
#include <variant>
#include <algorithm>
#include <exception>

#include "logging.hpp"
#include "tools/tmpl.hpp"

namespace structure {
  namespace bptree {
    template <typename key_t, uint8_t b_factor>
    struct Node {
      struct {
        bool isLeaf : 1;
        bool isEmpty : 1;
        uint8_t count : 8;
      } flags;
      std::array<key_t, b_factor> _keys;         /**< Keys stored in the node */
      Node* _next;                               /**< Link to right node */
    };

    template <typename key_t, uint8_t b_factor>
    struct LayerNode: public Node<key_t, b_factor> {
      /** Links for next nodes

          One for right link from biggest key
       */
      std::array<LayerNode<key_t, b_factor> *, b_factor + 1> _links;
    };

    template <typename key_t, typename value_t, uint8_t b_factor>
    struct Leaf: public Node<key_t, b_factor> {
    public:
      std::array<value_t, b_factor> _data;

      Leaf() {
        this->flags.isLeaf = true;
        this->flags.isEmpty = true;
        this->flags.count = 0;
      }

      value_t& getValue(key_t key) {
        const auto& location = std::find(this->_keys.cbegin(), this->_keys.cend(), key);
        if (location != this->_keys.cend()) {
          const auto index = std::distance(this->_keys.cbegin(), location);
          return _data[index];
        } else {
          throw std::out_of_range("Not such value in node");
        }
      }
    };

    template <typename key_t, typename value_t, uint8_t b_factor>
    class BPTree {
      LayerNode<key_t, b_factor> _root = {};

      Leaf<key_t, value_t, b_factor>& do_search(key_t key,
                                                Node<key_t, b_factor>& start) {
        if (start.flags.isLeaf)
          return static_cast<Leaf<key_t, value_t, b_factor>&>(start);

        auto& node = static_cast<LayerNode<key_t, b_factor>&>(start);

        uint8_t found = 0;
        for (uint8_t ind = 0; ind < b_factor; ind++) {
          if (ind == node.flags.count) {
            auto& right_node = *node._links[ind + 1];
            return do_search(key, right_node);
          }

          if (key < node._keys[ind]) {
            auto& left_node = *node._links[ind];
            return do_search(key, left_node);
          }

          if (key > node._keys[ind] && key < node._keys[ind + 1]) {
            auto& right_node = *node._links[ind + 1];
            return do_search(key, right_node);
          }
        }
        throw std::out_of_range("Key not found");
      }

    public:
      value_t& insert(key_t key) {
        auto& result_node = do_search(key, _root);
        return result_node._data[0];
      }
    };
  }
}
