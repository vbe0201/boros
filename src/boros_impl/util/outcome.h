/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* Captures the result of a function, either a value or error. */
typedef struct {
    PyObject *value;
} Outcome;

/* Initializes an empty outcome object. */
void outcome_init(Outcome *outcome);

/* Checks if the outcome instance is currently filled. */
bool outcome_empty(Outcome *outcome);

/* Garbage collection hooks for an outcome. */
int outcome_traverse(Outcome *outcome, visitproc visit, void *arg);
void outcome_clear(Outcome *outcome);

/* Setters for storing a value or error object as the outcome. */
void outcome_store_result(Outcome *outcome, PyObject *ob);
void outcome_store_error(Outcome *outcome, PyObject *ob);

/* Captures either the provided value or an error into the outcome. */
void outcome_capture(Outcome *outcome, PyObject *ob);

/* Captures the currently raised exception into the outcome. */
void outcome_capture_error(Outcome *outcome);

/* Captures the current errno value into the outcome. */
void outcome_capture_errno(Outcome *outcome);

/* Unwraps the outcome, either returning a value or raising an error. */
PyObject *outcome_unwrap(Outcome *outcome);
