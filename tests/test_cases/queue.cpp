#include "catch.hpp"
#include "fixtures.h"
#include "../src/queue.h"

TEST_CASE_METHOD(QueueFixture, "Queue/Push", "[queue][push]") {

    REQUIRE(queue != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    queue->push(root);

    CHECK_FALSE(queue->empty());
    CHECK(queue->size() == 1);
}

TEST_CASE_METHOD(QueueFixture, "Queue/Front", "[queue][front]") {

    REQUIRE(queue != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    queue->push(root);

    CHECK(queue->front() == root);
}

TEST_CASE_METHOD(QueueFixture, "Queue/Pop", "[queue][pop]") {

    REQUIRE(queue != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    queue->push(root);
    CHECK(queue->size() == 1);

    queue->pop();
    CHECK(queue->empty());
    CHECK(queue->size() == 0);
}

/**
                                SELECT

    Tests the queue's select function. First, it creates an artificial tree, where
    each node is the child of the last, and records what samples a prefix composed
    of all the nodes created would capture in a vector. Then, it adds the last node
    to the queue and calls select, which should return that node. It then checks
    that select returns all the necessary data correctly: the node, its prefix
    (including itself) and its prefix's captured vector.
    Afterwards, in order to test the select function's deletion of lazily marked
    nodes, the test calls delete subtree from the root's only child (whose descendants
    are the rest of the tree), and checks if all the nodes have been deleted except root, which
    should be unchanged, and the last node, which is a leaf node and should only be
    lazily marked for deletion. Then, it calls the queue'e select again, and
    checks if it deletes the leaf node marked for deletion and returns blank data.
**/
TEST_CASE_METHOD(QueueFixture, "Queue/Select", "[queue][select]") {

    REQUIRE(queue != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = root;
    int depth = nrules - 1;

    tracking_vector<unsigned short, DataStruct::Tree> prefix;

    // Vector of captured samples
    VECTOR captured_key;
    rule_vinit(nsamples, &captured_key);

    // As before, create a line of nodes
    for(int i = 0; i < depth; i++) {
        CAPTURE(i);

        n->set_done();
        n = tree->construct_node(i+1, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        tree->insert(n);

        prefix.push_back(i+1);

        // Here, we generated the vector of captured samples to then test against what the queue calculates
#ifdef GMP
        mpz_ior(captured_key, captured_key, rules[i+1].truthtable);
#else
        for(int j = 0; j < NENTRIES; j++) {
            CAPTURE(j);
            captured_key[j] = captured_key[j] | rules[i+1].truthtable[j];
        }
#endif
    }

    REQUIRE(tree->num_nodes() == (depth + 1));
    REQUIRE(n->depth() == depth);

    // Add n to the queue so we can select it
    queue->push(n);
    REQUIRE(queue->front() == n);

    VECTOR captured;
    rule_vinit(nsamples, &captured);

    // Test if selected a node returns it and its data correctly
    SECTION("Test select normal") {

        std::pair<Node*, tracking_vector<unsigned short, DataStruct::Tree>> prefix_node = queue->select(tree, captured);

        // Did it get the correct node from the queue?
        CHECK(prefix_node.first == n);

        // Did it get the node's correct prefix?
        CHECK(prefix_node.second == prefix);

        // Did it get the correct captured vector?
#ifdef GMP
        CHECK(mpz_cmp(captured, captured_key) == 0);
#else
        for(int i = 0; i < NENTRIES; i++) {
            CAPTURE(i);
            CHECK(captured[i] == captured_key[i]);
        }
#endif
    }

    // Test if selecting a node that is lazily marked deletes it and returns null
    SECTION("Test select lazy delete cleanup") {

        Node * t = root->child(1);
        REQUIRE(t != NULL);
        REQUIRE(t->done());

        // Instead of just setting n to deleted, make it with the delete_subtree function (as an extra check for the trie)
        root->delete_child(t->id());
        delete_subtree(tree, t, false, false);

        REQUIRE(tree->num_nodes() == 2);

        // Leaf nodes should be lazily marked, not deleted
        REQUIRE(n->deleted());

        std::pair<Node*, tracking_vector<unsigned short, DataStruct::Tree>> prefix_node = queue->select(tree, captured);

        // Was the node found to be lazily marked and deleted?
        CHECK(prefix_node.first == NULL);
        CHECK(tree->num_nodes() == 1);

        // No node was returned, so no prefix
        CHECK(prefix_node.second.size() == 0);

        // Is captured empty?
#ifdef GMP
        mpz_t temp;
        mpz_init2(temp, nsamples);

        CHECK(mpz_cmp(captured, temp) == 0);

        mpz_clear(temp);
#else
        for(int i = 0; i < NENTRIES; i++) {
            CAPTURE(i);
            CHECK(captured[i] == 0);
        }
#endif
    }

    rule_vfree(&captured);
    rule_vfree(&captured_key);
}
