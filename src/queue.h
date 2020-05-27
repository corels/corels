#pragma once

#include "pmap.h"
#include "alloc.h"
#include <functional>
#include <queue>
#include <set>

// pass custom allocator function to track memory allocations in the queue
typedef std::priority_queue<Node*, tracking_vector<Node*, DataStruct::Queue>,
        std::function<bool(Node*, Node*)> > q;

// orders based on depth (BFS)
static std::function<bool(Node*, Node*)> base_cmp = [](Node* left, Node* right) {
    return left->depth() >= right->depth();
};

// orders based on curiosity metric.
static std::function<bool(Node*, Node*)> curious_cmp = [](Node* left, Node* right) {
    return left->get_curiosity() >= right->get_curiosity();
};

// orders based on lower bound.
static std::function<bool(Node*, Node*)> lb_cmp = [](Node* left, Node* right) {
    return left->lower_bound() >= right->lower_bound();
};

// orders based on objective.
static std::function<bool(Node*, Node*)> objective_cmp = [](Node* left, Node* right) {
    return left->objective() >= right->objective();
};

// orders based on depth (DFS)
static std::function<bool(Node*, Node*)> dfs_cmp = [](Node* left, Node* right) {
    return left->depth() <= right->depth();
};

class Queue {
    public:
        Queue(std::function<bool(Node*, Node*)> cmp, char const *type);
        // by default, initialize this as a BFS queue
        Queue() : Queue(base_cmp, "BFS") {};
        ~Queue();
        Node* front() {
            return q_->top();
        }
        inline void pop() {
            q_->pop();
        }
        void push(Node* node) {
            q_->push(node);
        }
        size_t size() {
            return q_->size();
        }
        bool empty() {
            return q_->empty();
        }
        inline char const * type() {
            return type_;
        }

        std::pair<Node*, tracking_vector<unsigned short, DataStruct::Tree> > select(CacheTree* tree, VECTOR captured) {
            int cnt;
            tracking_vector<unsigned short, DataStruct::Tree> prefix;
            Node *selected_node, *node;
            bool valid = true;
            double lb;
            do {
                selected_node = q_->top();
                q_->pop();
                if (tree->ablation() != 2)
                    lb = selected_node->lower_bound() + tree->c();
                else
                    lb = selected_node->lower_bound();
                logger->setCurrentLowerBound(lb);

                node = selected_node;
                // delete leaf nodes that were lazily marked
                if (node->deleted() || (lb >= tree->min_objective())) {
                    tree->decrement_num_nodes();
                    logger->removeFromMemory(sizeof(*node), DataStruct::Tree);
                    delete node;
                    valid = false;
                } else {
                    valid = true;
                }
            } while (!q_->empty() && !valid);
            if (!valid) {
                return std::make_pair((Node*)NULL, prefix);
            }

            rule_vclear(tree->nsamples(), captured);
            while (node != tree->root()) {
                rule_vor(captured,
                         captured, tree->rule(node->id()).truthtable,
                         tree->nsamples(), &cnt);
                prefix.push_back(node->id());
                node = node->parent();
            }
            std::reverse(prefix.begin(), prefix.end());
            return std::make_pair(selected_node, prefix);
        }

    private:
        q* q_;
        char const *type_;
};

extern void bbound_begin(CacheTree* tree, Queue* q);
extern void bbound_loop(CacheTree* tree, Queue* q, PermutationMap* p);
extern int bbound_end(CacheTree* tree, Queue* q, PermutationMap* p, bool early);

extern void evaluate_children(CacheTree* tree, Node* parent, tracking_vector<unsigned short, DataStruct::Tree> parent_prefix,
        VECTOR parent_not_captured, Queue* q, PermutationMap* p);
