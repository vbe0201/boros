/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "op/sockopt.h"

#include "util/python.h"

#include <liburing.h>
#include <string.h>

#include "module.h"

/* GetsockoptOperation implementation */

static void getsockopt_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    GetsockoptOperation *op = (GetsockoptOperation *)self;

    char *buf = PyBytes_AS_STRING(op->buf);
    io_uring_prep_cmd_sock(sqe, SOCKET_URING_OP_GETSOCKOPT, op->base.scratch, op->level, op->optname, buf, op->optlen);
}

static void getsockopt_complete(PyObject *self, struct io_uring_cqe *cqe) {
    GetsockoptOperation *op = (GetsockoptOperation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->base.outcome);
        return;
    }

    if (op->raw) {
        _PyBytes_Resize(&op->buf, cqe->res);
        outcome_capture(&op->base.outcome, Py_NewRef(op->buf));
    } else {
        int val;
        memcpy(&val, PyBytes_AS_STRING(op->buf), sizeof(val));
        outcome_capture(&op->base.outcome, PyLong_FromLong(val));
    }
}

static OperationVTable g_getsockopt_operation_vtable = {
    .prepare  = getsockopt_prepare,
    .complete = getsockopt_complete,
};

PyObject *getsockopt_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 3 && nargs != 4) {
        PyErr_Format(PyExc_TypeError, "Expected 3 or 4 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    int level;
    if (!python_parse_int(&level, args[1])) {
        return NULL;
    }

    int optname;
    if (!python_parse_int(&optname, args[2])) {
        return NULL;
    }

    bool raw;
    int optlen;
    if (nargs == 4) {
        if (!python_parse_int(&optlen, args[3])) {
            return NULL;
        }
        raw = true;
    } else {
        optlen = sizeof(int);
        raw    = false;
    }

    PyObject *buf = PyBytes_FromStringAndSize(NULL, optlen);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

    GetsockoptOperation *op = (GetsockoptOperation *)operation_alloc(state->GetsockoptOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_getsockopt_operation_vtable;
        op->base.scratch = fd;
        op->buf          = buf;
        op->level        = level;
        op->optname      = optname;
        op->optlen       = optlen;
        op->raw          = raw;
    } else {
        Py_DECREF(buf);
    }

    return (PyObject *)op;
}

static int getsockopt_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    GetsockoptOperation *op = (GetsockoptOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->buf);
    return operation_traverse(&op->base, visit, arg);
}

static int getsockopt_clear_impl(PyObject *self) {
    GetsockoptOperation *op = (GetsockoptOperation *)self;

    Py_CLEAR(op->buf);
    return operation_clear(&op->base);
}

static PyType_Slot g_getsockopt_operation_slots[] = {
    {Py_tp_traverse, getsockopt_traverse_impl},
    {Py_tp_clear, getsockopt_clear_impl},
    {0, NULL},
};

static PyType_Spec g_getsockopt_operation_spec = {
    .name      = "_impl._GetsockoptOperation",
    .basicsize = sizeof(GetsockoptOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_getsockopt_operation_slots,
};

PyTypeObject *getsockopt_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_getsockopt_operation_spec,
                                                    (PyObject *)state->Operation_type);
}

/* SetsockoptOperation implementation */

static void setsockopt_prepare(PyObject *self, struct io_uring_sqe *sqe) {
    SetsockoptOperation *op = (SetsockoptOperation *)self;

    char *buf = PyBytes_AS_STRING(op->buf);
    io_uring_prep_cmd_sock(sqe, SOCKET_URING_OP_SETSOCKOPT, op->base.scratch, op->level, op->optname, buf, op->optlen);
}

static void setsockopt_complete(PyObject *self, struct io_uring_cqe *cqe) {
    Operation *op = (Operation *)self;

    if (cqe->res < 0) {
        errno = -cqe->res;
        outcome_capture_errno(&op->outcome);
    } else {
        outcome_capture(&op->outcome, Py_None);
    }
}

static OperationVTable g_setsockopt_operation_vtable = {
    .prepare  = setsockopt_prepare,
    .complete = setsockopt_complete,
};

PyObject *setsockopt_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf) {
    ImplState *state = PyModule_GetState(mod);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 4) {
        PyErr_Format(PyExc_TypeError, "Expected 4 arguments, got %zu instead", nargs);
        return NULL;
    }

    int fd;
    if (!python_parse_int(&fd, args[0])) {
        return NULL;
    }

    int level;
    if (!python_parse_int(&level, args[1])) {
        return NULL;
    }

    int optname;
    if (!python_parse_int(&optname, args[2])) {
        return NULL;
    }

    PyObject *buf;
    int optlen;
    if (PyBytes_Check(args[3])) {
        buf    = Py_NewRef(args[3]);
        optlen = (int)PyBytes_GET_SIZE(buf);
    } else {
        int val;
        if (!python_parse_int(&val, args[3])) {
            return NULL;
        }
        buf = PyBytes_FromStringAndSize((const char *)&val, sizeof(val));
        if (buf == NULL) {
            return PyErr_NoMemory();
        }
        optlen = sizeof(val);
    }

    SetsockoptOperation *op = (SetsockoptOperation *)operation_alloc(state->SetsockoptOperation_type, state);
    if (op != NULL) {
        op->base.vtable  = &g_setsockopt_operation_vtable;
        op->base.scratch = fd;
        op->buf          = buf;
        op->level        = level;
        op->optname      = optname;
        op->optlen       = optlen;
    } else {
        Py_DECREF(buf);
    }

    return (PyObject *)op;
}

static int setsockopt_traverse_impl(PyObject *self, visitproc visit, void *arg) {
    SetsockoptOperation *op = (SetsockoptOperation *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(op->buf);
    return operation_traverse(&op->base, visit, arg);
}

static int setsockopt_clear_impl(PyObject *self) {
    SetsockoptOperation *op = (SetsockoptOperation *)self;

    Py_CLEAR(op->buf);
    return operation_clear(&op->base);
}

static PyType_Slot g_setsockopt_operation_slots[] = {
    {Py_tp_traverse, setsockopt_traverse_impl},
    {Py_tp_clear, setsockopt_clear_impl},
    {0, NULL},
};

static PyType_Spec g_setsockopt_operation_spec = {
    .name      = "_impl._SetsockoptOperation",
    .basicsize = sizeof(SetsockoptOperation),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .slots     = g_setsockopt_operation_slots,
};

PyTypeObject *setsockopt_operation_register(PyObject *mod) {
    ImplState *state = PyModule_GetState(mod);
    return (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_setsockopt_operation_spec,
                                                    (PyObject *)state->Operation_type);
}
