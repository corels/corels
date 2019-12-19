#include "utils.h"
#include <stdio.h>
#include <assert.h>
#include <sstream>

#if defined(R_BUILD)
 #define STRICT_R_HEADERS
 #include "R.h"
 // textual substitution
 #define printf Rprintf
#endif

std::string sizet_tostring(size_t v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

Logger::Logger(double c, size_t nrules, std::set<std::string> verbosity, char* log_fname, int freq) {
      _c = c;
      _nrules = nrules - 1;
      _v = verbosity;
      _freq = freq;
      setLogFileName(log_fname);
      initPrefixVec();
}

/*
 * Sets the logger file name and writes the header line to the file.
 */
void Logger::setLogFileName(char *fname) {
    if (!_v.size()) return;

    printf("writing logs to: %s\n\n", fname);
    _f.open(fname, ios::out | ios::trunc);

    _f << "total_time,evaluate_children_time,node_select_time,"
       << "rule_evaluation_time,lower_bound_time,lower_bound_num,"
       << "objective_time,objective_num,"
       << "tree_insertion_time,tree_insertion_num,queue_insertion_time,evaluate_children_num,"
       << "permutation_map_insertion_time,permutation_map_insertion_num,permutation_map_memory,"
       << "current_lower_bound,tree_min_objective,tree_prefix_length,"
       << "tree_num_nodes,tree_num_evaluated,tree_memory,"
       << "queue_size,queue_min_length,queue_memory,"
       << "pmap_size,pmap_null_num,pmap_discard_num,"
       << "log_remaining_space_size,prefix_lengths" << endl;
}

/*
 * Writes current stats about the execution to the log file.
 */
void Logger::dumpState() {
    // update timestamp here
    setTotalTime(time_diff(_state.initial_time));

    size_t space_size = 0;

#ifdef GMP
    space_size = getLogRemainingSpaceSize();
#endif

    _f << _state.total_time << ","
       << _state.evaluate_children_time << ","
       << _state.node_select_time << ","
       << _state.rule_evaluation_time << ","
       << _state.lower_bound_time << ","
       << _state.lower_bound_num << ","
       << _state.objective_time << ","
       << _state.objective_num << ","
       << _state.tree_insertion_time << ","
       << _state.tree_insertion_num << ","
       << _state.queue_insertion_time << ","
       << _state.evaluate_children_num << ","
       << _state.permutation_map_insertion_time << ","
       << _state.permutation_map_insertion_num << ","
       << _state.pmap_memory << ","
       << _state.current_lower_bound << ","
       << _state.tree_min_objective << ","
       << _state.tree_prefix_length << ","
       << _state.tree_num_nodes << ","
       << _state.tree_num_evaluated << ","
       << _state.tree_memory << ","
       << _state.queue_size << ","
       << _state.queue_min_length << ","
       << _state.queue_memory << ","
       << _state.pmap_size << ","
       << _state.pmap_null_num << ","
       << _state.pmap_discard_num << ","
       << space_size << ","
       << dumpPrefixLens().c_str() << endl;
}

#ifdef GMP
/*
 * Uses GMP library to dump a string version of the remaining state space size.
 * This number is typically very large (e.g. 10^20) which is why we use GMP instead of a long.
 * Note: this function may not work on some Linux machines.
 */
std::string Logger::dumpRemainingSpaceSize() {
    char* str = mpz_get_str(NULL, 10, _state.remaining_space_size);
    std::string ret(str);
    free(str);
    return ret;
}
#endif

/*
 * Function to convert vector of remaining prefix lengths to a string format for logging.
 */
std::string Logger::dumpPrefixLens() {
    std::string s = "";
    for(size_t i = 0; i < _nrules; ++i) {
        if (_state.prefix_lens[i] > 0) {
            s += sizet_tostring(i);
            s += ":";
            s += sizet_tostring(_state.prefix_lens[i]);
            s += ";";
        }
    }
    return s;
}

/*
 * Given a rulelist and predictions, will output a human-interpretable form to a file.
 */
void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[]) {
    assert(rulelist.size() == preds.size() - 1);

    printf("\nOPTIMAL RULE LIST\n");
    if (rulelist.size() > 0) {
        printf("if (%s) then (%s)\n", rules[rulelist[0]].features,
               labels[preds[0]].features);
        for (size_t i = 1; i < rulelist.size(); ++i) {
            printf("else if (%s) then (%s)\n", rules[rulelist[i]].features,
                   labels[preds[i]].features);
        }
        printf("else (%s)\n\n", labels[preds.back()].features);

        if (latex_out) {
            printf("\nLATEX form of OPTIMAL RULE LIST\n");
            printf("\\begin{algorithmic}\n");
            printf("\\normalsize\n");
            printf("\\State\\bif (%s) \\bthen (%s)\n", rules[rulelist[0]].features,
                   labels[preds[0]].features);
            for (size_t i = 1; i < rulelist.size(); ++i) {
                printf("\\State\\belif (%s) \\bthen (%s)\n", rules[rulelist[i]].features,
                       labels[preds[i]].features);
            }
            printf("\\State\\belse (%s)\n", labels[preds.back()].features);
            printf("\\end{algorithmic}\n\n");
        }
    } else {
        printf("if (1) then (%s)\n\n", labels[preds.back()].features);

        if (latex_out) {
            printf("\nLATEX form of OPTIMAL RULE LIST\n");
            printf("\\begin{algorithmic}\n");
            printf("\\normalsize\n");
            printf("\\State\\bif (1) \\bthen (%s)\n", labels[preds.back()].features);
            printf("\\end{algorithmic}\n\n");
        }
    }

    ofstream f;
    printf("writing optimal rule list to: %s\n\n", fname);
    f.open(fname, ios::out | ios::trunc);
    for(size_t i = 0; i < rulelist.size(); ++i) {
        f << rules[rulelist[i]].features << "~"
          << preds[i] << ";";
    }
    f << "default~" << preds.back();
    f.close();
}

// Returns true on success
bool parse_verbosity(char* str, char* verbstr, size_t verbstr_size, std::set<std::string>* verbosity) {
    char *vopt, *verb_trim;
    const char *vstr = VERBSTR;

    verb_trim = strtok(str, " ");
    strncpy(verbstr, verb_trim, verbstr_size);
    vopt = strtok(verb_trim, ",");
    while (vopt != NULL) {
        int voptlen = strlen(vopt);
        const char* opt = strstr(vstr, vopt);
        // Check if opt is actually a valid option rather than just contained in vstr
        if(voptlen && opt && (opt == vstr || *(opt - 1) == '|') &&
          (*(opt + voptlen) == '\0' || *(opt + voptlen) == '|')) {
          verbosity->insert(vopt);
          vopt = strtok(NULL, ",");
        } else {
          verbosity->clear();
          return false;
        }
    }

    return true;
}
