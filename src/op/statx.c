/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/statx.h"

#include "util/python.h"

#include <fcntl.h>
#include <stddef.h>

#include "module.h"

/* StatxResult implementation */

static int statx_result_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static int statx_result_clear(PyObject *self) {
    (void)self;
    return 0;
}

static PyMemberDef g_statx_result_members[] = {
    {"atime", Py_T_LONGLONG, offsetof(StatxResult, atime), Py_READONLY, NULL},
    {"atime_nsec", Py_T_UINT, offsetof(StatxResult, atime_nsec), Py_READONLY, NULL},
    {"blksize", Py_T_UINT, offsetof(StatxResult, blksize), Py_READONLY, NULL},
    {"blocks", Py_T_ULONGLONG, offsetof(StatxResult, blocks), Py_READONLY, NULL},
    {"ctime", Py_T_LONGLONG, offsetof(StatxResult, ctime), Py_READONLY, NULL},
    {"ctime_nsec", Py_T_UINT, offsetof(StatxResult, ctime_nsec), Py_READONLY, NULL},
    {"dev_major", Py_T_ULONGLONG, offsetof(StatxResult, dev_major), Py_READONLY, NULL},
    {"dev_minor", Py_T_ULONGLONG, offsetof(StatxResult, dev_minor), Py_READONLY, NULL},
    {"gid", Py_T_UINT, offsetof(StatxResult, gid), Py_READONLY, NULL},
    {"ino", Py_T_ULONGLONG, offsetof(StatxResult, ino), Py_READONLY, NULL},
    {"mode", Py_T_USHORT, offsetof(StatxResult, mode), Py_READONLY, NULL},
    {"mtime", Py_T_LONGLONG, offsetof(StatxResult, mtime), Py_READONLY, NULL},
    {"mtime_nsec", Py_T_UINT, offsetof(StatxResult, mtime_nsec), Py_READONLY, NULL},
    {"nlink", Py_T_UINT, offsetof(StatxResult, nlink), Py_READONLY, NULL},
    {"rdev_major", Py_T_ULONGLONG, offsetof(StatxResult, rdev_major), Py_READONLY, NULL},
    {"rdev_minor", Py_T_ULONGLONG, offsetof(StatxResult, rdev_minor), Py_READONLY, NULL},
    {"size", Py_T_ULONGLONG, offsetof(StatxResult, size), Py_READONLY, NULL},
    {"uid", Py_T_UINT, offsetof(StatxResult, uid), Py_READONLY, NULL},
    {NULL, 0, 0, 0, NULL},
};

static PyType_Slot g_statx_result_slots[] = {
    {Py_tp_dealloc, python_tp_dealloc},
    {Py_tp_traverse, statx_result_traverse},
    {Py_tp_clear, statx_result_clear},
    {Py_tp_members, g_statx_result_members},
    {0, NULL},
};

static PyType_Spec g_statx_result_spec = {
    .name      = "_impl.StatxResult",
    .basicsize = sizeof(StatxResult),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_statx_result_slots,
};

PyTypeObject *statx_result_register(PyObject *mod) {
    PyTypeObject *tp = (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_statx_result_spec, NULL);
    if (tp == NULL) {
        return NULL;
    }

    if (PyModule_AddType(mod, tp) < 0) {
        return NULL;
    }

    return tp;
}

/* StatxOperation implementation */

static void statx_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    StatxOperation *op = (StatxOperation *)self;

    const char *pathname = PyBytes_AS_STRING(op->path);
    io_uring_prep_statx(sqe, op->dfd, pathname, op->base.scratch, op->mask, &op->stx);
}

static void statx_complete(PyObject *self, struct io_uring_cqe *cqe) {
    StatxOperation *op = (StatxOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&(op->base.outcome));
        return;
    }

    ImplState *state = op->base.module_state;
    StatxResult *res = (StatxResult *)python_alloc(state->StatxResult_type);
    if (res == NULL) {
        outcome_capture_error(&(op->base.outcome));
        return;
    }

    res->atime      = op->stx.stx_atime.tv_sec;
    res->atime_nsec = op->stx.stx_atime.tv_nsec;
    res->blksize    = op->stx.stx_blksize;
    res->blocks     = op->stx.stx_blocks;
    res->ctime      = op->stx.stx_ctime.tv_sec;
    res->ctime_nsec = op->stx.stx_ctime.tv_nsec;
    res->dev_major  = op->stx.stx_dev_major;
    res->dev_minor  = op->stx.stx_dev_minor;
    res->gid        = op->stx.stx_gid;
    res->ino        = op->stx.stx_ino;
    res->mode       = op->stx.stx_mode;
    res->mtime      = op->stx.stx_mtime.tv_sec;
    res->mtime_nsec = op->stx.stx_mtime.tv_nsec;
    res->nlink      = op->stx.stx_nlink;
    res->rdev_major = op->stx.stx_rdev_major;
    res->rdev_minor = op->stx.stx_rdev_minor;
    res->size       = op->stx.stx_size;
    res->uid        = op->stx.stx_uid;

    outcome_capture(&(op->base.outcome), (PyObject *)res);
}

static OperationVTable g_statx_operation_vtable = {
    .prepare  = statx_prepare,
    .complete = statx_complete,
};

PyObject *statx_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 4) {
        PyErr_Format(PyExc_TypeError, "Expected 4 arguments, got %zu instead", nargs);
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

    int flags;
    if (!python_parse_int(&flags, args[2])) {
        Py_DECREF(path);
        return NULL;
    }

    unsigned int mask;
    if (!python_parse_unsigned_int(&mask, args[3])) {
        Py_DECREF(path);
        return NULL;
    }

    StatxOperation *op = (StatxOperation *)operation_alloc(state->StatxOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_statx_operation_vtable;
        op->base.scratch = flags;
        op->path         = path;
        op->dfd          = dfd;
        op->mask         = mask;
    } else {
        Py_DECREF(path);
    }

    return (PyObject *)op;
}

static int statx_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    StatxOperation *op = (StatxOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->path);
    return operation_traverse(&op->base, visit, arg);
}

static int statx_clear_impl(PyObject *self) {
    StatxOperation *op = (StatxOperation *)self;

    Py_CLEAR(op->path);
    return operation_clear(&op->base);
}

static PyType_Slot g_statx_operation_slots[] = {
    {Py_tp_traverse, statx_traverse_impl},
    {Py_tp_clear, statx_clear_impl},
    {0, NULL},
};

static PyType_Spec g_statx_operation_spec = {
    .name      = "_impl._StatxOperation",
    .basicsize = sizeof(StatxOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_statx_operation_slots,
};

PyTypeObject *statx_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_statx_operation_spec, (PyObject *)state->Operation_type);
}
