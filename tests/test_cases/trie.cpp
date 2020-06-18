#include "catch.hpp"
#include "fixtures.h"
#include "../src/queue.h"

/**
                            TEST TREE INITIALIZATION

    This test simply tests whether or not the initialization of the tree
    was successful: first by checking if all of the attributes that were passed
    to its constructor were correctly stored by the tree, and then by checking
    that it has the correct rule, label, and minority info by simply looping
    through all the rules, labels and minorities that the tree thinks there are
    and comparing them to the actual data that was loaded and stored in tests-main.cc.
    Also, some attributes of the root node are checked.
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Test trie initialization", "[trie][trie_init]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    SECTION("Check initialization") {

        CHECK(tree->num_nodes() == 1);
        CHECK(tree->num_evaluated() == 0);
        CHECK(tree->c() == c);
        CHECK(tree->nsamples() == nsamples);
        CHECK(tree->nrules() == nrules);
        CHECK(tree->ablation() == ablation);
        CHECK(tree->has_minority() == (bool)minority);
        CHECK(tree->calculate_size() == calculate_size);
    }

    SECTION("Test root initialization") {

        // In the labels file, the majority has label 1, so the default prediction should be true
        CHECK(root->default_prediction());

        CHECK(root->objective() == (double)labels[0].support / (double)nsamples);
        CHECK(root->equivalent_minority() == Approx((double)minority[0].support / (double)nsamples));
    }

    SECTION("Test rules") {

        for(int i = 0; i < nrules; i++) {
            CAPTURE(i);
            CHECK(tree->rule(i).support == rules[i].support);
            CHECK(tree->rule(i).cardinality == rules[i].cardinality);
            CHECK(std::string(tree->rule(i).features) == std::string(rules[i].features));
            CHECK(std::string(tree->rule_features(i)) == std::string(rules[i].features));

#ifdef GMP
            CHECK(mpz_cmp(tree->rule(i).truthtable, rules[i].truthtable) == 0);
#else
            for(int j = 0; j < NENTRIES; j++) {
                CAPTURE(j);
                CHECK(tree->rule(i).truthtable[j] == rules[i].truthtable[j]);
            }
#endif
        }
    }

    SECTION("Test labels") {

        for(int i = 0; i < nlabels; i++) {
            CAPTURE(i);
            CHECK(tree->label(i).support == labels[i].support);
            CHECK(tree->label(i).cardinality == labels[i].cardinality);
            CHECK(std::string(tree->label(i).features) == std::string(labels[i].features));

#ifdef GMP
            CHECK(mpz_cmp(tree->label(i).truthtable, labels[i].truthtable) == 0);
#else
            for(int j = 0; j < NENTRIES; j++) {
                CAPTURE(j);
                CHECK(tree->label(i).truthtable[j] == labels[i].truthtable[j]);
            }
#endif
        }
    }

    SECTION("Test minority") {

        if(minority != NULL) {
            for(int i = 0; i < nminority; i++) {
                CAPTURE(i);
                CHECK(tree->minority(i).support == minority[i].support);
                CHECK(tree->minority(i).cardinality == minority[i].cardinality);
                CHECK(std::string(tree->minority(i).features) == std::string(minority[i].features));

#ifdef GMP
                CHECK(mpz_cmp(tree->minority(i).truthtable, minority[i].truthtable) == 0);
#else
                for(int j = 0; j < NENTRIES; j++) {
                    CAPTURE(j);
                    CHECK(tree->minority(i).truthtable[j] == minority[i].truthtable[j]);
                }
#endif
            }
        }
    }
}

/**
                            CONSTRUCT AND INSERT NODE

    This test constructs a node (with the root as a parent), checks if the node
    has all of its attributes stored correctly, inserts it into the tree, and
    checks if the insertion was succesful and the heirachy is correctly stored
    (the root knows it has it a child, the node knows it has the root as parent)
**/

TEST_CASE_METHOD(TrieFixture, "Trie/Construct node", "[trie][construct_node]") {

    Node * parent = root;

    REQUIRE(tree != NULL);
    REQUIRE(parent != NULL);

    unsigned short rule_id = 1;
    bool prediction = true;
    bool default_prediction = true;
    double lower_bound = 0.1;
    double objective = 0.12;
    int num_not_captured = 5;
    int len_prefix = 0;
    double equivalent_minority = 0.1;

    Node * n = tree->construct_node(rule_id, nrules, prediction, default_prediction,
                                    lower_bound, objective, parent,
                                    num_not_captured, nsamples, len_prefix,
                                    c, equivalent_minority);

    // Was node created?
    REQUIRE(n != NULL);

    SECTION("Test attributes") {

        CHECK(n->id() == rule_id);
        CHECK(n->prediction() == prediction);
        CHECK(n->default_prediction() == default_prediction);
        CHECK(n->lower_bound() == lower_bound);
        CHECK(n->objective() == objective);
        CHECK(n->num_captured() == (nsamples - num_not_captured));
        CHECK(n->depth() == (len_prefix + 1));
        CHECK(n->equivalent_minority() == equivalent_minority);
        CHECK_FALSE(n->deleted());
        CHECK(n->depth() == 1);
    }

    SECTION("Test insert (hierachy)") {

        tree->insert(n);

        // Root + this node = 2
        CHECK(tree->num_nodes() == 2);

        // Check heirarchy of tree
        CHECK(n->parent() == parent);
        CHECK(parent->children_begin()->second == n);
        CHECK(parent->num_children() == 1);
        CHECK(parent->child(rule_id) == n);
    }
}

TEST_CASE_METHOD(TrieFixture, "Trie/Check node delete behavior", "[trie][delete_node]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = tree->construct_node(T_NEW_RULE, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, T_PARENT, T_NNC, T_NSAMPLES, T_LPREFIX, T_C, T_MINOR);

    REQUIRE(n != NULL);

    tree->insert(n);

    // Check if the deleted behavior actually works
    n->set_deleted();
    CHECK(n->deleted());

    // Check if delete_child alters the tree correctly
    root->delete_child(1);

    CHECK(root->num_children() == 0);
}

/**
                        NODE GET PREFIX AND PREDICTIONS

    Creates a artificial tree with nrules - 1 (the actual number of rules) nodes
    (not including root, and each being the child of the last), with ids 1, 2, 3, etc.,
    and alternating predictions (first has false, second has true, etc.) and
    random data for the rest. As it is created, the prefix and predictions are
    stored in arrays which are then checked against what the last node's
    get_prefix_and_predictions function gets, to see if the function has the
    expected behavior.
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Node get prefix and predictions", "[trie][node_prefix]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = root;
    int depth = nrules - 1;

    tracking_vector<unsigned short, DataStruct::Tree> prefix;
    tracking_vector<bool, DataStruct::Tree> predictions;

    // Create a nrules-deep tree and store its prefix and predictions
    for(int i = 0; i < depth; i++) {
        CAPTURE(i);
        n = tree->construct_node(i+1, T_NRULES, (bool)(i % 2), T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        tree->insert(n);

        prefix.push_back(i+1);
        predictions.push_back((bool)(i % 2));
    }

    // depth is the number of nodes added, then add root
    REQUIRE(tree->num_nodes() == (depth + 1));
    REQUIRE(n->depth() == depth);

    std::pair<tracking_vector<unsigned short, DataStruct::Tree>, tracking_vector<bool, DataStruct::Tree>> p =
        n->get_prefix_and_predictions();

    // Check that the node correctly determines its prefix and predictions
    CHECK(p.first == prefix);
    CHECK(p.second == predictions);
}

TEST_CASE_METHOD(TrieFixture, "Trie/Increment num evaluated", "[trie][num_eval]") {

    REQUIRE(tree != NULL);

    size_t num = tree->num_evaluated();
    tree->increment_num_evaluated();

    REQUIRE(tree->num_evaluated() == (num + 1));
}

TEST_CASE_METHOD(TrieFixture, "Trie/Decrement num nodes", "[trie][num_nodes]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    // Root
    REQUIRE(tree->num_nodes() == 1);

    tree->decrement_num_nodes();

    REQUIRE(tree->num_nodes() == 0);
}

TEST_CASE_METHOD(TrieFixture, "Trie/Update minimum objective", "[trie][min_obj]") {

    REQUIRE(tree != NULL);

    double min0 = tree->min_objective();
    double min1 = min0 + 0.01;

    tree->update_min_objective(min1);

    REQUIRE(tree->min_objective() == min1);
}

/**
                                PRUNE UP

    Creates an artificial tree, where the root has one childless child and also
    a second child that has in turn nrules - 2 children (not including itself),
    with each one being the child of the last. Then, the tree is first pruned up from
    the deepest child of that subtree, and then from the root's childless child,
    and the behavior of those two calls is examined.
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Prune up", "[trie][prune_up]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = root;
    int depth = nrules - 1;

    // Create one childless child of the root
    Node * s = tree->construct_node(2, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, T_LPREFIX, T_C, T_MINOR);
    tree->insert(s);

    // Then create a deep line of nodes from another child of the root
    for(int i = 0; i < depth; i++) {
        CAPTURE(i);
        n = tree->construct_node(i+1, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        tree->insert(n);
    }

    REQUIRE(tree->num_nodes() == (depth + 2));
    REQUIRE(n->depth() == depth);

    tree->prune_up(n);

    // Prune up should have deleted all the nodes with one child that are
    // ancestors of n, which does not include root (multiple children) or s (not an ancestor)
    CHECK(tree->num_nodes() == 2);

    tree->prune_up(s);

    // Pruning up s should have deleted s and decremented for root, so no more nodes
    CHECK(tree->num_nodes() == 0);

    // But root should still exist (not actually deleted)
    CHECK(root->id() == 0);
}

/**
                                CHECK PREFIX

    Creates an artificial tree with nrules - 1 nodes (not including root), with
    each node the child of the last, and store the prefix of all the rule ids
    in order in a vector. Then, check if the tree correctly finds the prefix created by
    calling check_prefix. As follow-up checks, change the last id of the
    prefix vector and make sure that the tree doesn't find the prefix now, and
    also delete the node corresponding to the last id in the prefix vector from
    the tree and make sure that the tree doesn't find the prefix again as well.
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Check prefix", "[trie][check_prefix]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = root;
    int depth = nrules - 1;

    tracking_vector<unsigned short, DataStruct::Tree> prefix;

    for(int i = 0; i < depth; i++) {
        CAPTURE(i);
        n = tree->construct_node(i+1, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        tree->insert(n);
        prefix.push_back(i+1);
    }

    REQUIRE(tree->num_nodes() == (depth + 1));
    REQUIRE(n->depth() == depth);

    // Check if the tree correctly finds a prefix
    CHECK(tree->check_prefix(prefix) == n);

    SECTION("Wrong rule") {

        // What if one of the prefix has a rule that isn't in the tree where it should be?
        prefix[depth - 1] += 1;
        CHECK(tree->check_prefix(prefix) == NULL);
    }

    SECTION("Not enough rules") {

        // What if the tree's not deep enough?
        n->parent()->delete_child(prefix[depth - 1]);
        CHECK(tree->check_prefix(prefix) == NULL);
    }
}

/**
                            DELETE subtree

    Creates an artificial tree and then deletes it, first non-destructively,
    so we can check if it labels the correct nodes for destruction, and then
    destructively, to see if it deletes the correct nodes, and then deletes
    everything up to the root node as well.
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Delete subtree", "[trie][delete_subtree]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = root;
    int depth = nrules - 1;

    tracking_vector<unsigned short, DataStruct::Tree> prefix;

    // This time, make two children of root, then from the left child make
    // two more, and from the left of those two make two more, etc for a certain depth
    for(int i = 0; i < depth; i++) {
        CAPTURE(i);

        // delete subtree requires the done information (interior nodes are 'done')
        n->set_done();

        Node * n1 = tree->construct_node(i+1, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        Node * n2 = tree->construct_node(i+2, T_NRULES, T_PRED, T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        tree->insert(n1);
        tree->insert(n2);

        n = n1;

        prefix.push_back(i+1);
    }

    // each iteration of the for loop creates two nodes, and root
    REQUIRE(tree->num_nodes() == (2 * depth + 1));
    REQUIRE(tree->check_prefix(prefix) == n);

    REQUIRE(n->depth() == depth);

    // n is a leaf node
    REQUIRE_FALSE(n->done());

    // Get the child of root that has a big subtree under it
    Node * t = root->child(1);
    REQUIRE(t != NULL);
    REQUIRE(t->done());

    SECTION("Not destructive") {

        // Have to delete the node from its parent, since it isn't removed by delete_subtree
        root->delete_child(t->id());
        delete_subtree(tree, t, false, false);

        // Leaf nodes should be lazily marked, not deleted
        CHECK(n->deleted());
        // check_prefix should no longer find it, since most of it has been deleted
        CHECK(tree->check_prefix(prefix) == NULL);

        CHECK(tree->num_nodes() == (depth + 2));
    }

    SECTION("Destructive") {

        root->delete_child(t->id());
        delete_subtree(tree, t, true, false);

        // All deleted except root and its one childless child
        CHECK(tree->num_nodes() == 2);
    }

    SECTION("Full destructive") {

        delete_subtree(tree, root, true, false);

        // Everything's been deleted!
        CHECK(tree->num_nodes() == 0);

        // So the destructor of this fixture doesn't segfault
        tree->insert_root();
    }
}

TEST_CASE_METHOD(TrieFixture, "Trie/Update optimal rulelist", "[trie][optimal_list]") {

    tracking_vector<unsigned short, DataStruct::Tree> rule_list = {0, 2, 1, 3};
    unsigned short new_rule = 5;
    tree->update_opt_rulelist(rule_list, new_rule);

    rule_list.push_back(new_rule);

    REQUIRE(tree->opt_rulelist() == rule_list);
}

/**
                        UPDATE OPTIMAL PREDICTIONS

    Creates a simple artificial tree of depth 3 and assigns the nodes particular
    predictions, then gives the tree the deepest node as the end of the optimal
    rule list and sees if the tree then determines the list of predictions of said
    rule list correctly.
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Update optimal predictions", "[trie][optimal_preds]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    // Array of predictions for the line of nodes about to be created
    // (first child has false, child of that has true, child of that has false, etc.)
    tracking_vector<bool, DataStruct::Tree> predictions = {false, true, false};
    bool new_pred = false;
    bool new_default_pred = true;

    Node * n = root;
    int depth = predictions.size();

    for(int i = 0; i < depth; i++) {
        CAPTURE(i);

        n = tree->construct_node(i+1, T_NRULES, predictions[i], T_DEF_PRED, T_LOWER, T_OBJ, n, T_NNC, T_NSAMPLES, i, T_C, T_MINOR);
        tree->insert(n);
    }

    REQUIRE(tree->num_nodes() == (depth + 1));
    REQUIRE(n->depth() == depth);

    // This function should find the predictions of all its ancestors, plus new_pred (its prediction) and the default rule
    // Therefore it should get the same result as the predictions array used to create is ancestors
    tree->update_opt_predictions(n, new_pred, new_default_pred);

    // Plus these
    predictions.push_back(new_pred);
    predictions.push_back(new_default_pred);

    REQUIRE(tree->opt_predictions() == predictions);
}

/**
                                GARBAGE COLLECT

    Creates 3 children of the root node, each with 2 children of its own. The three
    children have higher, equal, and lower lower bounds than the minimum objective,
    respectively, and then the tree is garbage collected to see if the correct nodes
    (the first and second of the three children) are deleted, along with their
    children. Then, for fun, we run the queue's select function to see if it
    correctly cleans up the nodes marked for deletion (the children of those
    two nodes that were deleted).
**/
TEST_CASE_METHOD(TrieFixture, "Trie/Garbage collect", "[trie][garbage_collect]") {

    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    double minobj = 0.5;
    tree->update_min_objective(minobj);
    REQUIRE(tree->min_objective() == minobj);

    // We create three children from the root node. In order:
    // higher, equal, and lower lower bound than the minimum objective (0.5).
    // Each one has two children.

    Node * nodes[3][3];

    nodes[0][0] = tree->construct_node(1, T_NRULES, T_PRED, T_DEF_PRED, minobj + 0.2, 0.12, root, T_NNC, T_NSAMPLES, 0, T_C, T_MINOR);
    nodes[1][0] = tree->construct_node(2, T_NRULES, T_PRED, T_DEF_PRED, minobj, 0.12, root, T_NNC, T_NSAMPLES, 0, T_C, T_MINOR);
    nodes[2][0] = tree->construct_node(3, T_NRULES, T_PRED, T_DEF_PRED, minobj - 0.2, 0.12, root, T_NNC, T_NSAMPLES, 0, T_C, T_MINOR);

    root->set_done();

    for(int i = 0; i < 3; i++) {
        CAPTURE(i);

        nodes[i][0]->set_done();
        tree->insert(nodes[i][0]);

        nodes[i][1] = tree->construct_node(4, T_NRULES, T_PRED, T_DEF_PRED, minobj - 0.1, 0.12, nodes[i][0], T_NNC, T_NSAMPLES, 1, T_C, T_MINOR);
        nodes[i][2] = tree->construct_node(5, T_NRULES, T_PRED, T_DEF_PRED, minobj - 0.15, 0.12, nodes[i][0], T_NNC, T_NSAMPLES, 1, T_C, T_MINOR);

        tree->insert(nodes[i][1]);
        tree->insert(nodes[i][2]);
    }

    // Check if all the nodes were counted
    CHECK(tree->num_nodes() == 10);

    tree->garbage_collect();

    SECTION("Check if correctly deleted") {

        // The first and second nodes (higher and equal lower bounds) should have been deleted
        CHECK(tree->num_nodes() == 8);

        // And their childeren lazily marked
        CHECK(nodes[0][1]->deleted());
        CHECK(nodes[0][2]->deleted());
        CHECK(nodes[1][1]->deleted());
        CHECK(nodes[1][2]->deleted());

        // But not the third node's children (lower lower bound)
        CHECK_FALSE(nodes[2][1]->deleted());
        CHECK_FALSE(nodes[2][2]->deleted());
    }

    SECTION("Test deletion of lazily marked nodes") {

        Queue * queue = new Queue(lb_cmp, "LOWER BOUND");

        REQUIRE(queue != NULL);

        // Add all the leaf nodes that need to be deleted to the queue
        queue->push(nodes[0][1]);
        queue->push(nodes[0][2]);
        queue->push(nodes[1][1]);
        queue->push(nodes[1][2]);

        VECTOR captured;
        rule_vinit(nsamples, &captured);

        std::pair<Node*, tracking_vector<unsigned short, DataStruct::Tree>> prefix_node = queue->select(tree, captured);

        // Were all the nodes found to be lazily marked deleted?
        // Only the root, the root's third child and its two children should remain
        CHECK(tree->num_nodes() == 4);

        // Since all the nodes in the queue were deleted
        CHECK(prefix_node.first == NULL);

        rule_vfree(&captured);
    }
}



