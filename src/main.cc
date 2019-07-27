#include "queue.hh"
#include <iostream>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include "run.hh"

#define BUFSZ 512

int main(int argc, char *argv[]) {
    const char usage[] = "USAGE: %s [-b] "
        "[-n max_num_nodes] [-r regularization] [-v (rule|label|minor|samples|progress|loud|silent)] "
        "-c (1|2|3|4) -p (0|1|2) [-f logging_frequency] "
        "-a (0|1|2) [-s] [-L latex_out]"
        "data.out data.label [data.minor]\n\n"
        "%s\n";

    extern char *optarg;
    bool run_bfs = false;
    bool run_curiosity = false;
    int curiosity_policy = 0;
    bool latex_out = false;
    bool use_prefix_perm_map = false;
    bool use_captured_sym_map = false;
    char *vopt, *verb_trim;
    std::set<std::string> verbosity;
    bool verr = false;
    const char *vstr = "rule|label|minor|samples|progress|loud|silent";
    int map_type = 0;
    int max_num_nodes = 100000;
    double c = 0.01;
    char ch;
    bool error = false;
    char error_txt[BUFSZ];
    int freq = 1000;
    int ablation = 0;
    bool calculate_size = false;
    char verbstr[BUFSZ]; 
    verbstr[0] = '\0';
    /* only parsing happens here */
    while ((ch = getopt(argc, argv, "bsLc:p:v:n:r:f:a:u:")) != -1) {
        switch (ch) {
        case 'b':
            run_bfs = true;
            break;
        case 's':
            calculate_size = true;
            break;
        case 'c':
            run_curiosity = true;
            curiosity_policy = atoi(optarg);
            break;
        case 'L':
            latex_out = true;
            break;
        case 'p':
            map_type = atoi(optarg);
            use_prefix_perm_map = map_type == 1;
            use_captured_sym_map = map_type == 2;
            break;
        case 'v':
            verb_trim = strtok(optarg, " ");
            strcpy(verbstr, verb_trim);
            vopt = strtok(verb_trim, ",");
            while (vopt != NULL) {
                if (!strstr(vstr, vopt)) {
                    verr = true;
                }
                verbosity.insert(vopt);
                vopt = strtok(NULL, ",");
            }
            break;
        case 'n':
            max_num_nodes = atoi(optarg);
            break;
        case 'r':
            c = atof(optarg);
            break;
        case 'f':
            freq = atoi(optarg);
            break;
        case 'a':
            ablation = atoi(optarg);
            break;
        default:
            error = true;
            snprintf(error_txt, BUFSZ, "unknown option: %c", ch);
        }
    }
    if (max_num_nodes < 0) {
        error = true;
        snprintf(error_txt, BUFSZ, "number of nodes must be positive");
    }
    if (c < 0) {
        error = true;
        snprintf(error_txt, BUFSZ, "regularization constant must be postitive");
    }
    if (map_type > 2 || map_type < 0) {
        error = true;
        snprintf(error_txt, BUFSZ, "symmetry-aware map must be (0|1|2)");
    }
    if ((run_bfs + run_curiosity) != 1) {
        error = true;
        snprintf(error_txt, BUFSZ,
                "you must use exactly one of (-b | -c)");
    }
    if (argc < 2 + optind) {
        error = true;
        snprintf(error_txt, BUFSZ,
                "you must specify data files for rules and labels");
    }
    if (run_curiosity && !((curiosity_policy >= 1) && (curiosity_policy <= 4))) {
        error = true;
        snprintf(error_txt, BUFSZ,
                "you must specify a curiosity type (1|2|3|4)");
    }
    if (verr) {
        error = true;
        snprintf(error_txt, BUFSZ,
                 "verbosity options must be one or more of (rule|label|samples|progress|loud|silent), separated with commas (i.e. -v progress,samples)");
    }
    else {
        if (verbosity.count("samples") && !(verbosity.count("rule") || verbosity.count("label") || verbosity.count("minor") || verbosity.count("loud"))) {
            error = true;
            snprintf(error_txt, BUFSZ,
                     "verbosity 'samples' option must be combined with at least one of (rule|label|minor|samples|progress|loud|silent)");
        }
        if (verbosity.size() > 1 && verbosity.count("silent")) {
            snprintf(error_txt, BUFSZ,
                     "verbosity 'silent' option must be passed without any additional verbosity parameters");
        }
    }

    if (error) {
        fprintf(stderr, usage, argv[0], error_txt);
        return 1;
    }

    if (verbosity.size() == 0) {
        verbosity.insert("progress");
        strcpy(verbstr, "progress");
    }

    if (verbosity.count("silent")) {
        verbosity.clear();
        verbstr[0] = '\0';
    }
    
    std::map<int, std::string> curiosity_map;
    curiosity_map[1] = "curiosity";
    curiosity_map[2] = "curious_lb";
    curiosity_map[3] = "curious_obj";
    curiosity_map[4] = "dfs";

    argc -= optind;
    argv += optind;

    int nrules, nsamples, nlabels, nsamples_chk;
    rule_t *rules, *labels;
    
    if(rules_init(argv[0], &nrules, &nsamples, &rules, 1) != 0) {
        fprintf(stderr, "Failed to load out file from path: %s\n", argv[0]);
        return 1;
    }
    
    if(rules_init(argv[1], &nlabels, &nsamples_chk, &labels, 0) != 0) {
        fprintf(stderr, "Failed to load label file from path: %s\n", argv[1]);
        rules_free(rules, nrules, 1);
        return 1;
    }
    
    if(nlabels != 2) {
        fprintf(stderr, "nlabels must be equal to 2\n");
        rules_free(rules, nrules, 1);
        rules_free(labels, nlabels, 0);
        return 1;
    }
    
    if(nsamples_chk != nsamples) {
        fprintf(stderr, "nsamples mismatch between out file (%d) and label file (%d)\n", nsamples, nsamples_chk);
        rules_free(rules, nrules, 1);
        rules_free(labels, nlabels, 0);
        return 1;
    }

    int nmeta, nsamples_check;
    // Equivalent points information is precomputed, read in from file, and stored in meta
    rule_t *meta;
    if (argc == 3) {
        if(rules_init(argv[2], &nmeta, &nsamples_check, &meta, 0) != 0) {
            fprintf(stderr, "Failed to load minor file from path: %s, skipping...\n", argv[2]);
            meta = NULL;
            nmeta = 0;
        }
        else if(nsamples_check != nsamples) {
            fprintf(stderr, "nsamples mismatch between out file (%d) and minor file (%d), skipping minor file...\n", nsamples, nsamples_check);
            rules_free(meta, nmeta, 0);
            meta = NULL;
            nmeta = 0;
        }
    }
    else
        meta = NULL;
    
    char froot[BUFSZ];
    char log_fname[BUFSZ];
    char opt_fname[BUFSZ];
    const char* pch = strrchr(argv[0], '/');
    snprintf(froot, BUFSZ, "../logs/for-%s-%s%s-%s-%s-removed=%s-max_num_nodes=%d-c=%.7f-v=%s-f=%d",
            pch ? pch + 1 : "",
            run_bfs ? "bfs" : "",
            run_curiosity ? curiosity_map[curiosity_policy].c_str() : "",
            use_prefix_perm_map ? "with_prefix_perm_map" :
                (use_captured_sym_map ? "with_captured_symmetry_map" : "no_pmap"),
            meta ? "minor" : "no_minor",
            ablation ? ((ablation == 1) ? "support" : "lookahead") : "none",
            max_num_nodes, c, verbstr, freq);
    snprintf(log_fname, BUFSZ, "%s.txt", froot);
    snprintf(opt_fname, BUFSZ, "%s-opt.txt", froot);

    int ret = 0;

    if(run_corels_begin(c, &verbstr[0], curiosity_policy, map_type, ablation, calculate_size,
                        nrules, nlabels, nsamples, rules, labels, meta, freq, &log_fname[0]) == 0)
    {
        while(run_corels_loop(max_num_nodes) == 0) { }

        int* rulelist = NULL;
        int rulelist_size = 0;
        int* classes = NULL;
    
        run_corels_end(&rulelist, &rulelist_size, &classes, 0, latex_out, rules, labels, &opt_fname[0]);

        if(rulelist)
            free(rulelist);

        if(classes)
            free(classes);
    } else {
        fprintf(stderr, "Setup failed!\n");
        ret = 2;
    }
    
    if(meta) {
        rules_free(meta, nmeta, 0);
    }
    rules_free(rules, nrules, 1);
    rules_free(labels, nlabels, 0);
        
    return ret;
}
