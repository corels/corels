#include <Python.h>
#include <string.h>

#define NPY_NO_DEPRECATED_API NPY_API_VERSION

#include <numpy/arrayobject.h>

#include "../src/run.hh"

#include "../src/rule.h"

#include "utils.h"

static PyObject *pycorels_tofile(PyObject *self, PyObject *args)
{
    PyObject *list;
    const char *fname;

    if(!PyArg_ParseTuple(args, "Os", &list, &fname))
        return NULL;

    if(!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "Argument must be a list");
        return NULL;
    }

    PyObject *tuple, *vector;
    char *features;

    npy_intp list_len = PyList_Size(list);

    FILE *fp;
    if(!(fp = fopen(fname, "w")))
        return NULL;

    for(Py_ssize_t i = 0; i < list_len; i++) {
        if(!(tuple = PyList_GetItem(list, i)))
            goto error;

        if(!PyTuple_Check(tuple)) {
            PyErr_SetString(PyExc_TypeError, "Array members must be tuples");
            goto error;
        }

        if(!PyArg_ParseTuple(tuple, "sO", &features, &vector))
            goto error;

        fprintf(fp, "%s ", features);

        int type = PyArray_TYPE((PyArrayObject*)vector);
        if(PyArray_NDIM((PyArrayObject*)vector) != 1 && (PyTypeNum_ISINTEGER(type) || PyTypeNum_ISBOOL(type))) {
            PyErr_SetString(PyExc_TypeError, "Each rule truthable must be a 1-d array of integers or booleans");
            goto error;
        }

        PyArrayObject *clean = (PyArrayObject*)PyArray_FromAny(vector, PyArray_DescrFromType(NPY_BYTE), 0, 0, NPY_ARRAY_CARRAY | NPY_ARRAY_ENSURECOPY | NPY_ARRAY_FORCECAST, NULL);
        if(clean == NULL) {
            PyErr_SetString(PyExc_Exception, "Could not cast array to byte carray");
            goto error;
        }

        char *data = PyArray_BYTES(clean);
        npy_intp b_len = PyArray_SIZE(clean);

        for(npy_intp j = 0; j < b_len-1; j++) {
            fprintf(fp, "%d ", !!data[j]);
        }

        fprintf(fp, "%d\n", !!data[b_len-1]);

        Py_DECREF(clean);
    }

    fclose(fp);

    Py_INCREF(Py_None);
    return Py_None;

error:
    fclose(fp);
    return NULL;
}

static PyObject *pycorels_tolist(PyObject *self, PyObject *args)
{
    const char *fname;

    if(!PyArg_ParseTuple(args, "s", &fname))
        return NULL;

    rule_t *rules;
    int nrules, nsamples;

    if(rules_init(fname, &nrules, &nsamples, &rules, 0) != 0) {
        PyErr_SetString(PyExc_ValueError, "Could not load rule file");
        return NULL;
    }

    PyObject *list = PyList_New(nrules);

    PyObject *res = fill_list(list, rules, 0, nrules, nsamples);
    if(!res) {
        Py_XDECREF(list);
        list = NULL;
    }

    rules_free(rules, nrules, 0);

    return list;
}

static PyObject* pycorels_run(PyObject* self, PyObject* args, PyObject* keywds)
{
    PyObject* out_data;
    char* out_fname = NULL;

    PyObject* label_data;
    char* label_fname = NULL;

    PyObject* minor_data = NULL;
    char* minor_fname = NULL;

    run_params_t params;
    set_default_params(&params);

    static char* kwlist[] = {"out_data", "label_data", "minor_data", "opt_file", "log_file", "curiosity_policy", "latex_out", "map_type",
                             "verbosity", "log_freq", "max_num_nodes", "c", "ablation", "calculate_size"};

    if(!PyArg_ParseTupleAndKeywords(args, keywds, "OO|Ossibisiidibb", kwlist, &out_data, &label_data, &minor_data,
                                    &params.opt_fname, &params.log_fname, &params.curiosity_policy, &params.latex_out, &params.map_type, &params.vstring,
                                    &params.freq, &params.max_num_nodes, &params.c, &params.ablation, &params.calculate_size))
    {
        return NULL;
    }

    char error_txt[BUFSZ];
    PyObject* error_type = PyExc_ValueError;

    if(PyBytes_Check(out_data)) {
        if(!(out_fname = strdup(PyBytes_AsString(out_data))))
            return NULL;
    }
    else if(PyUnicode_Check(out_data)) {
        PyObject* bytes = PyUnicode_AsUTF8String(out_data);
        if(!bytes)
            return NULL;

        else if(!(out_fname = strdup(PyBytes_AsString(bytes))))
            return NULL;

        Py_DECREF(bytes);
    }
    else if(PyList_Check(out_data)) {
        if(!PyList_Size(out_data)) {
            snprintf(error_txt, BUFSZ, "out list must be non-empty");
            goto error;
        }
    }
    else {
        snprintf(error_txt, BUFSZ, "out data must be either a python list or a file path");
        error_type = PyExc_TypeError;
        goto error;
    }

    if(PyBytes_Check(label_data)) {
        if(!(label_fname = strdup(PyBytes_AsString(label_data))))
            return NULL;
    }
    else if(PyUnicode_Check(label_data)) {
        PyObject* bytes = PyUnicode_AsUTF8String(label_data);
        if(!bytes)
            return NULL;

        else if(!(label_fname = strdup(PyBytes_AsString(bytes))))
            return NULL;

        Py_DECREF(bytes);
    }
    else if(PyList_Check(label_data)) {
        if(!PyList_Size(label_data)) {
            snprintf(error_txt, BUFSZ, "label list must be non-empty");
            goto error;
        }
    }
    else {
        snprintf(error_txt, BUFSZ, "label data must be either a python list or a file path");
        error_type = PyExc_TypeError;
        goto error;
    }

    if(minor_data) {
        if(PyBytes_Check(minor_data)) {
            if(!(minor_fname = strdup(PyBytes_AsString(minor_data))))
                return NULL;
        }
        else if(PyUnicode_Check(minor_data)) {
            PyObject* bytes = PyUnicode_AsUTF8String(minor_data);
            if(!bytes)
                return NULL;

            else if(!(minor_fname = strdup(PyBytes_AsString(bytes))))
                return NULL;

            Py_DECREF(bytes);
        }
        else if(PyList_Check(minor_data)) {
            if(!PyList_Size(minor_data)) {
                snprintf(error_txt, BUFSZ, "minor list must be non-empty");
                goto error;
            }
        }
        else {
            snprintf(error_txt, BUFSZ, "minor data must be either a python list or a file path");
            error_type = PyExc_TypeError;
            goto error;
        }
    }

    if(!params.opt_fname || !strlen(params.opt_fname)) {
        snprintf(error_txt, BUFSZ, "optimal rulelist file must be a valid file path");
        goto error;
    }
    if(!params.log_fname || !strlen(params.log_fname)) {
        snprintf(error_txt, BUFSZ, "log file must be a valid file path");
        goto error;
    }
    if (params.max_num_nodes < 0) {
        snprintf(error_txt, BUFSZ, "maximum number of nodes must be positive");
        goto error;
    }
    if (params.c < 0.0) {
        snprintf(error_txt, BUFSZ, "regularization constant must be postitive");
        goto error;
    }
    if (params.map_type > 2 || params.map_type < 0) {
        snprintf(error_txt, BUFSZ, "symmetry-aware map must be (0|1|2)");
        goto error;
    }
    if (params.curiosity_policy < 0 || params.curiosity_policy > 4) {
        snprintf(error_txt, BUFSZ, "you must specify a curiosity type (0|1|2|3|4)");
        goto error;
    }

    //printf("out_fname: %s\n", out_fname);

    if(out_fname) {
        if(rules_init(out_fname, &params.nrules, &params.nsamples, &params.rules, 1) != 0) {
            snprintf(error_txt, BUFSZ, "could not load out file at path '%s'", out_fname);
            goto error;
        }

        free(out_fname);
    } else {
        if(load_list(out_data, &params.nrules, &params.nsamples, &params.rules, 1) != 0)
            return NULL;
    }

    int nsamples_chk;
    if(label_fname) {
        if(rules_init(label_fname, &params.nlabels, &nsamples_chk, &params.labels, 0) != 0) {
            rules_free(params.rules, params.nrules, 1);
            snprintf(error_txt, BUFSZ, "could not load label file at path '%s'", label_fname);
            goto error;
        }

        free(label_fname);
    } else {
        if(load_list(label_data, &params.nlabels, &nsamples_chk, &params.labels, 0) != 0) {
            rules_free(params.rules, params.nrules, 1);
            return NULL;
        }
    }

    int nmeta, nsamples_check;

    params.meta = NULL;
    if (minor_data) {
        if(minor_fname) {
            if(rules_init(minor_fname, &nmeta, &nsamples_check, &params.meta, 0) != 0) {
                rules_free(params.rules, params.nrules, 1);
                rules_free(params.labels, params.nlabels, 0);
                snprintf(error_txt, BUFSZ, "could not load minority file at path '%s'", minor_fname);
                goto error;
            }

            free(minor_fname);
        } else {
            if(load_list(minor_data, &nmeta, &nsamples_check, &params.meta, 0) != 0) {
                rules_free(params.rules, params.nrules, 1);
                rules_free(params.labels, params.nlabels, 0);
                return NULL;
            }
        }
    }

    if(params.nlabels != 2) {
        rules_free(params.rules, params.nrules, 1);
        rules_free(params.labels, params.nlabels, 0);
        rules_free(params.meta, nmeta, 0);

        snprintf(error_txt, BUFSZ, "the number of labels in the label file must be 2, not %d", params.nlabels);
        goto error;
    }

    double accuracy = run_corels(params);

    rules_free(params.rules, params.nrules, 1);
    rules_free(params.labels, params.nlabels, 0);

    if(params.meta)
        rules_free(params.meta, nmeta, 0);

    return Py_BuildValue("d", accuracy);

error:

    PyErr_SetString(error_type, error_txt);

    return NULL;
}

static PyMethodDef pycorelsMethods[] = {
    {"run", (PyCFunction)pycorels_run, METH_VARARGS | METH_KEYWORDS },
    {"tolist", (PyCFunction)pycorels_tolist, METH_VARARGS },
    {"tofile", (PyCFunction)pycorels_tofile, METH_VARARGS },
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION > 2

static struct PyModuleDef pycorelsModule = {
    PyModuleDef_HEAD_INIT,
    "pycorels",
    "Python binding to CORELS algorithm",
    -1,
    pycorelsMethods
};

PyMODINIT_FUNC PyInit_pycorels(void)
{
    import_array();

    return PyModule_Create(&pycorelsModule);
}

#else

PyMODINIT_FUNC initpycorels(void)
{
    import_array();

    Py_InitModule("pycorels", pycorelsMethods);
}

#endif
