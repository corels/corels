#include <stdio.h>
#include <iostream>
#include <set>

#include "queue.hh"
#include "run.hh"

#define BUFSZ 512

NullLogger* logger;

extern "C" {

double run_corels (run_params_t params) {

    std::set<std::string> verbosity;

    const char *voptions = "rule|label|samples|progress|log|silent";

    char *vopt = NULL;
    char *vcopy = strdup(params.vstring);
    while ((vopt = strsep(&vcopy, ",")) != NULL) {
        if (!strstr(voptions, vopt)) {
            fprintf(stderr, "verbosity options must be one or more of (%s), separated with commas (i.e. -v progress,log)\n", voptions);
            return -1.0;
        }
        verbosity.insert(vopt);
    }
    free(vcopy);

    if (verbosity.count("samples") && !(verbosity.count("rule") || verbosity.count("label"))) {
        fprintf(stderr, "verbosity 'samples' option must be combined with at least one of (rule|label)\n");
        return -1.0;
    }
    if (verbosity.size() > 2 && verbosity.count("silent")) {
        fprintf(stderr, "verbosity 'silent' option must be passed without any additional verbosity parameters\n");
        return -1.0;
    }

    if (verbosity.size() == 0) {
        verbosity.insert("progress");
    }

    if (verbosity.count("silent")) {
        verbosity.clear();
    }

    if (verbosity.count("log")) {
        print_machine_info();
    }

    if (verbosity.count("rule")) {
        printf("%d rules %d samples\n\n", params.nrules, params.nsamples);
        rule_print_all(params.rules, params.nrules, params.nsamples);
        printf("\n\n");
    }

    if (verbosity.count("label")) {
        printf("Labels (%d) for %d samples\n\n", params.nlabels, params.nsamples);
        rule_print_all(params.labels, params.nlabels, params.nsamples);
        printf("\n\n");
    }

    if (verbosity.count("log")) {
        logger = new Logger(params.c, params.nrules, 2, params.log_fname, params.freq);
    } else {
        logger = new NullLogger();
        logger->setVerbosity(0);
    }

    double init = timestamp();
    char run_type[BUFSZ];
    Queue* q;
    strcpy(run_type, "LEARNING RULE LIST via ");
    char const *type = "node";
    if (params.curiosity_policy == 1) {
        strcat(run_type, "CURIOUS");
        q = new Queue(curious_cmp, run_type);
        type = "curious";
    } else if (params.curiosity_policy == 2) {
        strcat(run_type, "LOWER BOUND");
        q = new Queue(lb_cmp, run_type);
    } else if (params.curiosity_policy == 3) {
        strcat(run_type, "OBJECTIVE");
        q = new Queue(objective_cmp, run_type);
    } else if (params.curiosity_policy == 4) {
        strcat(run_type, "DFS");
        q = new Queue(dfs_cmp, run_type);
    } else {
        strcat(run_type, "BFS");
        q = new Queue(base_cmp, run_type);
    }

    PermutationMap* p;
    if (params.map_type == 1) {
        strcat(run_type, " Prefix Map\n");
        PrefixPermutationMap* prefix_pmap = new PrefixPermutationMap;
        p = (PermutationMap*) prefix_pmap;
    } else if (params.map_type == 2) {
        strcat(run_type, " Captured Symmetry Map\n");
        CapturedPermutationMap* cap_pmap = new CapturedPermutationMap;
        p = (PermutationMap*) cap_pmap;
    } else {
        strcat(run_type, " No Permutation Map\n");
        NullPermutationMap* null_pmap = new NullPermutationMap;
        p = (PermutationMap*) null_pmap;
    }

    CacheTree* tree = new CacheTree(params.nsamples, params.nrules, params.c, params.rules, params.labels, params.meta, params.ablation, params.calculate_size, type);
    if (verbosity.count("progress"))
        printf("%s", run_type);
    // runs our algorithm
    bbound(tree, params.max_num_nodes, q, p);

    const tracking_vector<unsigned short, DataStruct::Tree>& r_list = tree->opt_rulelist();

    double accuracy = 1.0 - tree->min_objective() + params.c*r_list.size();

    if (verbosity.count("progress")) {
        printf("final num_nodes: %zu\n", tree->num_nodes());
        printf("final num_evaluated: %zu\n", tree->num_evaluated());
        printf("final min_objective: %1.5f\n", tree->min_objective());
        printf("final accuracy: %1.5f\n", accuracy);
   }

    print_final_rulelist(r_list, tree->opt_predictions(),
                     params.latex_out, params.rules, params.labels, params.opt_fname);

    if (verbosity.count("progress"))
        printf("final total time: %f\n", time_diff(init));

    logger->dumpState();
    logger->closeFile();

    if (verbosity.count("progress")) {
        printf("\ndelete tree\n");
    }
    delete tree;

    if (verbosity.count("progress")) {
        printf("\ndelete symmetry-aware map\n");
    }
    delete p;

    if (verbosity.count("progress")) {
        printf("\ndelete priority queue\n");
    }
    delete q;

    /*if (verbosity.count("progress")) {
        printf("\ndelete logger\n");
    }
    delete logger;*/

    return accuracy;
}

} // end extern C
