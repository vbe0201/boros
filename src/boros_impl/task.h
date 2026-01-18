/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "util/python.h"

/* An intrusive list link embedded into Task. */
typedef struct _TaskLink {
    struct _TaskLink *prev;
    struct _TaskLink *next;
} TaskLink;

/* An intrusive, doubly-linked list of Tasks. */
typedef struct {
    TaskLink head;
} TaskList;

/* A concurrent thread of execution. */
typedef struct {
    PyObject_HEAD
    TaskLink link;
    PyObject *name;
    PyObject *coro;
} Task;

/* TaskList API */

/* Initializes a new task list to empty state. */
void task_list_init(TaskList *self);

/* Checks if the list is currently empty. */
bool task_list_empty(TaskList *self);

/* Getters for back and front element of the list. */
Task *task_list_back(TaskList *self);
Task *task_list_front(TaskList *self);

/* Adds a new element to back or front of the list. */
void task_list_push_back(TaskList *self, Task *task);
void task_list_push_front(TaskList *self, Task *task);

/* Removes the element from back or front of the list. */
void task_list_pop_back(TaskList *self);
void task_list_pop_front(TaskList *self);

/* Removes a given element that is currently in the list. */
void task_list_remove(TaskList *self, Task *task);

/* Clears all elements from the list. */
void task_list_clear(TaskList *self);

/* Task API */

/* Allocates a Task instance. The object can be exposed to Python. */
Task *task_create(PyObject *mod, PyObject *name, PyObject *coro);

/* Registers Task as a Python class onto the module. */
PyTypeObject *task_register(PyObject *mod);
