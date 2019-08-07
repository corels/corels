#ifndef RUN_H
#define RUN_H

#include "rule.hh"

int run_corels_begin(double c, char* vstring, int curiosity_policy,
                  int map_type, int ablation, int calculate_size, int nrules, int nlabels,
                  int nsamples, rule_t* rules, rule_t* labels, rule_t* meta, int freq, char* log_fname,
                  PermutationMap*& g_pmap, CacheTree*& g_tree, Queue*& g_queue, double& g_init,
                  std::set<std::string>& g_verbosity);

int run_corels_loop(size_t max_num_nodes, PermutationMap* g_pmap, CacheTree* g_tree, Queue* g_queue);

double run_corels_end(int** rulelist, int* rulelist_size, int** classes, int early, int latex_out, rule_t* rules,
                      rule_t* labels, char* opt_fname, PermutationMap*& g_pmap, CacheTree*& g_tree, Queue*& g_queue,
                      double g_init, std::set<std::string>& g_verbosity);

#endif
