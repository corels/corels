
#include "queue.h"
#include "run.h"

#define STRICT_R_HEADERS
#include <Rcpp.h>

#define BUFSZ 512

/*
 * Logs statistics about the execution of the algorithm and dumps it to a file.
 * To turn off, pass verbosity <= 1
 */
//NullLogger* logger;

//' R Interface to 'Certifiably Optimal RulE ListS (Corels)'
//'
//' CORELS is a custom discrete optimization technique for building rule lists over a categorical feature space. The algorithm provides the optimal solution with a certificate of optimality. By leveraging algorithmic bounds, efficient data structures, and computational reuse, it achieves several orders of magnitude speedup in time and a massive reduction of memory consumption. This approach produces optimal rule lists on practical problems in seconds, and offers a novel alternative to CART and other decision tree methods.
//' @title Corels interace
//' @param rules_file Character variable with file name for training data; see corels documentation and data section below.
//' @param labels_file Character variable with file name for training data labels; see corels documentation and data section below.
//' @param log_dir Character variable with logfile directory name
//' @param meta_file Optional character variable with file name for minor data with bit vector to support equivalent points bound (see Theorem 20 in Section 3.14).
//' @param run_bfs Boolean toggle for \sQuote{breadth-first search}. Exactly one of \sQuote{breadth-first search} or \sQuote{curiosity_policy} \emph{must} be specified.
//' @param calculate_size Optional boolean toggle to calculate upper bound on remaining search space size which adds a small overheard; default is to not do this.
//' @param run_curiosity Boolean toggle
//' @param curiosity_policy Integer value (between 1 and 4) for best-fist search policy. Exactly one of \sQuote{breadth-first search} or \sQuote{curiosity_policy} \emph{must} be specified. The four different prirization schemes are chosen, respectively, by values of one for prioritize by curiousity (see Section 5.1 of the paper), two for prioritize by the lower bound, three for prioritize by the objective or four for depth-first search.
//' @param latex_out Optional boolean toggle to select LaTeX output of the output rule list.
//' @param map_type Optional integer value for the symmetry-aware map. Use zero for no symmetry-aware map (this is also the default), one for permutation map, and two for the captured vector map.
//' @param verbosity_policy Optional character variable one containing one or more of the terms \sQuote{rule}, \sQuote{label}, \sQuote{minor}, \sQuote{samples}, \sQuote{progress}, \sQuote{loud}, or \sQuote{silent}.
//' @param max_num_nodes Integer value for the maximum trie cache size; execution stops when the number of node isn trie exceeds this number; default is 100000.
//' @param regularization Optional double value, default is 0.01 which can be thought of as a penalty equivalent to misclassifying 1\% of the data when increasing the length of a rule list by one association rule.
//' @param logging_frequency Optional integer value with default of 1000.
//' @param ablation Integer value, default value is zero, one excludes the minimum support bounds (see Section 3.7), two excludes the lookahead bound (see Lemma 2 in Section 3.4).
//' @return A constant bool for now
//' @seealso The corels C++ implementation at https://github.com/nlarusstone/corels, the website at https://github.com/nlarusstone/corels and the Python implementation at https://github.com/fingoldin/pycorels.
//' @references Elaine Angelino, Nicholas Larus-Stone, Daniel Alabi, Margo Seltzer, and Cynthia Rudin. *Learning Certifiably Optimal Rule Lists for Categorical Data.* JMLR 2018, http://www.jmlr.org/papers/volume18/17-716/17-716.pdf
//' Nicholas Larus-Stone, Elaine Angelino, Daniel Alabi, Margo Seltzer, Vassilios Kaxiras, Aditya Saligrama, Cynthia Rudin. *Systems Optimizations for Learning Certifiably Optimal Rule Lists*. SysML 2018 http://www.sysml.cc/doc/2018/54.pdf
//' Nicholas Larus-Stone. *Learning Certifiably Optimal Rule Lists: A Case For Discrete Optimization in the 21st Century. Senior thesis 2017. https://dash.harvard.edu/handle/1/38811502.
//' Elaine Angelino, Nicholas Larus-Stone, Daniel Alabi, Margo Seltzer, Cynthia Rudin. *Learning certifiably optimal rule lists for categorical data*. KDD 2017, https://www.kdd.org/kdd2017/papers/view/learning-certifiably-optimal-rule-lists-for-categorical-data.
//' @examples
//' library(RcppCorels)
//'
//' logdir <- tempdir()
//' rules_file <- system.file("sample_data", "compas_train.out", package="RcppCorels")
//' labels_file <- system.file("sample_data", "compas_train.label", package="RcppCorels")
//' meta_file <- system.file("sample_data", "compas_train.minor", package="RcppCorels")
//'
//' stopifnot(file.exists(rules_file),
//'           file.exists(labels_file),
//'           file.exists(meta_file),
//'           dir.exists(logdir))
//'
//' corels(rules_file, labels_file, logdir, meta_file,
//'        verbosity_policy = "silent",
//'        regularization = 0.015,
//'        curiosity_policy = 2,   # by lower bound
//'        map_type = 1) 	   # permutation map
//' cat("See ", logdir, " for result file.")
// [[Rcpp::export]]
bool corels(std::string rules_file,
            std::string labels_file,
            std::string log_dir,
            std::string meta_file = "",
            bool run_bfs = false,
            bool calculate_size = false,
            bool run_curiosity = false,
            int curiosity_policy = 0,
            bool latex_out = false,
            int map_type = 0,
            std::string verbosity_policy = 0,
            int max_num_nodes = 100000,
            double regularization = 0.01,
            int logging_frequency = 1000,
            int ablation = 0) {

    //bool run_bfs = false;
    //bool run_curiosity = false;
    //int curiosity_policy = 0;
    //bool latex_out = false;
    bool use_prefix_perm_map = (map_type==1);
    bool use_captured_sym_map = (map_type==2);
    //int verbosity = 0;
    //int map_type = 0;
    //int max_num_nodes = 100000;
    double c = /*0.01*/ regularization;
    //char ch;
    //bool error = false;
    //char error_txt[BUFSZ];
    int freq = /*1000*/ logging_frequency;
    //int ablation = 0;
    //bool calculate_size = false;

    std::set<std::string> verbosity;
    char verbstr[64];
    verbstr[0] = '\0';
    if (!parse_verbosity(const_cast<char*>(verbosity_policy.c_str()), &verbstr[0],
                         sizeof(verbstr), &verbosity)) {
        Rcpp::stop("verbosity options must be one or more of (%s), separated with commas (i.e. -v progress,samples)", VERBSTR);
    }
    if (verbosity.count("samples") && !(verbosity.count("rule") || verbosity.count("label")
                                        || verbosity.count("minor") || verbosity.count("loud"))) {
        Rcpp::stop("verbosity 'samples' option must be combined with at least one of (rule|label|minor|samples|progress|loud|silent)");
    }
    if (verbosity.size() > 1 && verbosity.count("silent")) {
        Rcpp::stop("verbosity 'silent' option must be passed without any additional verbosity parameters");
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

    int nrules, nsamples, nlabels, nsamples_label;
    rule_t *rules, *labels;

    if (rules_init(rules_file.c_str(), &nrules, &nsamples, &rules, 1) != 0) {
        Rcpp::stop("Failed to load out file from path: %s\n", rules_file.c_str());
    }

    if (rules_init(labels_file.c_str(), &nlabels, &nsamples_label, &labels, 0) != 0) {
        rules_free(rules, nrules, 1);
        Rcpp::stop("Failed to load label file from path: %s\n", labels_file.c_str());
    }

    if (nlabels != 2) {
        rules_free(rules, nrules, 1);
        rules_free(labels, nlabels, 0);
        Rcpp::stop("nlabels must be equal to 2, got %d\n", nlabels);
    }

    if(nsamples_label != nsamples) {
        rules_free(rules, nrules, 1);
        rules_free(labels, nlabels, 0);
        Rcpp::stop("nsamples mismatch between out file (%d) and label file (%d)\n", nsamples, nsamples_label);
    }

    rules_init(rules_file.c_str(), &nrules, &nsamples, &rules, 1);
    rules_init(labels_file.c_str(), &nlabels, &nsamples_label, &labels, 0);



    int nmeta, nsamples_meta;
    // Equivalent points information is precomputed, read in from file, and stored in meta
    rule_t *meta;
    if (meta_file != "") {
        if (rules_init(meta_file.c_str(), &nmeta, &nsamples_meta, &meta, 0) != 0) {
            Rcpp::stop("Failed to load minor file from path: %s, skipping...", meta_file.c_str());
            meta = NULL;
            nmeta = 0;
        } else if(nsamples_meta != nsamples) {
            Rprintf("nsamples mismatch between out file (%d) and minor file (%d), skipping minor file...\n", nsamples, nsamples_meta);
            rules_free(meta, nmeta, 0);
            meta = NULL;
            nmeta = 0;
        }
    } else {
        meta = NULL;
    }

    char froot[BUFSZ+64];
    char log_fname[BUFSZ+4+64];
    char opt_fname[BUFSZ+8+64];
    const char* pch = strrchr(rules_file.c_str(), '/');
    snprintf(froot, BUFSZ+64, "%s/for-%s-%s%s-%s-%s-removed=%s-max_num_nodes=%d-c=%.7f-v=%s-f=%d",
             log_dir.c_str(),
             pch ? pch + 1 : "",
             run_bfs ? "bfs" : "",
             run_curiosity ? curiosity_map[curiosity_policy].c_str() : "",
             use_prefix_perm_map ? "with_prefix_perm_map" :
                (use_captured_sym_map ? "with_captured_symmetry_map" : "no_pmap"),
             meta ? "minor" : "no_minor",
             ablation ? ((ablation == 1) ? "support" : "lookahead") : "none",
             max_num_nodes, c, verbstr, freq);
    snprintf(log_fname, BUFSZ+4+64, "%s.txt", froot);
    snprintf(opt_fname, BUFSZ+8+64, "%s-opt.txt", froot);


    PermutationMap* pmap = NULL;
    CacheTree* tree = NULL;
    Queue* queue = NULL;
    double init = 0.0;
    std::set<std::string> run_verbosity;

    if (run_corels_begin(c, &verbstr[0], curiosity_policy, map_type, ablation, calculate_size,
                        nrules, nlabels, nsamples, rules, labels, meta, freq, &log_fname[0],
                        pmap, tree, queue, init, run_verbosity) == 0) {
        while (run_corels_loop(max_num_nodes, pmap, tree, queue) == 0) {
            // do nothing
    }

        std::vector<int> rulelist;
        std::vector<int> classes;

        run_corels_end(&rulelist, &classes, 0, latex_out, rules, labels, &opt_fname[0],
                       pmap, tree, queue, init, verbosity);

    } else {
        Rcpp::stop("Setup failed!");
    }

    if (meta) rules_free(meta, nmeta, 0);
    rules_free(rules, nrules, 1);
    rules_free(labels, nlabels, 0);


    return true;                  // more to fill in, naturally
}
