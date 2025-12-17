/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "context/run_config.h"

#include <stddef.h>

#include "util/python.h"

PyDoc_STRVAR(g_run_config_doc, "Configuration for the runtime context.\n\n"
                               "These settings are used to create the subsystems and must be passed to\n"
                               "every runner invocation.");

PyDoc_STRVAR(g_run_config_sq_size_doc, "The capacity of the io_uring submission queue.");
PyDoc_STRVAR(g_run_config_cq_size_doc, "The capacity of the io_uring completion queue.");
PyDoc_STRVAR(g_run_config_wqfd_doc, "The fd of an existing io_uring instance whose work queue should be shared.");

static int run_config_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int run_config_clear(PyObject *self) {
    (void)self;
    return 0;
}

static int run_config_init(PyObject *self, PyObject *args, PyObject *kwds) {
    (void)args;
    (void)kwds;

    RunConfig *conf = (RunConfig *)self;
    conf->sq_size   = 0;
    conf->cq_size   = 0;
    conf->wqfd      = -1;

    return 0;
}

static PyMemberDef g_run_config_members[] = {
    {"sq_size", Py_T_UINT, offsetof(RunConfig, sq_size), 0, g_run_config_sq_size_doc},
    {"cq_size", Py_T_UINT, offsetof(RunConfig, cq_size), 0, g_run_config_cq_size_doc},
    {"wqfd", Py_T_INT, offsetof(RunConfig, wqfd), 0, g_run_config_wqfd_doc},
    {NULL, 0, 0, 0, NULL},
};

// clang-format off
static PyType_Slot g_run_config_slots[] = {
    {Py_tp_doc, (void *)g_run_config_doc},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_dealloc, python_tp_dealloc},
    {Py_tp_init, run_config_init},
    {Py_tp_traverse, run_config_traverse},
    {Py_tp_clear, run_config_clear},
    {Py_tp_members, g_run_config_members},
    {0, NULL},
};
// clang-format on

static PyType_Spec g_run_config_spec = {
    .name      = "_impl.RunConfig",
    .basicsize = sizeof(RunConfig),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    .slots     = g_run_config_slots,
};

PyTypeObject *run_config_register(PyObject *mod) {
    PyTypeObject *tp = (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_run_config_spec, NULL);
    if (tp == NULL) {
        return NULL;
    }

    if (PyModule_AddType(mod, tp) < 0) {
        return NULL;
    }

    return tp;
}
