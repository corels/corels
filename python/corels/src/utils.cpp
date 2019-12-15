#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "utils.hh"

static rule_t *sample_array = NULL;
static int samples_nrules = 0;

int sample_comp(const void *a, const void *b) {
  return rule_vector_cmp(sample_array[*(int*)a].truthtable, sample_array[*(int*)b].truthtable,
                         samples_nrules, samples_nrules);
}

int minority(rule_t* rules, int nrules, rule_t* labels, int nsamples, rule_t* minority_out, int verbose)
{
  if(!rules || !labels || !minority_out) {
    return -1;
  }
  int ret = 0, begin_group = 0, nones;
  int *sample_indices = NULL;
  char *line_clean = NULL, *minority = NULL;

  samples_nrules = nrules;

  line_clean = (char*)malloc(nrules + 1);
  sample_array = (rule_t*)malloc(sizeof(rule_t) * nsamples);
  minority = (char*)malloc(nsamples + 1);
  sample_indices = (int*)malloc(sizeof(int) * nsamples);
  
  // Generate the sample bitvectors
  for(int s = 0; s < nsamples; s++) {
    for(int i = 0; i < nrules; i++)
      line_clean[i] = rule_isset(rules[i].truthtable, nsamples - s - 1, nsamples) + '0';

    line_clean[nrules] = '\0';

    if(ascii_to_vector(line_clean, nrules, &nrules, &nones, &sample_array[s].truthtable) != 0) {
      ret = -1;
      nsamples = s;
      goto end;
    }
  }

  for(int i = 0; i < nsamples; i++)
    sample_indices[i] = i;
  
  // Sort the samples by value (this groups those samples that are identically featured together)
  qsort(sample_indices, nsamples, sizeof(int), sample_comp);

  // Loop through the sorted samples, finding identically-featured groups
  for(int i = 1; i < (nsamples + 1); i++) {
    if(i == nsamples || rule_vector_cmp(sample_array[sample_indices[i]].truthtable,
                            sample_array[sample_indices[i-1]].truthtable, nrules, nrules) != 0) {
      int ones = 0;
      int zeroes = 0;
      char c, nc;
      // Find the number of zero-labelled and one-labelled samples in this group
      for(int j = begin_group; j < i; j++) {
        int idx = sample_indices[j];
        if(rule_isset(labels[0].truthtable, nsamples - idx - 1, nsamples))
          zeroes++;
        else
          ones++;
      }

      // What should happen if zeroes == ones??
      // Right now it just replicates bbcache/processing/minority.py
      if(zeroes < ones) {
        c = '1';
        nc = '0';
      }
      else {
        c = '0';
        nc = '1';
      }
      
      // Set all the samples in this group to either 0 or 1 in the minority file
      for(int j = begin_group; j < i; j++) {
        int idx = sample_indices[j];
        if(rule_isset(labels[0].truthtable, nsamples - idx - 1, nsamples)) {
          minority[idx] = c;
        }
        else {
          minority[idx] = nc;
        }
      }

      begin_group = i;
    }
  }
  
  minority[nsamples] = '\0';
 
  minority_out->support = 0;
  if (ascii_to_vector(minority, nsamples, &nsamples, &minority_out->support, &minority_out->truthtable) != 0) {
    ret = -1;
    goto end;
  }

  minority_out->cardinality = 1;
  minority_out->features = (char*)malloc(9);
  strcpy(minority_out->features, "minority");
  minority_out->ids = NULL;

  if(verbose)
    printf("Generated minority bound with support %f\n", (double)minority_out->support / (double)nsamples);

end:
  if(line_clean)
    free(line_clean);

  if(sample_array) {
    for(int i = 0; i < nsamples; i++)
      rule_vfree(&sample_array[i].truthtable);

    free(sample_array);
    sample_array = NULL;
  }

  samples_nrules = 0;

  if(minority)
    free(minority);

  if(sample_indices)
    free(sample_indices);

  return ret;
}

// Cycles through all possible permutations of the numbers 1 through n-1 of length r
int getnextperm(int n, int r, int *arr, int first)
{
  // Initialization case
  if(first) {
    for(int i = 0; i < r; i++)
      arr[i] = i;

    return 0;
  }

  for(int i = 1; i < (r + 1); i++) {
    if(arr[r - i] < (n - i)) {
      arr[r - i]++;
      for(int j = (r - i + 1); j < r; j++)
        arr[j] = arr[r - i] + (j - r + i);
      return 0;
    }
  }

  return -1;
}

int mine_rules(char **features, rule_t *samples, int nfeatures, int nsamples, 
                int max_card, double min_support, rule_t **rules_out, int verbose)
{
  if(!samples || !features) {
    return -1;
  }
  int ntotal_rules = 0, nrules = 0, rule_alloc = 0, rule_alloc_block = 32, nrules_mine = 0, ret = 0;
  int *rule_ids = NULL, *rule_names_mine_lengths = NULL;
  rule_t *rules_vec = NULL, *rules_vec_mine = NULL;
  
  nrules = nfeatures * 2;
  rule_alloc = nrules + 1;
  rules_vec = (rule_t*)malloc(sizeof(rule_t) * rule_alloc);
  if(!rules_vec) {
      ret = -1;
      goto end;
  }

  rules_vec_mine = (rule_t*)malloc(sizeof(rule_t) * nrules);
  if(!rules_vec_mine) {
      ret = -1;
      goto end;
  }

  rule_names_mine_lengths = (int*)malloc(sizeof(int) * nrules);
  if(!rule_names_mine_lengths) {
      ret = -1;
      goto end;
  }
 
  for(int i = 0; i < nrules; i++) {
    if(rule_vinit(nsamples, &rules_vec[i + 1].truthtable) != 0) {
      ntotal_rules = i;
      ret = -1;
      goto end;
    }
  }

  for(int i = 0; i < nfeatures; i++)
  {
    for(int j = 0; j < nsamples; j++)
    {
      if(rule_isset(samples[j].truthtable, nfeatures - i - 1, nfeatures)) {
        rule_set(rules_vec[i + 1].truthtable, nsamples - j - 1, 1, nsamples);
        rule_set(rules_vec[nrules / 2 + i + 1].truthtable, nsamples - j - 1, 0, nsamples);
      }
      else {
        rule_set(rules_vec[i + 1].truthtable, nsamples - j - 1, 0, nsamples);
        rule_set(rules_vec[nrules / 2 + i + 1].truthtable, nsamples - j - 1, 1, nsamples);
      }
    }
  }
  
  // File rules_vec, the mpz_t version of the rules array
  for(int i = 0; i < nrules; i++) {
    int ones = count_ones_vector(rules_vec[i + 1].truthtable, nsamples);
    
    // If the rule satisfies the threshold requirements, add it to the out file.
    // If it exceeds the maximum threshold, it is still kept for later rule mining
    // If it less than the minimum threshold, don't add it
    if((double)ones / (double)nsamples < min_support) {
      rule_vfree(&rules_vec[i + 1].truthtable);
      continue;
    }

    if(rule_vinit(nsamples, &rules_vec_mine[nrules_mine].truthtable) != 0) {
        ret = -1;
        goto end;
    }
    rule_copy(rules_vec_mine[nrules_mine].truthtable, rules_vec[i + 1].truthtable, nsamples);
    
    if(i < (nrules / 2)) {
      rules_vec_mine[nrules_mine].features = strdup(features[i]);
      rules_vec_mine[nrules_mine].cardinality = i + 1;
    }
    else {
      rules_vec_mine[nrules_mine].features = (char*)malloc(strlen(features[i - (nrules / 2)]) + 5);
      strcpy(rules_vec_mine[nrules_mine].features, features[i - (nrules / 2)]);
      strcat(rules_vec_mine[nrules_mine].features, "-not");
      rules_vec_mine[nrules_mine].cardinality = -(i - (nrules / 2)) - 1;
    }

    rule_names_mine_lengths[nrules_mine] = strlen(rules_vec_mine[nrules_mine].features);
    
    if((double)ones / (double)nsamples <= 1.0 - min_support) {
      memcpy(&rules_vec[ntotal_rules + 1], &rules_vec[i + 1], sizeof(rule_t));
      rules_vec[ntotal_rules + 1].cardinality = 1;
      rules_vec[ntotal_rules + 1].support = ones;
      rules_vec[ntotal_rules + 1].ids = (int*)malloc(sizeof(int));
      rules_vec[ntotal_rules + 1].ids[0] = rules_vec_mine[nrules_mine].cardinality;
      rules_vec[ntotal_rules + 1].cardinality = 1;
      rules_vec[ntotal_rules + 1].support = ones;

      if(i < (nrules / 2))
        rules_vec[ntotal_rules + 1].features = strdup(features[i]);
      else {
        rules_vec[ntotal_rules + 1].features = (char*)malloc(strlen(features[i - (nrules / 2)]) + 5);
        strcpy(rules_vec[ntotal_rules + 1].features, features[i - (nrules / 2)]);
        strcat(rules_vec[ntotal_rules + 1].features, "-not");
      }
   
      ntotal_rules++;
      
      if(verbose)
        printf("(%d) %s generated with support %f\n", ntotal_rules, rules_vec_mine[nrules_mine].features, (double)ones / (double)nsamples);
    }
    else
      rule_vfree(&rules_vec[i + 1].truthtable);

    nrules_mine++;
  }

  rule_t gen_rule;
  rule_vinit(nsamples, &gen_rule.truthtable);

  rule_ids = (int*)malloc(sizeof(int) * max_card);

  // Generate higher-cardinality rules
  for(int card = 2; card <= max_card; card++) {
    // getnextperm works sort of like strtok
    int r = getnextperm(nrules_mine, card, rule_ids, 1);

    while(r != -1) {
      int valid = 1;
      
      rule_copy(gen_rule.truthtable, rules_vec_mine[rule_ids[0]].truthtable, nsamples);
      int ones = count_ones_vector(gen_rule.truthtable, nsamples);

      // Generate the new rule by successive and operations, and check if it has a valid support
      if((double)ones / (double)nsamples >= min_support) {
        for(int i = 1; i < card; i++) {
          rule_vand(gen_rule.truthtable, rules_vec_mine[rule_ids[i]].truthtable, gen_rule.truthtable, nsamples, &ones);
          if((double)ones / (double)nsamples < min_support) {
            valid = 0;
            break;
          }
        }

        if(valid && (double)ones / (double)nsamples > 1.0 - min_support)
          valid = 0;
      }
      else
        valid = 0;

      if(valid) {
        ntotal_rules++;

        if(ntotal_rules + 1 > rule_alloc) {
          rule_alloc += rule_alloc_block;
          rules_vec = (rule_t*)realloc(rules_vec, sizeof(rule_t) * rule_alloc);
        }

        rule_vinit(nsamples, &rules_vec[ntotal_rules].truthtable);
        rule_copy(rules_vec[ntotal_rules].truthtable, gen_rule.truthtable, nsamples);

        int name_len = 0;
        for(int i = 0; i < card; i++)
          name_len += rule_names_mine_lengths[rule_ids[i]] + 1;
        
        rules_vec[ntotal_rules].features = (char*)malloc(name_len);

        int ch_id = 0;
        for(int i = 0; i < card; i++) {
          for(int j = 0; j < rule_names_mine_lengths[rule_ids[i]]; j++)
            rules_vec[ntotal_rules].features[ch_id + j] = rules_vec_mine[rule_ids[i]].features[j];
          
          ch_id += rule_names_mine_lengths[rule_ids[i]] + 1;

          rules_vec[ntotal_rules].features[ch_id - 1] = ',';
        }

        rules_vec[ntotal_rules].features[ch_id - 1] = '\0';
      
        rules_vec[ntotal_rules].cardinality = card;
        rules_vec[ntotal_rules].ids = (int*)malloc(sizeof(int) * card);
        for(int k = 0; k < card; k++)
          rules_vec[ntotal_rules].ids[k] = rules_vec_mine[rule_ids[k]].cardinality;

        rules_vec[ntotal_rules].support = ones;

        if(verbose) {
          printf("(%d) {", ntotal_rules);
          fputs(rules_vec_mine[rule_ids[0]].features, stdout);
          for(int i = 1; i < card; i++) {
            putchar(',');
            fputs(rules_vec_mine[rule_ids[i]].features, stdout);
          }
          printf("} generated with support %f\n", (double)ones / (double)nsamples);
        }
      }

      r = getnextperm(nrules_mine, card, rule_ids, 0);
    }
  }
  
  rules_vec[0].cardinality = 1;
  rules_vec[0].support = nsamples;
  rules_vec[0].ids = (int*)malloc(sizeof(int));
  rules_vec[0].ids[0] = 0;
  rules_vec[0].features = (char*)malloc(8);
  strcpy(rules_vec[0].features, "default"); 
  make_default(&rules_vec[0].truthtable, nsamples);

  ntotal_rules++;

end:
  rule_vfree(&gen_rule.truthtable);

  if(rule_ids)
    free(rule_ids);

  if(rule_names_mine_lengths)
    free(rule_names_mine_lengths);

  if(rules_vec_mine) {
    for(int i = 0; i < nrules_mine; i++) {
      rule_vfree(&rules_vec_mine[i].truthtable);
      free(rules_vec_mine[i].features);
    }

    free(rules_vec_mine);
  }

  if(verbose)
    printf("Generated %d rules\n", ntotal_rules - 1);

  if (ret == -1) {
    if(rules_vec) {
      for(int i = 0; i < ntotal_rules; i++) {
        rule_vfree(&rules_vec[i + 1].truthtable);
        free(rules_vec[i + 1].features);
      }

      free(rules_vec);
    }
    *rules_out = NULL;
    return -1;
  }
  else {
    *rules_out = rules_vec;
    return ntotal_rules;
  }
}
