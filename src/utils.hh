#pragma once

#include <cstdlib>
#include <string.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <set>
#include <chrono>

#include "rule.hh"



using namespace std;

enum class DataStruct { Tree, Queue, Pmap};

class NullLogger {
  public:
    virtual void closeFile() {}
    NullLogger() {}
    NullLogger(double c, size_t nrules, std::set<std::string> verbosity, char* log_fname, int freq) {}
    virtual ~NullLogger() {}

    virtual void setLogFileName(char *fname) {}
    virtual void dumpState() {}
    virtual std::string dumpPrefixLens() { return ""; }

    virtual inline void setVerbosity(int verbosity) {}
    virtual inline int getVerbosity() { return 0; }
    virtual inline void setFrequency(int frequency) {}
    virtual inline int getFrequency() { return 1000; }
    virtual inline void addToLowerBoundTime(double t) {}
    virtual inline void incLowerBoundNum() {}
    virtual inline void addToObjTime(double t) {}
    virtual inline void incObjNum() {}
    virtual inline void addToTreeInsertionTime(double t) {}
    virtual inline void incTreeInsertionNum() {}
    virtual inline void addToRuleEvalTime(double t) {}
    virtual inline void incRuleEvalNum() {}
    virtual inline void addToNodeSelectTime(double t) {}
    virtual inline void incNodeSelectNum() {}
    virtual inline void addToEvalChildrenTime(double t) {}
    virtual inline void incEvalChildrenNum() {}
    virtual inline void setInitialTime(double t) {}
    virtual inline double getInitialTime() { return 0.0; }
    virtual inline void setTotalTime(double t) {}
    virtual inline void addToPermMapInsertionTime(double t) {}
    virtual inline void incPermMapInsertionNum() {}
    virtual inline void setCurrentLowerBound(double lb) {}
    virtual inline void setTreeMinObj(double o) {}
    virtual inline void setTreePrefixLen(size_t n) {}
    virtual inline void setTreeNumNodes(size_t n) {}
    virtual inline void setTreeNumEvaluated(size_t n) {}
    virtual inline size_t getTreeMemory() { return 0; }
    virtual inline void addToQueueInsertionTime(double t) {}
    virtual inline void setQueueSize(size_t n) {}
    virtual inline size_t getQueueMemory() { return 0; }
    virtual inline void setNRules(size_t nrules) {}
    virtual inline void setC(double c) {}
    virtual inline void initPrefixVec() {}
    virtual inline void incPrefixLen(size_t n) {}
    virtual inline void decPrefixLen(size_t n) {}
    virtual inline size_t sumPrefixLens() { return 0; }
    virtual inline void updateQueueMinLen() {}
    virtual inline size_t getQueueMinLen() { return 0; }
    virtual inline void incPmapSize() {}
    virtual inline void decreasePmapSize(size_t n) {}
    virtual inline void incPmapNullNum() {}
    virtual inline void incPmapDiscardNum() {}
    virtual inline size_t getPmapMemory() { return 0; }
    virtual inline void addToMemory(size_t n, DataStruct s) {}
    virtual inline void removeFromMemory(size_t n, DataStruct s) {}
    virtual inline void addQueueElement(unsigned int len_prefix, double lower_bound, bool approx) {}
    virtual inline void removeQueueElement(unsigned int len_prefix, double lower_bound, bool approx) {}
    /*
     * Initializes the logger by setting all fields to 0.
     * This allows us to write a log record immediately.
     */
    void initializeState(bool calculate_size) {
        _state.total_time = 0.;
        _state.evaluate_children_time = 0.;
        _state.evaluate_children_num = 0;
        _state.node_select_time = 0.;
        _state.node_select_num = 0;
        _state.rule_evaluation_time = 0.;
        _state.rule_evaluation_num = 0;
        _state.lower_bound_time = 0.;
        _state.lower_bound_num = 0;
        _state.objective_time = 0.;
        _state.objective_num = 0;
        _state.tree_insertion_time = 0.;
        _state.tree_insertion_num = 0;
        _state.permutation_map_insertion_time = 0.;
        _state.permutation_map_insertion_num = 0;
        _state.current_lower_bound = 0.;
        _state.tree_min_objective = 1.;
        _state.tree_prefix_length = 0;
        _state.tree_num_nodes = 0;
        _state.tree_num_evaluated = 0;
        _state.tree_memory = 0;
        _state.queue_insertion_time = 0;
        _state.queue_size = 0;
        _state.queue_min_length = 0;
        _state.queue_memory = 0;
        _state.pmap_size = 0;
        _state.pmap_memory = 0;
        _state.pmap_null_num = 0;
        _state.pmap_discard_num = 0;
    }


  protected:
    struct State {
        double initial_time;                    // initial time stamp
        double total_time;
        double evaluate_children_time;
        size_t evaluate_children_num;
        double node_select_time;
        size_t node_select_num;
        double rule_evaluation_time;
        size_t rule_evaluation_num;
        double lower_bound_time;
        size_t lower_bound_num;
        double objective_time;
        size_t objective_num;
        double tree_insertion_time;
        size_t tree_insertion_num;
        double permutation_map_insertion_time;
        size_t permutation_map_insertion_num;   // number of calls to `permutation_insert` function
        double current_lower_bound;             // monotonically decreases for curious lower bound
        double tree_min_objective;
        size_t tree_prefix_length;
        size_t tree_num_nodes;
        size_t tree_num_evaluated;
        size_t tree_memory;
        double queue_insertion_time;
        size_t queue_size;
        size_t queue_min_length;                // monotonically increases
        size_t queue_memory;
        size_t pmap_size;                       // size of pmap
        size_t pmap_null_num;                   // number of pmap lookups that return null
        size_t pmap_discard_num;                // number of pmap lookups that trigger discard
        size_t pmap_memory;
        size_t* prefix_lens;
    };
    double _c;
    size_t _nrules;
    State _state;
    int _v;                                     // verbosity
    int _freq;                                  // frequency of logging
    ofstream _f;                                // output file
};

class PyLogger : public NullLogger {
    inline void setVerbosity(int verbosity) override {
        _v = verbosity;
    }
    inline int getVerbosity() { return _v; }
};

class Logger : public NullLogger {
  public:
    void closeFile() override { if (_f.is_open()) _f.close(); }
    Logger(double c, size_t nrules, int verbosity, char* log_fname, int freq);
    ~Logger() {
        free(_state.prefix_lens);
        closeFile();
    }

    void setLogFileName(char *fname) override;
    void dumpState() override;
    std::string dumpPrefixLens() override;

    inline void setVerbosity(int verbosity) override {
        _v = verbosity;
    }
    inline int getVerbosity() override {
        return _v;
    }
    inline void setFrequency(int frequency) override {
        _freq = frequency;
    }
    inline int getFrequency() override {
        return _freq;
    }
    inline void addToLowerBoundTime(double t) override {
        _state.lower_bound_time += t;
    }
    inline void incLowerBoundNum() override {
        ++_state.lower_bound_num;
    }
    inline void addToObjTime(double t) override {
        _state.objective_time += t;
    }
    inline void incObjNum() override {
        ++_state.objective_num;
    }
    inline void addToTreeInsertionTime(double t) override {
        _state.tree_insertion_time += t;
    }
    inline void incTreeInsertionNum() override {
        ++_state.tree_insertion_num;
    }
    inline void addToRuleEvalTime(double t) override {
        _state.rule_evaluation_time += t;
    }
    inline void incRuleEvalNum() override {
        ++_state.rule_evaluation_num;
    }
    inline void addToNodeSelectTime(double t) override {
        _state.node_select_time += t;
    }
    inline void incNodeSelectNum() override {
        ++_state.node_select_num;
    }
    inline void addToEvalChildrenTime(double t) override {
        _state.evaluate_children_time += t;
    }
    inline void incEvalChildrenNum() override {
        ++_state.evaluate_children_num;
    }
    inline void setInitialTime(double t) override {
        _state.initial_time = t;
    }
    inline double getInitialTime() override {
        return _state.initial_time;
    }
    inline void setTotalTime(double t) override {
        _state.total_time = t;
    }
    inline void addToPermMapInsertionTime(double t) override {
        _state.permutation_map_insertion_time += t;
    }
    inline void incPermMapInsertionNum() override {
        ++_state.permutation_map_insertion_num;
    }
    inline void setCurrentLowerBound(double lb) override {
        _state.current_lower_bound = lb;
    }
    inline void setTreeMinObj(double o) override {
        _state.tree_min_objective = o;
    }
    inline void setTreePrefixLen(size_t n) override {
        _state.tree_prefix_length = n;
    }
    inline void setTreeNumNodes(size_t n) override {
        _state.tree_num_nodes = n;
    }
    inline void setTreeNumEvaluated(size_t n) override {
        _state.tree_num_evaluated = n;
    }
    inline size_t getTreeMemory() override {
        return _state.tree_memory;
    }
    inline void addToQueueInsertionTime(double t) override {
        _state.queue_insertion_time += t;
    }
    inline void setQueueSize(size_t n) override {
        _state.queue_size = n;
    }
    inline size_t getQueueMemory() override {
        return _state.queue_memory;
    }
    inline void initPrefixVec() override {
        _state.prefix_lens = (size_t*) calloc(_nrules, sizeof(size_t));
    }
    inline void incPrefixLen(size_t n) override {
        ++_state.prefix_lens[n];
        if (_state.prefix_lens[n] == 1)
            updateQueueMinLen();
    }
    inline void decPrefixLen(size_t n) override {
        --_state.prefix_lens[n];
        if (_state.prefix_lens[n] == 0)
            updateQueueMinLen();
    }
    /*
     * Returns the size of the logical queue.
     */
    inline size_t sumPrefixLens() override {
        size_t tot = 0;
        for(size_t i = 0; i < _nrules; ++i) {
            tot += _state.prefix_lens[i];
        }
        return tot;
    }
    inline void updateQueueMinLen() override {
        // Note: min length is logically undefined when queue size is 0
        size_t min_length = 0;
        for(size_t i = 0; i < _nrules; ++i) {
            if (_state.prefix_lens[i] > 0) {
                min_length = i;
                break;
            }
        }
        _state.queue_min_length = min_length;
    }
    inline size_t getQueueMinLen() override {
        return _state.queue_min_length;
    }
    inline void incPmapSize() override {
        ++_state.pmap_size;
    }
    inline void decreasePmapSize(size_t n) override {
        _state.pmap_size -= n;
    }
    inline void incPmapNullNum() override {
        ++_state.pmap_null_num;
    }
    inline void incPmapDiscardNum() override {
        ++_state.pmap_discard_num;
    }
    inline size_t getPmapMemory() override {
        return _state.pmap_memory;
    }
    inline void addToMemory(size_t n, DataStruct data_struct) override{
        if (data_struct == DataStruct::Tree)
            _state.tree_memory += n;
        else if (data_struct == DataStruct::Queue)
            _state.queue_memory += n;
        else if (data_struct == DataStruct::Pmap)
            _state.pmap_memory += n;
    }
    inline void removeFromMemory(size_t n, DataStruct data_struct) override{
        if (data_struct == DataStruct::Tree)
            _state.tree_memory -= n;
        else if (data_struct == DataStruct::Queue)
            _state.queue_memory -= n;
        else if (data_struct == DataStruct::Pmap)
            _state.pmap_memory -= n;
    }
};

extern NullLogger* logger;

inline double timestamp() {
    std::chrono::duration<double> duration = std::chrono::steady_clock::now().time_since_epoch();

    return duration.count();
}

inline double time_diff(double t0) {
    return timestamp() - t0;
}

#include "alloc.hh"
/*
 * Prints the final rulelist that CORELS returns.
 * rulelist -- rule ids of optimal rulelist
 * preds -- corresponding predictions of rules (+ default prediction)
 */
void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[]);
