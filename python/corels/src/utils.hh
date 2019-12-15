#ifndef UTILS_H
#define UTILS_H

#include "rule.hh"

int mine_rules(char** features, rule_t *samples, int nfeatures, int nsamples, 
                int max_card, double min_support, rule_t **rules_out, int verbose);

int minority(rule_t* rules, int nrules, rule_t* labels, int nsamples, rule_t* minority_out, int verbose);

#endif
