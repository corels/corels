#include "params.h"

void set_default_params(run_params_t* out)
{
    out->opt_fname = (char*)D_OPT_FNAME;
    out->log_fname = (char*)D_LOG_FNAME;
    out->max_num_nodes = D_MAX_NUM_NODES;
    out->c = D_C;
    out->vstring = (char*)D_VSTRING;
    out->curiosity_policy = D_CURIOSITY_POLICY;
    out->map_type = D_MAP_TYPE;
    out->freq = D_FREQ;
    out->ablation = D_ABLATION;
    out->calculate_size = D_CALCULATE_SIZE;
    out->latex_out = D_LATEX_OUT;

    out->nrules = 0;
    out->nlabels = 0;
    out->nsamples = 0;
    out->rules = NULL;
    out->labels = NULL;
    out->meta = NULL;
}
