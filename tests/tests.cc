/***  MAIN FILE WITH THE ACTUAL TESTS ***/

#include "catch.hpp"

#include "fixtures.hh"


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

/**
                                INSERT INTO EMPTY MAP

    Tests correct insertion (all attributed are recored correctly and the map is updated
    accordingly) into an empty prefix permutation map. Simply inserts a node into the map
    and checks if all the attributes of the node that were passed to the map's insert function
    are stored correctly in the created node, and if the map now has a new entry that
    has the correct key and value (canonical prefix, lower bound and indices) that were
    passed to the map's insert function.
**/
TEST_CASE_METHOD(PrefixMapFixture, "Prefix Map/Insert into empty map", "[prefixmap][insert_empty][prefix_empty]") {

    REQUIRE(pmap != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = pmap->insert(new_rule, nrules, prediction, default_prediction,
                            lower_bound, objective, root, num_not_captured,
                            nsamples, len_prefix, c, equivalent_minority, tree,
                            NULL, parent_prefix);

    REQUIRE(n != NULL);

    SECTION("Check if node was created correctly") {

        CHECK(n->parent() == root);
        CHECK(n->prediction() == prediction);
        CHECK(n->default_prediction() == default_prediction);
        CHECK(n->lower_bound() == lower_bound);
        CHECK(n->objective() == objective);
        CHECK(n->num_captured() == (nsamples - num_not_captured));
        CHECK(n->equivalent_minority() == equivalent_minority);
    }

    SECTION("Check if permutation map was updated correctly") {

        REQUIRE(pmap->getMap()->size() == 1);

        PrefixMap::iterator inserted = pmap->getMap()->begin();

        // Is inserted a valid pointer?
        REQUIRE(inserted != pmap->getMap()->end());

        // Check if the lower bound was recorded correctly
        CHECK(inserted->second.first == lower_bound);

        unsigned short* key = inserted->first.key;
        unsigned char* indices = inserted->second.second;

        CHECK(key[0] == indices[0]);
        CHECK(key[0] == len_prefix);

        // Check if the inserted key and indices are correct
        for(int i = 0; i < len_prefix+1; i++) {
            CAPTURE(i);
            CHECK(key[i] == correct_key.at(i));
            CHECK(indices[i] == correct_indices.at(i));
        }
    }
}

/**
                            INSERT WITH HIGHER LOWER BOUND

    Inserts a node into the prefix permutation map, then tries to insert another
    with the same prefix but a higher lower bound. Then, it checks if the second
    insertion is blocked by the map, and the information for that prefix is not
    updated but remains what it was from the first node.
**/
TEST_CASE_METHOD(PrefixMapFixture, "Prefix Map/Insert with higher lower bound", "[prefixmap][insert_higher][prefix_higher]") {

    REQUIRE(pmap != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = pmap->insert(new_rule, nrules, prediction, default_prediction,
                            lower_bound, objective, root, num_not_captured,
                            nsamples, len_prefix, c, equivalent_minority, tree,
                            NULL, parent_prefix);

    REQUIRE(n != NULL);
    REQUIRE(pmap->getMap()->size() == 1);

    double h_bound = lower_bound + 0.02;

    // Expected behavior is that the map remains unchanged, since h_bound
    // is greated than lower_bound
    Node * n2 = pmap->insert(new_rule_2, nrules, prediction, default_prediction,
                             h_bound, objective, root, num_not_captured,
                             nsamples, len_prefix, c, equivalent_minority, tree,
                             NULL, parent_prefix_2);

    REQUIRE(n2 == NULL);
    REQUIRE(pmap->getMap()->size() == 1);

    PrefixMap::iterator inserted = pmap->getMap()->begin();

    REQUIRE(inserted != pmap->getMap()->end());

    unsigned short* key = inserted->first.key;
    unsigned char* indices = inserted->second.second;

    CHECK(key[0] == indices[0]);
    CHECK(key[0] == len_prefix);

    // Check if the key and indices are unchanged (same as the ones inserted with n instead of n2)
    for(int i = 0; i < len_prefix+1; i++) {
        CAPTURE(i);
        CHECK(key[i] == correct_key.at(i));
        CHECK(indices[i] == correct_indices.at(i));
    }

    // Check if the node wasn't inserted (it should not have, since the permutation bound should block it)
    CHECK(n2 == NULL);

    // Check if the lower bound is unchanged
    CHECK(inserted->second.first == lower_bound);
}

/**
                            INSERT WITH LOWER LOWER BOUND

    Inserts a node into the prefix permutation map, then tries to insert another
    with the same prefix but a lower lower bound. Then, it checks if the second
    insertion updates the map correctly to the new lowest lower bound (the lower
    bound of the second insertion) and the corresponding new prefix (indices of the
    second insertion).
**/
TEST_CASE_METHOD(PrefixMapFixture, "Prefix Map/Insert with lower lower bound", "[prefixmap][insert_lower][prefix_lower]") {

    REQUIRE(pmap != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = pmap->insert(new_rule, nrules, prediction, default_prediction,
                            lower_bound, objective, root,
                            0, nsamples, len_prefix, c, 0.0, tree, NULL,
                            parent_prefix);

    REQUIRE(n != NULL);
    REQUIRE(pmap->getMap()->size() == 1);

    double l_bound = lower_bound - 0.02;

    // Expected behavior is that the map will change the indices
    // and update the best permutation
    Node * n2 = pmap->insert(new_rule_2, nrules, prediction, default_prediction,
                             l_bound, objective, root, num_not_captured,
                             nsamples, len_prefix, c, equivalent_minority, tree,
                             NULL, parent_prefix_2);

    REQUIRE(n2 != NULL);
    REQUIRE(pmap->getMap()->size() == 1);

    PrefixMap::iterator inserted = pmap->getMap()->begin();

    REQUIRE(inserted != pmap->getMap()->end());

    unsigned short* key = inserted->first.key;
    unsigned char* indices = inserted->second.second;

    CHECK(key[0] == indices[0]);
    CHECK(key[0] == len_prefix);

    // Check if the indices are changed and are correct (new values)
    for(int i = 0; i < len_prefix+1; i++) {
        CAPTURE(i);
        CHECK(key[i] == correct_key.at(i));
        CHECK(indices[i] == correct_indices_2.at(i));
    }

    // Check if the lower bound is the new correct lower bound
    CHECK(inserted->second.first == l_bound);
}

/**
                                INSERT INTO EMPTY MAP

    Tests correct insertion (all attributed are recored correctly and the map is updated
    accordingly) into an empty captured permutation map. Simply inserts a node into the map
    and checks if all the attributes of the node that were passed to the map's insert function
    are stored correctly in the created node, and if the map now has a new entry that
    has the correct key and value (captured vector, lower bound and prefix) that were
    passed to the map's insert function.
**/
TEST_CASE_METHOD(CapturedMapFixture, "Captured Map/Insert into empty map", "[capturemap][insert_empty][capture_empty]") {

    REQUIRE(pmap != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = pmap->insert(new_rule, nrules, prediction, default_prediction,
                            lower_bound, objective, root, num_not_captured,
                            nsamples, len_prefix, c, equivalent_minority, tree,
                            not_captured, parent_prefix);

    REQUIRE(n != NULL);

    SECTION("Check if node was created correctly") {

        CHECK(n->parent() == root);
        CHECK(n->prediction() == prediction);
        CHECK(n->default_prediction() == default_prediction);
        CHECK(n->lower_bound() == lower_bound);
        CHECK(n->objective() == objective);
        CHECK(n->num_captured() == (nsamples - num_not_captured));
        CHECK(n->equivalent_minority() == equivalent_minority);
    }

    SECTION("Check if permutation map was updated correctly") {

        REQUIRE(pmap->getMap()->size() == 1);

        CapturedMap::iterator inserted = pmap->getMap()->begin();

        // Is inserted a valid pointer?
        CHECK(inserted != pmap->getMap()->end());

        // Check if the lower bound was recorded correctly
        CHECK(inserted->second.first == lower_bound);

        // Check if the key is correct
#ifdef GMP
        CHECK(mpz_cmp(inserted->first.key, not_captured) == 0);
#else
        for(int i = 0; i < NENTRIES; i++) {
            CAPTURE(i);
            CHECK(inserted->first.key[i] == not_captured[i]);
        }
#endif

        parent_prefix.push_back(new_rule);
        CHECK(inserted->second.second == parent_prefix);
    }
}

/**
                            INSERT WITH HIGHER LOWER BOUND

    Inserts a node into the captured permutation map, then tries to insert another
    with the same captured vector but a higher lower bound. Then, it checks if the second
    insertion is blocked by the map, and the information for that captured vector key is not
    updated but remains what it was from the first node.
**/
TEST_CASE_METHOD(CapturedMapFixture, "Captured Map/Insert with higher lower bound", "[capturemap][insert_higher][capture_higher]") {

    REQUIRE(pmap != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = pmap->insert(new_rule, nrules, prediction, default_prediction,
                            lower_bound, objective, root, num_not_captured,
                            nsamples, len_prefix, c, equivalent_minority, tree,
                            not_captured, parent_prefix);

    REQUIRE(n != NULL);
    REQUIRE(pmap->getMap()->size() == 1);

    double h_bound = lower_bound + 0.02;

    // Expected behavior is that the map remains unchanged, since h_bound
    // is greated than lower_bound
    Node * n2 = pmap->insert(new_rule_2, nrules, prediction, default_prediction,
                             h_bound, objective, root, num_not_captured,
                             nsamples, len_prefix, c, equivalent_minority, tree,
                             not_captured, parent_prefix_2);

    REQUIRE(pmap->getMap()->size() == 1);

    CapturedMap::iterator inserted = pmap->getMap()->begin();

    CHECK(inserted != pmap->getMap()->end());

    // Check if the key is the same as always
#ifdef GMP
    CHECK(mpz_cmp(inserted->first.key, not_captured) == 0);
#else
    for(int i = 0; i < NENTRIES; i++) {
        CAPTURE(i);
        CHECK(inserted->first.key[i] == not_captured[i]);
    }
#endif

    // Check if the prefix is the same as the first node, since the second node should have been blocked by the permutation bound
    parent_prefix.push_back(new_rule);
    CHECK(inserted->second.second == parent_prefix);

    // Check if the node wasn't inserted (it should not have, since the permutation bound should block it)
    CHECK(n2 == NULL);

    // Check if the lower bound is unchanged
    CHECK(inserted->second.first == lower_bound);
}

/**
                            INSERT WITH LOWER LOWER BOUND

    Inserts a node into the captured permutation map, then tries to insert another
    with the same captured vector but a lower lower bound. Then, it checks if the second
    insertion updates the map correctly to the new lowest lower bound (the lower
    bound of the second insertion) and the corresponding new prefix.
**/
TEST_CASE_METHOD(CapturedMapFixture, "Captured Map/Insert with lower lower bound", "[capturemap][insert_lower][capture_lower]") {

    REQUIRE(pmap != NULL);
    REQUIRE(tree != NULL);
    REQUIRE(root != NULL);

    Node * n = pmap->insert(new_rule, nrules, prediction, default_prediction,
                            lower_bound, objective, root, num_not_captured,
                            nsamples, len_prefix, c, equivalent_minority, tree,
                            not_captured, parent_prefix);

    REQUIRE(n != NULL);
    REQUIRE(pmap->getMap()->size() == 1);

    double l_bound = lower_bound - 0.02;

    // Expected behavior is that the map will change the indices
    // and update the best permutation
    Node * n2 = pmap->insert(new_rule_2, nrules, prediction, default_prediction,
                             l_bound, objective, root, num_not_captured,
                             nsamples, len_prefix, c, equivalent_minority, tree,
                             not_captured, parent_prefix_2);

    REQUIRE(pmap->getMap()->size() == 1);

    CapturedMap::iterator inserted = pmap->getMap()->begin();

    CHECK(inserted != pmap->getMap()->end());

    // Check if the key is the same
#ifdef GMP
    CHECK(mpz_cmp(inserted->first.key, not_captured) == 0);
#else
    for(int i = 0; i < NENTRIES; i++) {
        CAPTURE(i);
        CHECK(inserted->first.key[i] == not_captured[i]);
    }
#endif

    // Check if the prefix has been updated to the new best lower bound (the second prefix)
    parent_prefix_2.push_back(new_rule_2);
    CHECK(inserted->second.second == parent_prefix_2);

    // Check if the node was inserted
    CHECK(n2 != NULL);

    // Check if the lower bound is the new correct lower bound
    CHECK(inserted->second.first == l_bound);
}

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
