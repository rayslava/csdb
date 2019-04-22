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
      std::array<key_t, b_factor> _keys = {};    /**< Keys stored in the node */
      Node* _next = nullptr;                     /**< Link to right node */
      Node* _parent = nullptr;                   /**< Link to parent node (for splitting) */

      virtual ~Node() {};
    };

    template <typename key_t, uint8_t b_factor>
    struct LayerNode: public Node<key_t, b_factor> {
      /** Links for next nodes

          One for right link from biggest key
       */
      std::array<std::unique_ptr<Node<key_t, b_factor> >, b_factor + 1> _links = {};
      virtual ~LayerNode() {};
    };

    template <typename key_t, typename value_t, uint8_t b_factor>
    struct Leaf: public Node<key_t, b_factor> {
    public:
      std::array<value_t, b_factor> _data = {};

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

      virtual ~Leaf() {};
    };

    template <typename key_t, typename value_t, uint8_t b_factor>
    class BPTree {
      LayerNode<key_t, b_factor> _root = {};

      Leaf<key_t, value_t, b_factor>& find_node(key_t key,
                                                Node<key_t, b_factor>& start) {
        if (start.flags.isLeaf)
          return static_cast<Leaf<key_t, value_t, b_factor>&>(start);

        auto& node = static_cast<LayerNode<key_t, b_factor>&>(start);

        for (uint8_t ind = 0; ind < b_factor; ind++) {

	  /* This key is last in this node */
          if (ind == node.flags.count) {
            auto& right_node = *node._links[ind + 1];
            return find_node(key, right_node);
          }

	  /* This key is in the left subnode */
          if (key < node._keys[ind]) {
            auto& left_node = *node._links[ind];
            return find_node(key, left_node);
          }

	  /* This key should be in thid node */
          if (key > node._keys[ind] && key < node._keys[ind + 1]) {
            auto& right_node = *node._links[ind + 1];
            return find_node(key, right_node);
          }
        }

        throw std::logic_error("Implementation error in Tree");
      }

    public:
      BPTree() {
        for (int link_ind = b_factor - 1; link_ind >= 0; link_ind--) {
          auto& current = _root._links[link_ind];
          current = std::move(std::make_unique<Leaf<key_t, value_t, b_factor> >());
	  current->_parent = &_root;
          current->_next = link_ind < b_factor - 1 ?
                           _root._links[link_ind + 1].get() :
                           nullptr;
        }
      }

      value_t& get(key_t key) {
        auto& result_node = find_node(key, _root);
        for (int i = 0; i < b_factor; i++)
          if (result_node._keys[i] == key)
            return result_node._data[i];
        throw std::out_of_range("Key not found");
      }

      void split(LayerNode<key_t, b_factor>& node,
		 std::unique_ptr<Leaf<key_t, value_t, b_factor>> newLeaf) {
	if (node.flags.count < b_factor + 1) {
	  // We have place for one more node, just need to find place
	  int newNodeIndex = 0;
	  for (; newNodeIndex < b_factor; newNodeIndex++)
	    if (node._keys[newNodeIndex] > newLeaf->_keys[0])
	      break;

	  // Move rest of keys to find a place
	  for (int i = node.flags.count; i >= newNodeIndex; i--) {
	    node._keys[i] = std::move(node._keys[i-1]);
	    node._values[i] = std::move(node._values[i-1]);
	  }

	  node._keys[newNodeIndex] = newLeaf->_keys[0];
	  newLeaf->_parent = node;
	  node._values[newNodeIndex] = std::move(newLeaf);
	} else {
	  // Split the layernode too
	}
      }

      /** Splits the \c oldLeaf from \c index */
      void split(Leaf<key_t, value_t, b_factor>& oldLeaf, int index) {
	// Create new leaf to keep data
        auto &newLeaf = std::make_unique<Leaf<key_t, value_t, b_factor>>();

        if (index == 0) {
	  // Just create new node to the left
        } else if (index == b_factor - 1) {
	  // Just create new node to the right
	} else {
	  // Move data from index to count
	  for (int i = 0; i < index; ++i) {
	    newLeaf->_data[i] = std::move(oldLeaf->_data[i]);
	  }
	  // Correct count of data in old node
	  oldLeaf.flags.count -= (b_factor - index);

	  // Set up data in new node
	  newLeaf->flags.count = (b_factor - index);
	  newLeaf->flags.isEmpty = false;
        }

	// Now process the parent nodes
	split(*oldLeaf._parent, std::move(newLeaf));
      }

      void insert(key_t key, value_t value) {
        auto& result_node = find_node(key, _root);
	// This key is in tree already, so we know where to place key
        for (int i = 0; i < b_factor; i++)
          if (result_node._keys[i] == key) {
            result_node._data[i] = value;
	  }

	if (result_node._keys[0] > key) {
	  // Key should be first in this node

	  // And there's place for new one
	  if (result_node.flags.count < b_factor - 1) {
	    for (int i = result_node.flags.count; i >= 0; i--)
	      result_node._keys[i] = result_node._keys[i-1];
	    result_node._keys[0] = key;
	    result_node.flags.count++;
	  }

	  // TODO: split from 0

	} else if (result_node._keys[b_factor - 1] < key) {
	  // Key should be last in the node

	  // And there's place for new one
	  if (result_node.flags.count < b_factor - 1) {
	    result_node._keys[result_node.flags.count] = key;
	    result_node.flags.count++;
	  }

	  // TODO: split from last
	} else {
	  // Key should be somewhere in the middle

	}
      }
    };
  }
}
