#ifndef RUN_H
#define RUN_H

#include "rule.hh"

int run_corels_begin(double c, char* vstring, int curiosity_policy,
                  int map_type, int ablation, int calculate_size, int nrules, int nlabels,
                  int nsamples, rule_t* rules, rule_t* labels, rule_t* meta, int freq, char* log_fname);

int run_corels_loop(size_t max_num_nodes);

double run_corels_end(int** rulelist, int* rulelist_size, int** classes, int early, int latex_out, rule_t* rules, rule_t* labels, char* opt_fname);

#endif
