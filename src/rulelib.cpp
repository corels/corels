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
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include "rule.h"
#include "utils.h"

#if defined(R_BUILD)
 #define STRICT_R_HEADERS
 #include "R.h"
 // textual substitution
 #define printf Rprintf
#endif

/* Function declarations. */
#define RULE_INC 100

/* One-counting tools */
int bit_ones[] = {0, 1, 3, 7, 15, 31, 63, 127};

int byte_ones[] = {
/*   0 */ 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
/*  16 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*  32 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*  48 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*  64 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*  80 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*  96 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 112 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 128 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/* 144 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 160 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 176 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 192 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 208 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 224 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 240 */ 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

#define BYTE_MASK	0xFF

/*
 * Preprocessing step.
 * INPUTS: Using the python from the BRL_code.py: Call get_freqitemsets
 * to generate data files of the form:
 * 	Rule<TAB><bit vector>\n
 *
 * OUTPUTS: an array of rule_t's
 */
int
rules_init(const char *infile, int *nrules,
    int *nsamples, rule_t **rules_ret, int add_default_rule)
{
	std::ifstream fi(infile);
	std::string line;
	char *rulestr, *line_cpy = NULL;
	int rule_cnt, sample_cnt, rsize;
	int i, ones, ret;
	rule_t *rules=NULL;
	size_t len = 0;
	size_t rulelen;

	sample_cnt = rsize = 0;

	if (!fi.is_open())
		return (1);

	/*
	 * Leave a space for the 0th (default) rule, which we'll add at
	 * the end.
	 */
	rule_cnt = add_default_rule != 0 ? 1 : 0;
	while (std::getline(fi, line)) {
		len = line.length();
		line_cpy = strdup(line.c_str());
		if (rule_cnt >= rsize) {
			rsize += RULE_INC;
                	rules = (rule_t*)realloc(rules, rsize * sizeof(rule_t));
			if (rules == NULL)
				goto err;
		}

		/* Get the rule string; line will contain the bits. */
		if ((rulestr = strtok(line_cpy, " ")) == NULL)
			goto err;

		rulelen = strlen(rulestr) + 1;
		len -= rulelen;
		char* line_data = &line_cpy[rulelen];

		if ((rules[rule_cnt].features = strdup(rulestr)) == NULL)
			goto err;

		if (ascii_to_vector(line_data, len, &sample_cnt, &ones,
		    &rules[rule_cnt].truthtable) != 0) {
                        #if !defined(R_BUILD)
                        fprintf(stderr, "Loading rule '%s' failed\n", rulestr);
                        #else
                        REprintf("Loading rule '%s' failed\n", rulestr);
                        #endif
                        errno = 1;
                        goto err;
		}
		rules[rule_cnt].support = ones;

		/* Now compute the number of clauses in the rule. */
		rules[rule_cnt].cardinality = 1;
		for (char *cp = rulestr; *cp != '\0'; cp++)
			if (*cp == ',')
				rules[rule_cnt].cardinality++;
		rule_cnt++;
		line = "";
		free(line_cpy);
		line_cpy = NULL;
	}
	/* All done! */
	fi.close();

	/* Now create the 0'th (default) rule. */
	if (add_default_rule) {
		rules[0].support = sample_cnt;
		rules[0].features = (char*)"default";
		rules[0].cardinality = 0;
		if (make_default(&rules[0].truthtable, sample_cnt) != 0)
		    goto err;
	}

	*nsamples = sample_cnt;
	*nrules = rule_cnt;
	*rules_ret = rules;

	return (0);

err:
	ret = errno;

	if (line_cpy)
		free(line_cpy);

	/* Reclaim space. */
	if (rules != NULL) {
		for (i = 1; i < rule_cnt; i++) {
			free(rules[i].features);
#ifdef GMP
			mpz_clear(rules[i].truthtable);
#else
			free(rules[i].truthtable);
#endif
		}
		free(rules);
	}
	fi.close();
	return (ret);
}

void
rules_free(rule_t *rules, const int nrules, int add_default) {
	int start;

	/* Cannot free features for default rule. */
	start = 0;
	if (add_default) {
		rule_vfree(&rules[0].truthtable);
		start = 1;
	}

	for (int i = start; i < nrules; i++) {
		rule_vfree(&rules[i].truthtable);
		free(rules[i].features);
	}
	free(rules);
}

/* Malloc a vector to contain nsamples bits. */
int
rule_vinit(int len, VECTOR *ret)
{
#ifdef GMP
	mpz_init2(*ret, len);
#else
	int nentries;

	nentries = (len + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
	if ((*ret = (VECTOR)calloc(nentries, sizeof(v_entry))) == NULL)
		return(errno);
#endif
	return (0);
}

/* Clear vector -- set to all 0's */
void
rule_vclear(int len, VECTOR v) {
#ifdef GMP
    mpz_set_ui(v, 0);
#else
    int nentries = (len + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
    memset(v, 0, nentries * sizeof(v_entry));
#endif
}

/* Deallocate a vector. */
int
rule_vfree(VECTOR *v)
{
#ifdef GMP
	mpz_clear(*v);
	/* Clobber the memory. */
	memset(v, 0, sizeof(*v));

#else
	if (*v != NULL) {
		free(*v);
		*v = NULL;
	}
#endif
	return (0);
}

/*
 * Convert an ascii sequence of 0's and 1's to a bit vector.
 * Do this manually if we have the naive implementation; else use
 * GMP functions if we're using the GMP library.
 */
int
ascii_to_vector(char *line, size_t len, int *nsamples, int *nones, VECTOR *ret)
{
#ifdef GMP
	int retval;
	size_t s;

	if (mpz_init_set_str(*ret, line, 2) != 0) {
		retval = errno;
		mpz_clear(*ret);
		return (retval);
	}
	if ((s = mpz_sizeinbase (*ret, 2)) > (size_t) *nsamples)
		*nsamples = (int) s;

	*nones = mpz_popcount(*ret);
	return (0);
#else
	/*
	 * If *nsamples is 0, then we will set it to the number of
	 * 0's and 1's. If it is non-zero, then we'll ensure that
	 * the line is the right length.
	 */

	char *p;
	int i, bufsize, last_i, ones;
	v_entry val;
	v_entry *bufp, *buf;

	/* NOT DONE */
	assert(line != NULL);

	/* Compute bufsize in number of unsigned elements. */
	if (*nsamples == 0)
		bufsize = (len + BITS_PER_ENTRY - 1) / BITS_PER_ENTRY;
	else
		bufsize = (*nsamples + BITS_PER_ENTRY - 1) / BITS_PER_ENTRY;
	if ((buf = (v_entry*)malloc(bufsize * sizeof(v_entry))) == NULL)
		return(errno);

	bufp = buf;
	val = 0;
	i = 0;
	last_i = 0;
	ones = 0;


	for(p = line; len-- > 0 && *p != '\0'; p++) {
		switch (*p) {
			case '0':
				val <<= 1;
				i++;
				break;
			case '1':
				val <<= 1;
				val++;
				i++;
				ones++;
				break;
			default:
				break;
		}
		/* If we have filled up val, store it and reset it. */
		if (last_i != i && (i % BITS_PER_ENTRY) == 0) {
			*bufp = val;
			val = 0;
			bufp++;
			last_i = i;
		}
	}

	/* Store val if it contains any bits. */
    // Changed to make the last non-full v_entry left-aligned
	if ((i % BITS_PER_ENTRY) != 0)
		*bufp = val << (BITS_PER_ENTRY - (i % BITS_PER_ENTRY));

	if (*nsamples == 0)
		*nsamples = i;
	else if (*nsamples != i) {
                #if !defined(R_BUILD)
                fprintf(stderr, "Wrong number of samples. Expected %d got %d\n",
                    *nsamples, i);
                #else
                REprintf("Wrong number of samples. Expected %d got %d\n", *nsamples, i);
                #endif
                free(buf);
                buf = NULL;
        ones = 0;
	}
	*nones = ones;
	*ret = buf;
	return *ret == NULL;
#endif
}

/*
 * Create the truthtable for a default rule -- that is, it captures all samples.
 */
int
make_default(VECTOR *ttp, int len)
{
#ifdef GMP
	mpz_init2(*ttp, len);
	mpz_ui_pow_ui(*ttp, 2, (unsigned long)len);
	mpz_sub_ui (*ttp, *ttp, 1);
	return (0);
#else
	VECTOR tt;
	size_t nventry, nbytes;
	unsigned char *c;
	int m;

	nventry = (len + BITS_PER_ENTRY - 1) / BITS_PER_ENTRY;
	nbytes = nventry * sizeof(v_entry);

	if ((c = (unsigned char*)malloc(nbytes)) == NULL)
		return (errno);

	/* Set all full bytes */
	memset(c, BYTE_MASK, nbytes);
	*ttp = tt = (VECTOR)c;

	/* Fix the last entry so it has 0's for any unused bits. */
    // Changed to make the last non-full v_entry left-aligned
	m = len % BITS_PER_ENTRY;
	if (m != 0)
		tt[nventry - 1] = tt[nventry - 1] << (BITS_PER_ENTRY - m);

	return (0);
#endif
}

/* Create a ruleset. */
int
ruleset_init(int nrules,
    int nsamples, int *idarray, rule_t *rules, ruleset_t **retruleset)
{
	int cnt, i;
	rule_t *cur_rule;
	ruleset_t *rs;
	ruleset_entry_t *cur_re;
	VECTOR not_captured;

	/*
	 * Allocate space for the ruleset structure and the ruleset entries.
	 */
	rs = (ruleset_t*)malloc(sizeof(ruleset_t) + nrules * sizeof(ruleset_entry_t));
	if (rs == NULL)
		return (errno);
	/*
	 * Allocate the ruleset at the front of the structure and then
	 * the ruleset_entry_t array at the end.
	 */
	rs->n_rules = 0;
	rs->n_alloc = nrules;
	rs->n_samples = nsamples;
	make_default(&not_captured, nsamples);

	cnt = nsamples;
	for (i = 0; i < nrules; i++) {
		cur_rule = rules + idarray[i];
		cur_re = rs->rules + i;
		cur_re->rule_id = idarray[i];

		if (rule_vinit(nsamples, &cur_re->captures) != 0)
			goto err1;
		rs->n_rules++;
		rule_vand(cur_re->captures, not_captured,
		    cur_rule->truthtable, nsamples, &cur_re->ncaptured);

		rule_vandnot(not_captured, not_captured,
		    rs->rules[i].captures, nsamples, &cnt);
	}
	assert(cnt==0);

	*retruleset = rs;
	(void)rule_vfree(&not_captured);
	return (0);

err1:
	(void)rule_vfree(&not_captured);
	ruleset_destroy(rs);
	return (ENOMEM);
}

/*
 * Save the idarray for this ruleset incase we need to restore it.
 * We don't know how long the idarray currently is so always call
 * realloc, which will do the right thing.
 */
int
ruleset_backup(ruleset_t *rs, int **rs_idarray)
{
	int *ids;

	ids = *rs_idarray;

	if ((ids = (int*)realloc(ids, (rs->n_rules * sizeof(int)))) == NULL)
		return (errno);

	for (int i = 0; i < rs->n_rules; i++)
		ids[i] = rs->rules[i].rule_id;

	*rs_idarray = ids;

	return (0);
}

/*
 * When we copy rulesets, we always allocate new structures; this isn't
 * terribly efficient, but it's simpler. If the allocation and frees become
 * too expensive, we can make this smarter.
 */
int
ruleset_copy(ruleset_t **ret_dest, ruleset_t *src)
{
	int i;
	ruleset_t *dest;

	if ((dest = (ruleset_t*)malloc(sizeof(ruleset_t) +
	    (src->n_rules * sizeof(ruleset_entry_t)))) == NULL)
		return (errno);
	dest->n_alloc = src->n_rules;
	dest->n_rules = src->n_rules;
	dest->n_samples = src->n_samples;

	for (i = 0; i < src->n_rules; i++) {
		dest->rules[i].rule_id = src->rules[i].rule_id;
		dest->rules[i].ncaptured = src->rules[i].ncaptured;
		rule_vinit(src->n_samples, &(dest->rules[i].captures));
		rule_copy(dest->rules[i].captures,
		    src->rules[i].captures, src->n_samples);
	}
	*ret_dest = dest;

	return (0);
}

/* Reclaim resources associated with a ruleset. */
void
ruleset_destroy(ruleset_t *rs)
{
	for (int j = 0; j < rs->n_rules; j++)
		rule_vfree(&rs->rules[j].captures);
	free(rs);
}

/*
 * Add the specified rule to the ruleset at position ndx (shifting
 * all rules after ndx down by one).
 */
int
ruleset_add(rule_t *rules, int nrules, ruleset_t **rsp, int newrule, int ndx)
{
	int i, cnt;
	ruleset_t *expand, *rs;
	ruleset_entry_t *cur_re;
	VECTOR not_caught;

	rs = *rsp;

	/* Check for space. */
	if (rs->n_alloc < rs->n_rules + 1) {
		expand = (ruleset_t*)realloc(rs, sizeof(ruleset_t) +
		    (rs->n_rules + 1) * sizeof(ruleset_entry_t));
		if (expand == NULL)
			return (errno);
		rs = expand;
		rs->n_alloc = rs->n_rules + 1;
		*rsp = rs;
	}

	/*
	 * Compute all the samples that are caught by rules AFTER the
	 * rule we are inserting. Then we'll recompute all the captures
	 * from the new rule to the end.
	 */
	rule_vinit(rs->n_samples, &not_caught);
	for (i = ndx; i < rs->n_rules; i++)
	    rule_vor(not_caught,
	        not_caught, rs->rules[i].captures, rs->n_samples, &cnt);


	/*
	 * Shift later rules down by 1 if necessary. For GMP, what we're
	 * doing may be a little sketchy -- we're copying the mpz_t's around.
	 */
	if (ndx != rs->n_rules) {
		memmove(rs->rules + (ndx + 1), rs->rules + ndx,
		    sizeof(ruleset_entry_t) * (rs->n_rules - ndx));
	}

	/* Insert and initialize the new rule. */
	rs->n_rules++;
	rs->rules[ndx].rule_id = newrule;
	rule_vinit(rs->n_samples, &rs->rules[ndx].captures);

	/*
	 * Now, recompute all the captures entries for the new rule and
	 * all rules following it.
	 */

	for (i = ndx; i < rs->n_rules; i++) {
		cur_re = rs->rules + i;
		/*
		 * Captures for this rule gets anything in not_caught
		 * that is also in the rule's truthtable.
		 */
		rule_vand(cur_re->captures,
		    not_caught, rules[cur_re->rule_id].truthtable,
		    rs->n_samples, &cur_re->ncaptured);

		rule_vandnot(not_caught,
		    not_caught, cur_re->captures, rs->n_samples, &cnt);
	}
	assert(cnt == 0);
	rule_vfree(&not_caught);

	return(0);
}

/*
 * Delete the rule in the ndx-th position in the given ruleset.
 */
void
ruleset_delete(rule_t *rules, int nrules, ruleset_t *rs, int ndx)
{
	int i, nset;
	VECTOR tmp_vec;
	ruleset_entry_t *old_re, *cur_re;

	/* Compute new captures for all rules following the one at ndx.  */
	old_re = rs->rules + ndx;

	if (rule_vinit(rs->n_samples, &tmp_vec) != 0)
		return;
	for (i = ndx + 1; i < rs->n_rules; i++) {
		/*
		 * My new captures is my old captures or'd with anything that
		 * was captured by ndx and is captured by my rule.
		 */
		cur_re = rs->rules + i;
		rule_vand(tmp_vec, rules[cur_re->rule_id].truthtable,
		    old_re->captures, rs->n_samples, &nset);
		rule_vor(cur_re->captures, cur_re->captures,
		    tmp_vec, rs->n_samples, &rs->rules[i].ncaptured);

		/*
		 * Now remove the ones from old_re->captures that just got set
		 * for rule i because they should not be captured later.
		 */
		rule_vandnot(old_re->captures, old_re->captures,
		    cur_re->captures, rs->n_samples, &nset);
	}

	/* Now remove alloc'd data for rule at ndx and for tmp_vec. */
	rule_vfree(&tmp_vec);
	rule_vfree(&rs->rules[ndx].captures);

	/* Shift up cells if necessary. */
	if (ndx != rs->n_rules - 1)
		memmove(rs->rules + ndx, rs->rules + ndx + 1,
		    sizeof(ruleset_entry_t) * (rs->n_rules - 1 - ndx));

	rs->n_rules--;
	return;
}

/* dest must exist */
void
rule_copy(VECTOR dest, VECTOR src, int len)
{
#ifdef GMP
	mpz_set(dest, src);
#else
	int i, nentries;

	assert(dest != NULL);
	nentries = (len + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
	for (i = 0; i < nentries; i++)
		dest[i] = src[i];
#endif
}

/*
 * Swap rules i and j such that i + 1 = j.
 *	j.captures = j.captures | (i.captures & j.tt)
 *	i.captures = i.captures & ~j.captures
 * 	then swap positions i and j
 */
int
ruleset_swap(ruleset_t *rs, int i, int j, rule_t *rules)
{
	int nset;
	VECTOR tmp_vec;
	ruleset_entry_t re;

	assert(i < (rs->n_rules - 1));
	assert(j < (rs->n_rules - 1));
	assert(i + 1 == j);

	rule_vinit(rs->n_samples, &tmp_vec);

	/* tmp_vec =  i.captures & j.tt */
	rule_vand(tmp_vec, rs->rules[i].captures,
	    rules[rs->rules[j].rule_id].truthtable, rs->n_samples, &nset);
	/* j.captures = j.captures | tmp_vec */
	rule_vor(rs->rules[j].captures, rs->rules[j].captures,
	    tmp_vec, rs->n_samples, &rs->rules[j].ncaptured);

	/* i.captures = i.captures & ~j.captures */
	rule_vandnot(rs->rules[i].captures, rs->rules[i].captures,
	    rs->rules[j].captures, rs->n_samples, &rs->rules[i].ncaptured);

	/* Now swap the two entries */
	re = rs->rules[i];
	rs->rules[i] = rs->rules[j];
	rs->rules[j] = re;

	rule_vfree(&tmp_vec);
	return (0);
}

int
ruleset_swap_any(ruleset_t * rs, int i, int j, rule_t * rules)
{
	int temp, cnt, cnt_check, ret;
	VECTOR caught;

	if (i == j)
		return 0;

	assert(i < rs->n_rules);
	assert(j < rs->n_rules);

	/* Ensure that i < j. */
	if (i > j) {
		temp = i;
		i = j;
		j = temp;
	}

	/*
	 * The captured arrays before i and after j need not change.
	 * We first compute everything caught between rules i and j
	 * (inclusive) and then compute the captures array from scratch
	 * rules between rule i and rule j, both * inclusive.
	 */
	if ((ret = rule_vinit(rs->n_samples, &caught)) != 0)
		return (ret);

	for (int k = i; k <= j; k++)
		rule_vor(caught,
		    caught, rs->rules[k].captures, rs->n_samples, &cnt);

	/* Now swap the rules in the ruleset. */
	temp = rs->rules[i].rule_id;
	rs->rules[i].rule_id = rs->rules[j].rule_id;
	rs->rules[j].rule_id = temp;

	cnt_check = 0;
	for (int k = i; k <= j; k++) {
		/*
		 * Compute the items captured by rule k by anding the caught
		 * vector with the truthtable of the kth rule.
		 */
		rule_vand(rs->rules[k].captures, caught,
		    rules[rs->rules[k].rule_id].truthtable,
		    rs->n_samples, &rs->rules[k].ncaptured);
		cnt_check += rs->rules[k].ncaptured;

		/* Now remove the caught items from the caught vector. */
		rule_vandnot(caught,
		    caught, rs->rules[k].captures, rs->n_samples, &temp);
	}
	assert(temp == 0);
	assert(cnt == cnt_check);

	rule_vfree(&caught);
	return (0);
}

/*
 * Dest must have been created.
 */
void
rule_vand(VECTOR dest, VECTOR src1, VECTOR src2, int nsamples, int *cnt)
{
#ifdef GMP
	mpz_and(dest, src1, src2);
	*cnt = 0;
	*cnt = mpz_popcount(dest);
#else
	int i, count, nentries;

	count = 0;
	nentries = (nsamples + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
	assert(dest != NULL);
	for (i = 0; i < nentries; i++) {
		dest[i] = src1[i] & src2[i];
		count += count_ones(dest[i]);
	}
	*cnt = count;
	return;
#endif
}

/* Dest must have been created. */
void
rule_vor(VECTOR dest, VECTOR src1, VECTOR src2, int nsamples, int *cnt)
{
#ifdef GMP
	mpz_ior(dest, src1, src2);
	*cnt = mpz_popcount(dest);
#else
	int i, count, nentries;

	count = 0;
	nentries = (nsamples + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;

	for (i = 0; i < nentries; i++) {
		dest[i] = src1[i] | src2[i];
		count += count_ones(dest[i]);
	}
	*cnt = count;

	return;
#endif
}

/*
 * Dest must exist.
 * We use this to update existing vectors, so it has to work if the dest and
 * src1 are the same. It's easy in the naive implementation, but trickier in
 * the mpz because you can't do the AND and NOT at the same time. We benchmarked
 * allocating a temporary against doing this in 3 ops without the temporary and
 * the temporary is significantly faster (for large vectors).
 */
void
rule_vandnot(VECTOR dest,
    VECTOR src1, VECTOR src2, int nsamples, int *ret_cnt)
{
#ifdef GMP
	mpz_t tmp;

	rule_vinit(nsamples, &tmp);
	mpz_com(tmp, src2);
	mpz_and(dest, src1, tmp);
	*ret_cnt = 0;
	*ret_cnt = mpz_popcount(dest);
	rule_vfree(&tmp);
#else
	int i, count, nentries;

	nentries = (nsamples + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
	count = 0;
	assert(dest != NULL);
	for (i = 0; i < nentries; i++) {
		dest[i] = src1[i] & (~src2[i]);
		count += count_ones(dest[i]);
	}

	*ret_cnt = count;
    return;
#endif
}

/*
 * Doesn't handle two's complement
 */
void
rule_not(VECTOR dest, VECTOR src, int nsamples, int *ret_cnt)
{
#ifdef GMP
    mpz_com(dest, src);
    *ret_cnt = 0;
    *ret_cnt = mpz_popcount(dest);
#else
	int i, count, nentries;

	nentries = (nsamples + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
	count = 0;
	assert(dest != NULL);
	for (i = 0; i < nentries; i++) {
		dest[i] = ~src[i];
		count += count_ones(dest[i]);
	}
	*ret_cnt = count;
    return;
#endif
}

int
count_ones_vector(VECTOR v, int len) {
#ifdef GMP
	return mpz_popcount(v);
#else
    int cnt = 0;
    for (size_t i=0; i < (len+BITS_PER_ENTRY-1)/BITS_PER_ENTRY; i++) {
        cnt += count_ones(v[i]);
    }
    return cnt;
#endif
}

int
count_ones(v_entry val)
{
	int count;

	count = 0;
	for (size_t i = 0; i < sizeof(v_entry); i++) {
		count += byte_ones[val & BYTE_MASK];
		val >>= 8;
	}
	return (count);
}

void
ruleset_print(ruleset_t *rs, rule_t *rules, int detail)
{
	int i, n;
	int total_support;

	printf("%d rules %d samples\n", rs->n_rules, rs->n_samples);
	n = rs->n_samples;

	total_support = 0;
	for (i = 0; i < rs->n_rules; i++) {
		rule_print(rules, rs->rules[i].rule_id, n, detail);
		ruleset_entry_print(rs->rules + i, n, detail);
		total_support += rs->rules[i].ncaptured;
	}
	printf("Total Captured: %d\n", total_support);
}

void
ruleset_entry_print(ruleset_entry_t *re, int nsamples, int detail)
{
	printf("%d captured; \n", re->ncaptured);
	if (detail)
		rule_vector_print(re->captures, nsamples);
}

void
rule_print(rule_t *rules, int ndx, int nsamples, int detail)
{
    rule_t *r;

    r = rules + ndx;
    printf("RULE %d: ( %s ), support=%d, card=%d",
        ndx, r->features, r->support, r->cardinality);
    if (detail) {
        printf(":");
        rule_vector_print(r->truthtable, nsamples);
    }
    else
        printf("\n");
}

void
rule_vector_print(VECTOR v, int nsamples)
{
#ifdef GMP
	char* str = mpz_get_str(NULL, 2, v);
    int len = strlen(str);
    for(int i = 0; i < (nsamples - len); i++) {
      printf("0");
    }
    printf("%s\n", str);
#else
    v_entry m = ~(((v_entry) -1) >> 1);
    unsigned n = (nsamples + BITS_PER_ENTRY - 1) / BITS_PER_ENTRY;
    for(unsigned i = 0; i < n; i++) {
        v_entry t = v[i];
        for(unsigned j = 0; j < BITS_PER_ENTRY && (i * BITS_PER_ENTRY + j) < (unsigned)nsamples; j++) {
            printf("%d", !!(t & m));
            t <<= 1;
        }
    }
    printf("\n");
#endif

}

void
rule_print_all(rule_t *rules, int nrules, int nsamples, int detail)
{
	int i;

	for (i = 0; i < nrules; i++)
		rule_print(rules, i, nsamples, detail);
}

/*
 * Return 0 if bit e is not set in vector v; return non-0 otherwise.
 */
int
rule_isset(VECTOR v, int e, int n) {
#ifdef GMP
	return mpz_tstbit(v, e);
#else
    e = -e + n - 1;
    if(e >= n) {
        return 0;
    }

    v_entry one = 1;
    int shift = BITS_PER_ENTRY - (e % BITS_PER_ENTRY) - 1;
/*    if(e / BITS_PER_ENTRY == ((n + BITS_PER_ENTRY - 1) / BITS_PER_ENTRY - 1) && (n % BITS_PER_ENTRY) != 0) {
        shift -= BITS_PER_ENTRY - (n % BITS_PER_ENTRY);
    }
*/
	return !!(v[e / BITS_PER_ENTRY] & (one << shift));
#endif
}

void
rule_set(VECTOR v, int e, int val, int n) {
#ifdef GMP
    if(val)
        mpz_setbit(v, e);
    else
        mpz_clrbit(v, e);
#else
    e = -e + n - 1;
    if(e >= n) {
        return;
    }

    v_entry one = 1;
    int shift = BITS_PER_ENTRY - (e % BITS_PER_ENTRY) - 1;
    // Only needed if the last v_entry is not full and right-aligned
/*    if(e / BITS_PER_ENTRY == ((n + BITS_PER_ENTRY - 1) / BITS_PER_ENTRY - 1) && (n % BITS_PER_ENTRY) != 0) {
        shift -= BITS_PER_ENTRY - (n % BITS_PER_ENTRY);
    }
*/
    if(val)
        v[e / BITS_PER_ENTRY] |= (one << shift);
    else
	    v[e/BITS_PER_ENTRY] &= ((v_entry) -1) - (one << shift);
#endif
}

int
rule_vector_cmp(const VECTOR v1, const VECTOR v2, int len1, int len2) {
#ifdef GMP
    return mpz_cmp(v1, v2);
#else
    if (len1 != len2)
        return 2 * (len1 > len2) - 1;
    size_t nentries = (len1 + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
    for (size_t i = 0; i < nentries; i++) {
        if (v1[i] != v2[i])
            return 2 * (v1[i] > v2[i]) - 1;
    }
    return 0;
#endif
}

size_t
rule_vector_hash(const VECTOR v, short len) {
        unsigned long hash = 0;
		size_t i;
#ifdef GMP
        for(i = 0; i < mpz_size(v); ++i)
            hash = mpz_getlimbn(v, i) + (hash << 6) + (hash << 16) - hash;
#else
   		size_t nentries = (len + BITS_PER_ENTRY - 1)/BITS_PER_ENTRY;
		for(i = 0; i < nentries; ++i)
            hash = v[i] + (hash << 6) + (hash << 16) - hash;
#endif
        return hash;
}
