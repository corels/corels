#pragma once

#include "../src/queue.h"

#ifdef GMP
    #include <gmp.h>
#else
    #define NENTRIES  ((nsamples + (int)BITS_PER_ENTRY - 1) / (int)BITS_PER_ENTRY)
#endif

extern rule_t * rules;
extern rule_t * labels;
extern rule_t * minority;
extern int nrules;
extern int nsamples;
extern int nlabels;
extern int nminority;

/**
   These are the default values of the arguments passed to the tree's contruct_node
   function when constructing a new node (used when the values don't really matter).
**/
#define T_NEW_RULE  1
#define T_NRULES    nrules
#define T_PRED      true
#define T_DEF_PRED  true
#define T_LOWER     0.1
#define T_OBJ       0.5
#define T_PARENT    root
#define T_NNC       0
#define T_NSAMPLES  nsamples
#define T_LPREFIX   0
#define T_MINOR     0.0
#define T_TYPE      "node"

// Some more value that are used throughout the code: length constant, tree ablation
#define T_C         0.01
#define T_ABLATION  0


class TrieFixture {

public:
    TrieFixture() : c(T_C), type(T_TYPE), ablation(T_ABLATION), calculate_size(false),
                    tree(NULL), root(NULL) {
        tree = new CacheTree(T_NSAMPLES, T_NRULES, T_C, rules, labels,
                                 minority, ablation, calculate_size, type);
        if(tree) {
            tree->insert_root();
            root = tree->root();
        }
    }

    virtual ~TrieFixture() {
//        TODO: This causes segfaults in the tree's desctrutor when deleting a trivial tree
//        without any nodes except the root node. Most likely, the bug occurs because 
//        the root node gets deleted, but its reference in the Tree class is not set to null.
//        Thus in the tree's destructor, the tree thinks it has a root node when it doesn't
//        and tries to free already freed memory.
//
        if(tree)
            delete tree;
    }

protected:
    double c;
    const char * type;
    int ablation;
    bool calculate_size;
    CacheTree * tree;
    Node * root;
};


class PrefixMapFixture {

public:
    PrefixMapFixture() : pmap(NULL), tree(NULL), root(NULL), num_not_captured(T_NNC),
                         c(T_C), prediction(T_PRED), default_prediction(T_DEF_PRED),
                         lower_bound(T_LOWER), objective(T_OBJ), len_prefix(4), equivalent_minority(T_MINOR) {
        pmap = new PrefixPermutationMap();

        tree = new CacheTree(T_NSAMPLES, T_NRULES, T_C, rules, labels, NULL, T_ABLATION, false, T_TYPE);

        if(tree) {
            tree->insert_root();
            root = tree->root();
        }

        // Canonical prefix, also the prefix map key
        correct_key = {4, 1, 2, 4, 5};

        // First insertion
        parent_prefix = {4, 2, 1};
        new_rule = 5;
        correct_indices = {4, 2, 1, 0, 3};

        // Second insertion, for second and third tests
        parent_prefix_2 = {1, 4, 5};
        new_rule_2 = 2;
        correct_indices_2 = {4, 0, 3, 1, 2};
    }

    virtual ~PrefixMapFixture() {

        if(pmap)
            delete pmap;

        if(tree)
            delete tree;
    }

protected:

    PrefixPermutationMap * pmap;
    CacheTree * tree;
    Node * root;

    int num_not_captured;

    double c;

    bool prediction;
    bool default_prediction;
    double lower_bound;
    double objective;
    int len_prefix;

    std::vector<unsigned short> correct_key;

    tracking_vector<unsigned short, DataStruct::Tree> parent_prefix;
    int new_rule;
    std::vector<unsigned char> correct_indices;

    tracking_vector<unsigned short, DataStruct::Tree> parent_prefix_2;
    int new_rule_2;
    std::vector<unsigned char> correct_indices_2;

    double equivalent_minority;
};


class CapturedMapFixture {

public:
    CapturedMapFixture() : pmap(NULL), tree(NULL), root(NULL), num_not_captured(T_NNC),
                         c(T_C), prediction(T_PRED), default_prediction(T_DEF_PRED),
                         lower_bound(T_LOWER), objective(T_OBJ), len_prefix(4), equivalent_minority(T_MINOR) {
        pmap = new CapturedPermutationMap();

        tree = new CacheTree(T_NSAMPLES, T_NRULES, T_C, rules, labels, NULL, T_ABLATION, false, T_TYPE);

        if(tree) {
            tree->insert_root();
            root = tree->root();
        }

        // First insertion
        parent_prefix = {4, 2, 1};
        new_rule = 5;

        // Second insertion, for second and third tests
        parent_prefix_2 = {1, 4, 5};
        new_rule_2 = 2;

        // Canonical prefix
        std::vector<unsigned short> ordered_prefix = {1, 2, 4, 5};

        rule_vinit(nsamples, &not_captured);

        // Generate the not_captured vector, for use as the key in the captured map
#ifdef GMP
        // First find the captured vector, then take its complement
        for(size_t i = 0; i < ordered_prefix.size(); i++) {
            mpz_ior(not_captured, not_captured, rules[ordered_prefix.at(i)].truthtable);
        }

        mpz_com(not_captured, not_captured);
#else
        // Set all bits to 1 (since we won't be taking the complement as with GMP, initially
        // having all bits as one means it initializes the vector to all being not_captured)
        for(int i = 0; i < NENTRIES; i++) {
            not_captured[i] = ~(not_captured[i] & 0);
        }

        for(size_t i = 0; i < ordered_prefix.size(); i++) {
            for(int j = 0; j < NENTRIES; j++) {
                not_captured[j] = not_captured[j] & ~(rules[ordered_prefix.at(i)].truthtable[j]);
            }
        }
#endif
    }

    virtual ~CapturedMapFixture() {
        rule_vfree(&not_captured);

        if(pmap)
            delete pmap;

        if(tree)
            delete tree;
    }

protected:

    CapturedPermutationMap * pmap;
    CacheTree * tree;
    Node * root;

    int num_not_captured;

    double c;

    bool prediction;
    bool default_prediction;
    double lower_bound;
    double objective;
    int len_prefix;

    VECTOR not_captured;

    tracking_vector<unsigned short, DataStruct::Tree> parent_prefix;
    int new_rule;

    tracking_vector<unsigned short, DataStruct::Tree> parent_prefix_2;
    int new_rule_2;

    double equivalent_minority;
};


class QueueFixture {

public:
    QueueFixture() : queue(NULL), tree(NULL), root(NULL) {
        queue = new Queue(lb_cmp, "LOWER BOUND");

        tree = new CacheTree(T_NSAMPLES, T_NRULES, T_C, rules, labels, NULL, T_ABLATION, false, T_TYPE);

        if(tree) {
            tree->insert_root();
            root = tree->root();
        }
    }

    virtual ~QueueFixture() {
        if(queue)
            delete queue;

        if(tree)
            delete tree;
    }

protected:

    Queue * queue;
    CacheTree * tree;
    Node * root;
};
