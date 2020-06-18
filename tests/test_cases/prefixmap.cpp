#include "catch.hpp"
#include "fixtures.h"
#include "../src/queue.h"

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
