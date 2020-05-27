This folder contains code and data files for testing our data structures and algorithm

To compile the tests, simply run 'make' from the /src folder, then run 'make' in this folder to
compile the tests, and then run'./tests' in this folder to run them. If you wish to test only one part of
the system, enter './tests [name]' where name is one of the following:

            Name                Description
    -   prefixmap       Test the prefix permutation map
    -   capturemap      Test the captured permutation map
    -   trie            Test the rulelist trie
    -   queue           Test the priority queue

    -   trie_init       Test the initialization of the trie
    -   construct_node  Test the construction of a node by the trie
    -   delete_node     Test the deletion of a node by the trie
    -   node_prefix     Test getting the prefix and predictions from a node
    -   num_eval        Test the trie's behavior with the number of evaluated nodes
    -   num_nodes       Test the trie's decrement_num_nodes function
    -   min_obj         Test the the storing of the minimum objective
    -   prune_up        Test the trie's prune_up function
    -   check_prefix    Test the ability of the trie to check a prefix
    -   delete_subtree  Test the trie's delete_subtree function
    -   optimal_list    Test the trie's storing of the optimal rule list
    -   optimal_preds   Test the trie's storing of the optimal predictions

    -   garbage_collect Test the garbage collection function

    -   prefix_empty    Test inserting into an empty prefix permutation map
    -   prefix_higher   Test inserting a prefix with a higher lower bound than
                        previously stored into the prefix permutation map
    -   prefix_lower    Test inserting a prefix with a lower lower bound than
                        previously stored into the prefix permutation map

    -   capture_empty   Test inserting into an empty captured permutation map
    -   capture_higher  Test inserting a prefix with a higher lower bound than
                        previously stored into the captured permutation map
    -   capture_lower   Test inserting a prefix with a lower lower bound than
                        previously stored into the captured permutation map

    -   push            Test pushing to the priority queue
    -   front           Test the queue's front function, that gets the top queue element
    -   pop             Test popping the priority queue
    -   select          Test the priority queue's select function

For example, './tests [prefixmap]' would only run the tests for the prefix permutation map.

Further options controlling the behavior of Catch, the testing framework used for these tests, can be
passed after the tags specifying one part of the suite to run. To view these options, run './tests --help'

A small dataset, found in the files tests.out, tests.label and tests.minor, all in the src/evaluate
folder, is used across many of the tests. To print the contents of this dataset, simply append '-v'
to the end of the other options (it must be at the very end of argv). The total command is thus:

        ./tests [tags] [options] -v
