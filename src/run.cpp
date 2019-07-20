#include <stdio.h>
#include <iostream>
#include <set>

#include "queue.hh"
#include "run.h"

#define BUFSZ 512

NullLogger* logger = nullptr;
static PermutationMap* g_pmap = nullptr;
static CacheTree* g_tree = nullptr;
static Queue* g_queue = nullptr;
static double g_init = 0.0;
static std::set<std::string> g_verbosity;

char* m_strsep(char** stringp, char delim)
{
    if(stringp == NULL) {
        return NULL;
    }
    
    char* str = *stringp;
    if(str == NULL || *str == '\0') {
        return NULL;
    }

    char* out = NULL;

    while(1) {
        if(*str == delim || *str == '\0') {
            out = *stringp;
	        *stringp = (*str == '\0') ? NULL : str + 1;
            *str = '\0';
            break;
	    }
	    str++;
    }

    return out;
}

#ifndef _WIN32
#define _strdup strdup
#endif

int run_corels_begin(double c, char* vstring, int curiosity_policy,
                  int map_type, int ablation, int calculate_size, int nrules, int nlabels,
                  int nsamples, rule_t* rules, rule_t* labels, rule_t* meta) 
{
    g_verbosity.clear();

    const char *voptions = "rule|label|samples|progress|loud|mine|minor";

    char *vopt = NULL;
    char *vcopy = _strdup(vstring);
    char *vcopy_begin = vcopy;
    while ((vopt = m_strsep(&vcopy, ',')) != NULL) {
        if (!strstr(voptions, vopt)) {
            fprintf(stderr, "verbosity options must be one or more of (%s)\n", voptions);
            return -1;
        }
        g_verbosity.insert(vopt);
    }
    free(vcopy_begin);

    if (g_verbosity.count("loud")) {
        g_verbosity.insert("progress");
        g_verbosity.insert("label");
        g_verbosity.insert("rule");
        g_verbosity.insert("mine");
        g_verbosity.insert("minor");
    }

#ifndef GMP
    if (g_verbosity.count("progress"))
        printf("**Not using GMP library**\n");
#endif

    if (g_verbosity.count("rule")) {
        printf("%d rules %d samples\n\n", nrules, nsamples);
        rule_print_all(rules, nrules, nsamples, g_verbosity.count("samples"));
        printf("\n\n");
    }

    if (g_verbosity.count("label")) {
        printf("Labels (%d) for %d samples\n\n", nlabels, nsamples);
        rule_print_all(labels, nlabels, nsamples, g_verbosity.count("samples"));
        printf("\n\n");
    }
    
    if (g_verbosity.count("minor") && meta) {
        printf("Minority bound for %d samples\n\n", nsamples);
        rule_print_all(meta, 1, nsamples, g_verbosity.count("samples"));
        printf("\n\n");
    }
    
    if(!logger)
        logger = new PyLogger();
   
    if (g_tree)
        delete g_tree;
    g_tree = nullptr;

    if (g_queue)
        delete g_queue;
    g_queue = nullptr;

    if (g_pmap)
        delete g_pmap;
    g_pmap = nullptr;
    
    int v = 0;
    if (g_verbosity.count("loud"))
        v = 1000;
    else if (g_verbosity.count("progress"))
        v = 1;

    logger->setVerbosity(v);

    g_init = timestamp();
    char run_type[BUFSZ];
    strcpy(run_type, "LEARNING RULE LIST via ");
    char const *type = "node";
    if (curiosity_policy == 1) {
        strcat(run_type, "CURIOUS");
        g_queue = new Queue(curious_cmp, run_type);
        type = "curious";
    } else if (curiosity_policy == 2) {
        strcat(run_type, "LOWER BOUND");
        g_queue = new Queue(lb_cmp, run_type);
    } else if (curiosity_policy == 3) {
        strcat(run_type, "OBJECTIVE");
        g_queue = new Queue(objective_cmp, run_type);
    } else if (curiosity_policy == 4) {
        strcat(run_type, "DFS");
        g_queue = new Queue(dfs_cmp, run_type);
    } else {
        strcat(run_type, "BFS");
        g_queue = new Queue(base_cmp, run_type);
    }

    if (map_type == 1) {
        strcat(run_type, " Prefix Map\n");
        PrefixPermutationMap* prefix_pmap = new PrefixPermutationMap;
        g_pmap = (PermutationMap*) prefix_pmap;
    } else if (map_type == 2) {
        strcat(run_type, " Captured Symmetry Map\n");
        CapturedPermutationMap* cap_pmap = new CapturedPermutationMap;
        g_pmap = (PermutationMap*) cap_pmap;
    } else {
        strcat(run_type, " No Permutation Map\n");
        NullPermutationMap* null_pmap = new NullPermutationMap;
        g_pmap = (PermutationMap*) null_pmap;
    }

    g_tree = new CacheTree(nsamples, nrules, c, rules, labels, meta, ablation, calculate_size, type);
    if (g_verbosity.count("progress"))
        printf("%s", run_type);

    bbound_begin(g_tree, g_queue);

    return 0;
}

int run_corels_loop(size_t max_num_nodes) {
    if((g_tree->num_nodes() < max_num_nodes) && !g_queue->empty()) {
        bbound_loop(g_tree, g_queue, g_pmap);
        return 0;
    }
    return -1;
}

double run_corels_end(int** rulelist, int* rulelist_size, int** classes, int early)
{
    bbound_end(g_tree, g_queue, g_pmap, early);

    const tracking_vector<unsigned short, DataStruct::Tree>& r_list = g_tree->opt_rulelist();
    const tracking_vector<bool, DataStruct::Tree>& preds = g_tree->opt_predictions();

    double accuracy = 1.0 - g_tree->min_objective() + g_tree->c() * r_list.size();

    *rulelist = (int*)malloc(sizeof(int) * r_list.size());
    *classes = (int*)malloc(sizeof(int) * (1 + r_list.size()));
    *rulelist_size = r_list.size();
    for(size_t i = 0; i < r_list.size(); i++) {
        (*rulelist)[i] = r_list[i];
        (*classes)[i] = preds[i];
    }
    (*classes)[r_list.size()] = preds.back();

    if (g_verbosity.count("progress")) {
        printf("final num_nodes: %zu\n", g_tree->num_nodes());
        printf("final num_evaluated: %zu\n", g_tree->num_evaluated());
        printf("final min_objective: %1.5f\n", g_tree->min_objective());
        printf("final accuracy: %1.5f\n", accuracy);
        printf("final total time: %f\n", time_diff(g_init));
    }

    if(g_tree)
        delete g_tree;
    g_tree = nullptr;

    if(g_pmap)
        delete g_pmap;
    g_pmap = nullptr;
    
    if(g_queue)
        delete g_queue;
    g_queue = nullptr;
   
    return accuracy;
}
