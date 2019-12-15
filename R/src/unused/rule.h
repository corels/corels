/*
 * Copyright (c) 2016 Hongyu Yang, Cynthia Rudin, Margo Seltzer, and
 * The President and Fellows of Harvard College
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifdef GMP
#include <gmp.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>

/*
 * This library implements rule set management for Bayesian rule lists.
 */

/*
 * Rulelist is an ordered collection of rules.
 * A Rule is simply and ID combined with a large binary vector of length N
 * where N is the number of samples and a 1 indicates that the rule captures
 * the sample and a 0 indicates that it does not.
 *
 * Definitions:
 * captures(R, S) -- A rule, R, captures a sample, S, if the rule evaluates
 * true for Sample S.
 * captures(N, S, RS) -- In ruleset RS, the Nth rule captures S.
 */

/*
 * Even though every rule in a given experiment will have the same number of
 * samples (n_ys), we include it in the rule definition. Note that the size of
 * the captures array will be n_ys/sizeof(unsigned).
 *
 * Note that a rule outside a rule set stores captures(R, S) while a rule in
 * a rule set stores captures(N, S, RS).
 */

/*
 * Define types for bit vectors.
 */
typedef unsigned long v_entry;
#ifdef GMP
typedef mpz_t VECTOR;
#define VECTOR_ASSIGN(dest, src) mpz_init_set(dest, src)
#else
typedef v_entry *VECTOR;
#define VECTOR_ASSIGN(dest, src) dest = src
#endif

#define BITS_PER_ENTRY (sizeof(v_entry) * 8)

/*
 * We have slightly different structures to represent the original rules
 * and rulesets. The original structure contains the ascii representation
 * of the rule; the ruleset structure refers to rules by ID and contains
 * captures which is something computed off of the rule's truth table.
 */
typedef struct rule {
	char *features;			/* Representation of the rule. */
	int support;			/* Number of 1's in truth table. */
	int cardinality;
    int *ids;
	VECTOR truthtable;		/* Truth table; one bit per sample. */
} rule_t;

typedef struct ruleset_entry {
	unsigned rule_id;
	int ncaptured;			/* Number of 1's in bit vector. */
	VECTOR captures;		/* Bit vector. */
} ruleset_entry_t;

typedef struct ruleset {
	int n_rules;			/* Number of actual rules. */
	int n_alloc;			/* Spaces allocated for rules. */
	int n_samples;
	ruleset_entry_t *rules;		/* Array of rules. */
} ruleset_t;

typedef struct params {
	double lambda;
	double eta;
	double alpha[2];
	double threshold;
	int iters;
	int nchain;
} params_t;

typedef struct data {
	rule_t * rules;		/* rules in BitVector form in the data */
	rule_t * labels;	/* labels in BitVector form in the data */
	int nrules;		/* number of rules */
	int nsamples;		/* number of samples in the data. */
} data_t;

typedef struct interval {
	double a, b;
} interval_t;

typedef struct pred_model {
	ruleset_t *rs;		/* best ruleset. */
	double *theta;
	interval_t *confIntervals;
} pred_model_t;


/*
 * Functions in the library
 */
int ruleset_init(int, int, int *, rule_t *, ruleset_t **);
int ruleset_add(rule_t *, int, ruleset_t **, int, int);
int ruleset_backup(ruleset_t *, int **);
int ruleset_copy(ruleset_t **, ruleset_t *);
void ruleset_delete(rule_t *, int, ruleset_t *, int);
int ruleset_swap(ruleset_t *, int, int, rule_t *);
int ruleset_swap_any(ruleset_t *, int, int, rule_t *);
int pick_random_rule(int, ruleset_t *);

void ruleset_destroy(ruleset_t *);
void ruleset_print(ruleset_t *, rule_t *, int);
void ruleset_entry_print(ruleset_entry_t *, int, int);
int create_random_ruleset(int, int, int, rule_t *, ruleset_t **);

int rules_init(const char *, int *, int *, rule_t **, int);
void rules_free(rule_t *, const int, int);

void rule_print(rule_t *, int, int, int);
void rule_print_all(rule_t *, int, int, int);
void rule_vector_print(VECTOR, int);
void rule_copy(VECTOR, VECTOR, int);

int rule_isset(VECTOR, int, int);
void rule_set(VECTOR, int, int, int);
int rule_vinit(int, VECTOR *);
int rule_vfree(VECTOR *);
int make_default(VECTOR *, int);
void rule_vclear(int, VECTOR);
void rule_vand(VECTOR, VECTOR, VECTOR, int, int *);
void rule_vandnot(VECTOR, VECTOR, VECTOR, int, int *);
void rule_vor(VECTOR, VECTOR, VECTOR, int, int *);
void rule_not(VECTOR, VECTOR, int, int *);
int count_ones(v_entry);
int count_ones_vector(VECTOR, int);
int rule_vector_cmp(const VECTOR, const VECTOR, int, int);
size_t rule_vector_hash(const VECTOR, short);

int ascii_to_vector(char *, size_t, int *, int *, VECTOR *);

/* Functions for the Scalable Baysian Rule Lists */
double *predict(data_t *, pred_model_t *, params_t *);
void ruleset_proposal(ruleset_t *, int, int *, int *, char *, double *);
ruleset_t *run_mcmc(int, int, int, rule_t *, rule_t *, params_t *, double);
ruleset_t *run_simulated_annealing(int,
    int, int, int, rule_t *, rule_t *, params_t *);
pred_model_t *train(data_t *, int, int, params_t *);
#if defined(__cplusplus)
}
#endif
