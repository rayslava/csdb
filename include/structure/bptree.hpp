#pragma once
#include <cstdint>
#include <cassert>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <optional>
#include <algorithm>
#include <exception>
#if defined(_UNIT_TEST_BUILD)
#include <gtest/gtest_prod.h>
#include <iostream>
#include <sstream>
#endif

#include "logging.hpp"
#include "tools/tmpl.hpp"

namespace structure {
  namespace bptree {

    /** Generic tree node class
     *
     * All other tree note classes inherit from here
     */
    template <typename key_t, typename value_t, int b_factor>
    class BPTNode {
      using nodeptr_t = BPTNode<key_t, value_t, b_factor> *;
      using linkcell_t = std::variant<std::monostate, value_t, nodeptr_t>;

      struct {
        bool isLeaf : 1;
        bool isEmpty : 1;
        uint8_t size : 8;
      } status;

      /** Return size of node */
      uint8_t size() const { return status.size; };

      /** Return true if node is leaf */
      bool isLeaf() const { return status.isLeaf; };

      /** Return true if node is empty */
      bool empty() const { return status.isEmpty; };

      nodeptr_t parent = nullptr;
      key_t keys[b_factor] = {};            /**< Keys stored in the node */

      /** Links either to data or to next BPTNode
       *
       * When \c isLeaf is \c true contains \c value_t otherwise contains link
       * to next \c BPTNode
       */
      linkcell_t values[b_factor + 1] = {};

      static nodeptr_t findLeaf(nodeptr_t current_node, const key_t& key) {
        while (!current_node->isLeaf()) {
#if defined(_UNIT_TEST_BUILD)
          TRACE << "Searching " << key << " in " << current_node;
#endif
          for (int i = 0; i <= current_node->size(); i++) {
            if (i == current_node->size()) {
              current_node = std::get<nodeptr_t>(current_node->values[current_node->size()]);
#if defined(_UNIT_TEST_BUILD)
              TRACE << "Going " << current_node;
#endif
              break;
            }
            // Scan all children but last
            if (key < current_node->keys[i]) {
              current_node = std::get<nodeptr_t>(current_node->values[i]);
#if defined(_UNIT_TEST_BUILD)
              TRACE << "Going " << current_node;
#endif

              break;
            }
          }
        }
#if defined(_UNIT_TEST_BUILD)
        TRACE << "Found node: " << current_node;
#endif
        return current_node;
      }

      /** Scan values of leaf node */
      static linkcell_t scanValues(nodeptr_t current_node, const key_t& key) {
        assert(current_node->isLeaf());
        for (int i = 0; i < current_node->size(); i++)
          if (current_node->keys[i] == key)
            return current_node->values[i];
        return std::monostate{};
      }

      /** Split \c node recursively and return node to place \c key
       *
       * Returns: new left node, new right node, true if insertion is required
       * to left.
       **/
      static
      std::tuple<nodeptr_t, nodeptr_t, bool>
      split(nodeptr_t node, key_t key) {
        // Save maximal old key for further search
        const key_t max_old = node->keys[node->size() - 1];
#if defined(_UNIT_TEST_BUILD)
        TRACE << "Splitting " << node;
#endif
        // Create now node rightside, and keep parent for both
        auto newnode = new BPTNode<key_t, value_t, b_factor>{node->isLeaf(), node->parent};
        key_t min_new;

        {
          // Move half of values to new node, newnode is right (bigger) one
          int newsize = node->size() / 2;
          int i, j;
          for (i = newsize, j = 0; i <= node->size(); i++, j++) {
            newnode->keys[j] = node->keys[i];
            newnode->values[j] = node->values[i];
          }
          newnode->status.size = node->size() - newsize + (node->isLeaf()?0:1); // Size of new node
          newnode->status.isEmpty = false;

          min_new = newnode->keys[0];                    // Key for new node in parent

          if (node->isLeaf()) {                          // If this is leaf...
            newnode->values[newnode->size()] = node->values[node->size()]; // move old link
            node->values[newsize] = newnode;             // keep link to the righthand node
          } else {                                       // If this is node...
            for (i = 1; i <= newnode->size(); i++) {     // delete 1st element for deduplication
              newnode->keys[i - 1] = newnode->keys[i];
              newnode->values[i - 1] = newnode->values[i];
              newnode->status.size--;
            }
          }
          node->status.size = newsize;                   // Update size

#if defined(_UNIT_TEST_BUILD)
          TRACE << "   Node " << node;
          TRACE << "Newnode " << newnode;
          TRACE << "min_new " << min_new;
#endif
        }

        {
          // Update parent node
          auto p = node->parent;
#if defined(_UNIT_TEST_BUILD)
          TRACE << "Updating parent node " << p;
#endif
          if (!p) {
            TRACE << "Splitting root node";
            // Have to split root node
            // newnode is a new right node and node is a new left node
            // just have to swap node with newleft
            auto newleft = new BPTNode<key_t, value_t, b_factor>{node->isLeaf(), node->parent};

            for (int i = 0; i <= node->size(); i++) {
              newleft->keys[i] = node->keys[i];
              newleft->values[i] = node->values[i];
            }
            newleft->status.size = node->size();
            newleft->status.isLeaf = node->isLeaf();
            newleft->status.isEmpty = false;

            node->keys[0] = min_new;
            node->values[0] = newleft;
            node->values[1] = newnode;
            node->status.size = 1;
            node->status.isLeaf = false;
            node->status.isEmpty = false;

#if defined(_UNIT_TEST_BUILD)
            TRACE << " Newleft " << newleft;
            TRACE << " Newright " << newnode;
            TRACE << " min_new " << min_new;
            TRACE << "  Parent " << node;
#endif

            return {newleft, newnode, key < min_new};
          }

          // Split one leaf into two in existing node
          if (p->size() < b_factor) {
#if defined(_UNIT_TEST_BUILD)
            TRACE << "Adding to parent " << p;
#endif
            assert(!p->isLeaf());
            // There's enough space to save link
            // Find a place of node link
            int i = 0, j = 0;
            while (min_new > p->keys[i] && i < p->size()) i++;  // Now i is place of current node

            // Move last value right
            p->values[p->size() + 1] = p->values[p->size()];
            // Move the rest data right
            for (j = p->size(); j > i; j--) {
              p->keys[j] = p->keys[j - 1];
              p->values[j] = p->values[j - 1];
            }
            p->status.size++;

            // All keys left in \c node are less than \c newnode->keys[0]
            p->keys[i] = newnode->keys[0];
            p->values[i] = node;

            // min_new <= newnode->keys[] < max_old
            //            p->keys[j] = max_old;
            p->values[i + 1] = newnode;

#if defined(_UNIT_TEST_BUILD)
            TRACE << "New parent " << p;
#endif
          } else {
            // Not enough space, splitting parent
            // For easier implementation let's place both new nodes to
            // single new parent node (right one)

            // node and newnode contain 2 new leafs

#if defined(_UNIT_TEST_BUILD)
            TRACE << "Splitting parent " << p;
#endif

            auto [newleft, newright, left] = split(p, key);
            TRACE << " node " << node;
            TRACE << " newnode " << newnode;
            TRACE << " min_new " << min_new;
            TRACE << " newleft " << newleft;
            TRACE << " newright " << newright;
            TRACE << " p " << p;

            {
              auto& target = left?newleft:newright;
              int i = 0, j = 0;
              while (min_new > target->keys[i] && i <= target->size()) i++;
              // Move rest of keys and values to the right
              for (j = target->size() + 1; j > i; j--) {
                target->keys[j] = target->keys[j - 1];
                target->values[j] = target->values[j - 1];
              }
              // Increase size
              target->status.size += 1;
              target->status.isEmpty = false;

              // Set actual value
              target->keys[i] = min_new;
              target->values[i] = node;
              target->values[i + 1] = newnode;
            }

            TRACE << " newleft " << newleft;
            TRACE << " newright " << newright;
            TRACE << " p " << p;
          }
        }

        return {node, newnode, key < min_new};
      }

      /** Internal function to insert \c value to appropriate \c key
       *
       * Boundary checks must be done before
       */
      template <typename value_type>
      static void do_insert(nodeptr_t node, key_t key, value_type value) {
#ifndef NDEBUG
        assert(node);
        if (node->isLeaf())
          assert(typeid(value) == typeid(value_t));
        if (!node->isLeaf())
          assert(typeid(value) == typeid(nodeptr_t));
#endif
        int i = 0, j = 0;
        while (key > node->keys[i] && i < node->size()) i++;

        // If we're appending to end we have to save link
        if (node->isLeaf())
          node->values[node->size() + 1] = node->values[node->size()];

        // Move rest of keys and values to the right
        for (j = node->size(); j > i; j--) {
          node->keys[j] = node->keys[j - 1];
          node->values[j] = node->values[j - 1];
        }

        // Increase size
        node->status.size++;
        node->status.isEmpty = false;

        // Set actual value
        node->keys[i] = key;
        node->values[i] = value;

#if defined(_UNIT_TEST_BUILD)
        TRACE << "Placed '" << key << ":" << value << "' to " << node;
#endif
      }

    public:
#if defined(_UNIT_TEST_BUILD)
      operator std::string() const {
        char buf[19];
        snprintf(buf, 19, "0x%lx", reinterpret_cast<uint64_t>(reinterpret_cast<const void *>(this)));
        std::string result{
          std::string("(") + std::string(isLeaf() ? "l" : "n") + std::to_string(size()) + " " + std::string(buf) + " " };
        if (isLeaf()) {
          std::ostringstream ss;
          ss << "[ ";
          for (int i = 0; i < size(); i++)
            ss << keys[i] << " ";
          ss << "]";
          result += ss.str();
        } else {
          if (!empty()) {
            for (int i = 0; i < size(); i++) {
              std::string contents = *std::get<nodeptr_t>(values[i]);
              result += "(<" + std::to_string(keys[i]) + " " + contents + ") ";
            }
            std::string contents = *std::get<nodeptr_t>(values[size()]);
            result +=
              "(>" + std::to_string(keys[size() - 1]) + " " + contents + ")";
          }
        }
        result += ")";
        return result;
      }

      friend std::ostream& operator<< (std::ostream& out, const nodeptr_t& p) {
        if (!p)
          return out << "@0x0";
        std::string newline = *p;
        return out << newline;
      }

      friend std::ostream& operator<< (std::ostream& out, const BPTNode<key_t, value_t, b_factor>& n) {
        std::string newline = n;
        return out << newline;
      }
#endif
      BPTNode(bool leaf, nodeptr_t prt=nullptr) :
        status{leaf, true, 0},
        parent{prt}
      {
        static_assert(b_factor > 2);
        TRACE << "New " << (leaf ? "l" : "n") << " prt " << prt;
      };

      ~BPTNode() {
        // Remove child nodes if not leaf
        if (!isLeaf())
          for (int i = 0; i <= size(); i++)
            delete std::get<nodeptr_t>(values[i]);
      }

      std::optional<value_t> find(key_t key) {
        TRACE << "Searching " << key;
        if (isLeaf()) {
          // Scan keys to find appropriate value
          auto result = scanValues(this, key);
          if (!std::holds_alternative<std::monostate>(result))
            return std::get<value_t>(result);
        } else {
          // Dig inside, until we find a leaf
          auto leaf = findLeaf(this, key);
          if (!leaf->isLeaf())
            return {};
          auto result = scanValues(leaf, key);
          if (!std::holds_alternative<std::monostate>(result))
            return std::get<value_t>(result);
        }
        return {};
      }

      void insert(key_t key, value_t value) {
        TRACE << "Inserting " << key << ":" << value;
        // Find a leaf to insert key to
        auto node = findLeaf(this, key);
        assert(node->isLeaf());

        if (node->size() < b_factor) {
          // We have enough place to insert
          do_insert(node, key, value);
        } else {
          // Have to split node
          auto [newleft, newright, left] = split(node, key);
          do_insert(left ? newleft : newright, key, value);
        }
      }

#if defined(_UNIT_TEST_BUILD)
    private:
      FRIEND_TEST(node, construct3);
      FRIEND_TEST(node, small_split);
      FRIEND_TEST(node, root_split);
      FRIEND_TEST(node, random_insert);
      FRIEND_TEST(node, overfulling_insert);
#endif
    };
  }
}
