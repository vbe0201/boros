/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/mkdir.h"

#include "util/python.h"

#include "module.h"

static void mkdirat_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    MkdirAtOperation *op = (MkdirAtOperation *)self;

    const char *pathname = PyBytes_AS_STRING(op->path);
    io_uring_prep_mkdirat(sqe, op->dfd, pathname, op->base.scratch);
}

static void mkdirat_complete(PyObject *self, struct io_uring_cqe *cqe) {
    MkdirAtOperation *op = (MkdirAtOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
    } else {
        assert(cqe->res == 0);
        outcome_capture(&(op->base.outcome), Py_None);
    }
}

static OperationVTable g_mkdirat_operation_vtable = {
    .prepare  = mkdirat_prepare,
    .complete = mkdirat_complete,
};

PyObject *mkdirat_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "Expected 3 argument, got %zu instead", nargs);
        return NULL;
    }

    int dfd;
    if (args[0] == Py_None) {
        dfd = AT_FDCWD;
    } else if (!python_parse_int(&dfd, args[0])) {
        return NULL;
    }

    PyObject *path;
    if (!PyUnicode_FSConverter(args[1], &path)) {
        return NULL;
    }

    unsigned int mode = 0;
    if (!python_parse_unsigned_int(&mode, args[2])) {
        return NULL;
    }

    MkdirAtOperation *op = (MkdirAtOperation *)operation_alloc(state->MkdirAtOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_mkdirat_operation_vtable;
        op->path         = path;
        op->dfd          = dfd;
        op->base.scratch = mode;
    } else {
        Py_DECREF(path);
    }

    return (PyObject *)op;
}

static int mkdirat_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    MkdirAtOperation *op = (MkdirAtOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->path);
    return operation_traverse(&op->base, visit, arg);
}

static int mkdirat_clear_impl(PyObject *self) {
    MkdirAtOperation *op = (MkdirAtOperation *)self;

    Py_CLEAR(op->path);
    return operation_clear(&op->base);
}

static PyType_Slot g_mkdir_operation_slots[] = {
    {Py_tp_traverse, mkdirat_traverse_impl},
    {Py_tp_clear, mkdirat_clear_impl},
    {0, NULL},
};

static PyType_Spec g_mkdirat_operation_spec = {
    .name      = "_impl._MkdirAtOperation",
    .basicsize = sizeof(MkdirAtOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_mkdir_operation_slots,
};

PyTypeObject *mkdirat_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_mkdirat_operation_spec, (PyObject *)state->Operation_type);
}
