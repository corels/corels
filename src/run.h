#ifndef RUN_H
#define RUN_H

#include "rule.h"
#include "queue.h"

int run_corels_begin(double c, char* vstring, int curiosity_policy,
                  int map_type, int ablation, int calculate_size, int nrules, int nlabels,
                  int nsamples, rule_t* rules, rule_t* labels, rule_t* meta, int freq, char* log_fname,
                  PermutationMap*& pmap, CacheTree*& tree, Queue*& queue, double& init,
                  std::set<std::string>& verbosity);

int run_corels_loop(size_t max_num_nodes, PermutationMap* pmap, CacheTree* tree, Queue* queue);

double run_corels_end(std::vector<int>* rulelist, std::vector<int>* classes, int early, int latex_out, rule_t* rules,
                      rule_t* labels, char* opt_fname, PermutationMap*& pmap, CacheTree*& tree, Queue*& queue,
                      double init, std::set<std::string>& verbosity);

#endif
