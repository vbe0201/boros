/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include <linux/stat.h>

#include "op/base.h"

/* Python object exposing the result of a statx(2) call. */
typedef struct {
    PyObject_HEAD
    __s64 atime;
    __u32 atime_nsec;
    __u32 blksize;
    __u64 blocks;
    __s64 ctime;
    __u32 ctime_nsec;
    __u64 dev_major;
    __u64 dev_minor;
    __u32 gid;
    __u64 ino;
    __u16 mode;
    __s64 mtime;
    __u32 mtime_nsec;
    __u32 nlink;
    __u64 rdev_major;
    __u64 rdev_minor;
    __u64 size;
    __u32 uid;
} StatxResult;

typedef struct {
    /* flags is stored in base.scratch */
    Operation base;
    PyObject *path;
    int dfd;
    unsigned int mask;
    struct statx stx;
} StatxOperation;

PyTypeObject *statx_result_register(PyObject *mod);

PyObject *statx_operation_create(PyObject *mod, PyObject *const *args, Py_ssize_t nargsf);
PyTypeObject *statx_operation_register(PyObject *mod);
