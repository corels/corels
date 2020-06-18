#include "catch.hpp"
#include "fixtures.h"
#include "../src/queue.h"

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

