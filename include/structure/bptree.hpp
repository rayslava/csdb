#pragma once
#include <cstdint>
#include <cassert>
#include <array>
#include <type_traits>
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

#if defined(_UNIT_TEST_BUILD)
      static void dumpNode(nodeptr_t node) {
        if (node->isLeaf()) {
          for (int i = 0; i < node->size(); i++) {
            std::cout << "k" << node->keys[i]
                      << "v" << std::get<value_t>(node->values[i]) << " ";
          }
        } else {
          for (int i = 0; i < node->size(); i++) {
            std::cout << "k" << node->keys[i] << " -> { ";
            dumpNode(std::get<nodeptr_t>(node->values[i]));
            std::cout << " } " << std::endl;
          }
          std::cout << "k" << node->keys[node->size() - 1] << " -> { ";
          dumpNode(std::get<nodeptr_t>(node->values[node->size()]));
          std::cout << " } " << std::endl;
        }
      }

      void dumpDot() {
        std::cout << std::endl << std::endl << "-----CUT------" << std::endl;
        std::cout << "digraph G {" << std::endl;
        dumpNode(this);
        std::cout << std::endl << "}";
        std::cout << std::endl << "-----CUT------" << std::endl;
      };
#endif

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
	std::ostringstream ss;
	for (int i = 0; i < current_node->size(); i++)
	  ss << current_node->keys[i] << " ";

	TRACE << "Searching " << key << " among (" << std::to_string(current_node->size()) << ") " << ss.str();
#endif
          for (int i = 0; i <= current_node->size(); i++) {
            if (i == current_node->size()) {
              current_node = std::get<nodeptr_t>(current_node->values[current_node->size()]);
#if defined(_UNIT_TEST_BUILD)
	      TRACE << "Going " << std::to_string(current_node->size());
#endif
              break;
            }
            // Scan all children but last
            if (key < current_node->keys[i]) {
              current_node = std::get<nodeptr_t>(current_node->values[i]);
#if defined(_UNIT_TEST_BUILD)
	      TRACE << "Going " << std::to_string(i);
#endif

              break;
            }
          }
        }
#if defined(_UNIT_TEST_BUILD)
	std::ostringstream ss;
	for (int i = 0; i < current_node->size(); i++)
	  ss << current_node->keys[i] << " ";

	TRACE << "Found node (" << std::to_string(current_node->size()) << ") with " << ss.str();
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
	std::ostringstream ss;
	for (int i = 0; i < node->size(); i++)
	  ss << node->keys[i] << " ";

	TRACE << "Splitting " << ss.str();
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
          newnode->status.size = node->size() - newsize; // Size of new node
	  newnode->status.isEmpty = false;

          if (node->isLeaf()) {                          // If this is leaf...
            newnode->values[newnode->size()] = node->values[node->size()]; // move old link
            node->values[newsize] = newnode;             // keep link to the righthand node
          }

          node->status.size = newsize;                   // Update size
          min_new = newnode->keys[0];                    // Key for new node in parent

#if defined(_UNIT_TEST_BUILD)
	std::ostringstream ss;
	for (int i = 0; i < node->size(); i++)
	  ss << node->keys[i] << " ";

	TRACE << "   Node(" << std::to_string(node->size()) << "): " << ss.str();

	ss.str("");
	ss.clear();
	for (int i = 0; i < newnode->size(); i++)
	  ss << newnode->keys[i] << " ";

	TRACE << "Newnode(" << std::to_string(newnode->size()) << "): " << ss.str();
#endif

        }

        {
          // Update parent node
          auto p = node->parent;

          if (!p) {
            // Have to split root node
            // newnode is a new right node and node is a new left node
            // just have to swap node with newleft
            auto newleft = new BPTNode<key_t, value_t, b_factor>{node->isLeaf(), node};

            for (int i = 0; i <= node->size(); i++) {
              newleft->keys[i] = node->keys[i];
              newleft->values[i] = node->values[i];
            }
            newleft->status.size = node->size();
            newleft->status.isLeaf = node->isLeaf();
            newleft->status.isEmpty = false;
            newleft->values[newleft->size()] = newnode;

            node->keys[0] = newnode->keys[0];
            node->values[0] = newleft;
            node->values[1] = newnode;
            node->status.size = 1;
            node->status.isLeaf = false;
            node->status.isEmpty = false;

#if defined(_UNIT_TEST_BUILD)
	std::ostringstream ss;
	for (int i = 0; i < newleft->size(); i++)
	  ss << newleft->keys[i] << " ";

	TRACE << " Newleft(" << std::to_string(newleft->size()) << "): " << ss.str();

	ss.str("");
	ss.clear();
	for (int i = 0; i < newnode->size(); i++)
	  ss << newnode->keys[i] << " ";

	TRACE << "Newright(" << std::to_string(newnode->size()) << "): " << ss.str();

	ss.str("");
	ss.clear();
	for (int i = 0; i < node->size(); i++)
	  ss << node->keys[i] << " ";

	TRACE << "  Parent(" << std::to_string(node->size()) << "): " << ss.str();

#endif

            return {newleft, newnode, key < node->keys[0]};
          }

          if (p->size() < b_factor) {
            assert(!p->isLeaf());
            // There's enough space to save link
            // Find a place of node link
            int i = 0, j = 0;
            while (max_old > p->keys[i] && i < p->size()) i++;  // Now i is place of current node

            // Move last value right
            p->values[p->size() + 1] = p->values[p->size()];
            // Move the rest data right
            for (j = p->size(); j > i; j--) {
              p->keys[j] = p->keys[j - 1];
              p->values[j] = p->values[j - 1];
            }
            j++; // Return pointer to new node

            // All keys left in \c node are less than \c min_new
            p->keys[i] = min_new;
            p->values[i] = node;

            // min_new <= newnode->keys[] < max_old
            //p->keys[j] = max_old;
            p->values[j] = newnode;
            p->status.size++;

#if defined(_UNIT_TEST_BUILD)
	std::ostringstream ss;
	for (int i = 0; i < p->size(); i++)
	  ss << p->keys[i] << " ";

	TRACE << "New parent(" << std::to_string(p->size()) << "): " << ss.str();
#endif

          } else {
            // Not enough space, splitting parent
            // For easier implementation let's place both new nodes to
            // single new parent node (right one)

#if defined(_UNIT_TEST_BUILD)
	std::ostringstream ss;
	for (int i = 0; i < p->size(); i++)
	  ss << p->keys[i] << " ";

	TRACE << "New parent required instead of (" << std::to_string(int(p->size())) << "): " << ss.str();
#endif

            auto [newleft, newright, smaller] = split(p, key);
	    // TODO: Fix this
            {
              // Add node to newleft
	      key_t newkey = node->keys[0];
              int i = 0, j = 0;
              while (newkey > newleft->keys[i] && i < newleft->size()) i++;

              // Move rest of keys and values to the right
              for (j = newleft->size(); j > i; j--) {
                newleft->keys[j] = newleft->keys[j - 1];
                newleft->values[j] = newleft->values[j - 1];
              }

              // Increase size
              newleft->status.size++;

              // Set actual value
              newleft->keys[i] = newkey;
              newleft->values[i] = node;
            }

            {
              // Add newnode to newright
	      key_t newkey = newnode->keys[0];
              int i = 0, j = 0;
              while (newkey > newright->keys[i] && i < newright->size()) i++;

              // Move rest of keys and values to the right
              for (j = newright->size(); j > i; j--) {
                newright->keys[j] = newright->keys[j - 1];
                newright->values[j] = newright->values[j - 1];
              }

              // Increase size
              newright->status.size++;

              // Set actual value
              newright->keys[i] = newkey;
              newright->values[i] = newnode;
            }

#if defined(_UNIT_TEST_BUILD)
	ss.str("");
	ss.clear();
	for (int i = 0; i < node->size(); i++)
	  ss << node->keys[i] << " ";

	TRACE << " Newleft(" << std::to_string(node->size()) << "): " << ss.str();

	ss.str("");
	ss.clear();
	for (int i = 0; i < newnode->size(); i++)
	  ss << newnode->keys[i] << " ";

	TRACE << "Newright(" << std::to_string(newnode->size()) << "): " << ss.str();

	ss.str("");
	ss.clear();
	for (int i = 0; i < newright->size(); i++)
	  ss << newright->keys[i] << " ";

	TRACE << "  Parent(" << std::to_string(newright->size()) << "): " << ss.str();

#endif
          }
        }

        return {node, newnode, key < min_new};
      }

      /** Internal function to insert \c value to appropriate \c key
       *
       * Boundary checks must be done before
       */
      static void do_insert(nodeptr_t node, key_t key, value_t value) {
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
	std::ostringstream ss;
	for (int i = 0; i < node->size(); i++)
	  ss << node->keys[i] << " ";

	TRACE << "Placed " << key << " to (" << std::to_string(node->size()) << ") " << ss.str();
#endif
      }

    public:
      BPTNode(bool leaf, nodeptr_t prt=nullptr) :
        status{leaf, true, 0},
        parent{prt}
      {
        TRACE << "Creating new " << (leaf ? "leaf" : "node") << " with parent " << prt;
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
