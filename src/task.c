/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "task.h"

#include <assert.h>
#include <stddef.h>

#include "module.h"

static inline void task_link_init(TaskLink *self) {
    self->prev = self;
    self->next = self;
}

static inline bool task_link_linked(TaskLink *self) {
    return self->next != self;
}

static inline void task_link_link_prev(TaskLink *self, TaskLink *link) {
    assert(!task_link_linked(link));

    TaskLink *last_prev = link->prev;
    link->prev          = self->prev;
    last_prev->next     = self;
    self->prev->next    = link;
    self->prev          = last_prev;
}

static inline void task_link_link_next(TaskLink *self, TaskLink *link) {
    assert(!task_link_linked(link));

    TaskLink *last_prev = link->prev;
    link->prev          = self;
    last_prev->next     = self->next;
    self->next->prev    = last_prev;
    self->next          = link;
}

static inline void task_link_unlink(TaskLink *self) {
    self->prev->next = self->next;
    self->next->prev = self->prev;
    self->prev = self->next = self;
}

void task_list_init(TaskList *self) {
    task_link_init(&self->head);
}

bool task_list_empty(TaskList *self) {
    return !task_link_linked(&self->head);
}

void task_list_move(TaskList *dst, TaskList *src) {
    if (task_list_empty(src)) {
        task_list_init(dst);
        return;
    }

    dst->head.next       = src->head.next;
    dst->head.prev       = src->head.prev;
    dst->head.next->prev = &dst->head;
    dst->head.prev->next = &dst->head;
    task_list_init(src);
}

Task *task_list_back(TaskList *self) {
    return (Task *)((uint8_t *)(self->head.prev) - offsetof(Task, link));
}

Task *task_list_front(TaskList *self) {
    return (Task *)((uint8_t *)(self->head.next) - offsetof(Task, link));
}

void task_list_push_back(TaskList *self, Task *task) {
    Py_INCREF(task);
    task_link_link_prev(&self->head, &task->link);
}

void task_list_push_front(TaskList *self, Task *task) {
    Py_INCREF(task);
    task_link_link_next(&self->head, &task->link);
}

Task *task_list_pop_back(TaskList *self) {
    Task *task = task_list_back(self);
    task_link_unlink(&task->link);
    return task;
}

Task *task_list_pop_front(TaskList *self) {
    Task *task = task_list_front(self);
    task_link_unlink(&task->link);
    return task;
}

void task_list_remove(TaskList *self, Task *task) {
    (void)self;

    TaskLink *link = &task->link;
    assert(task_link_linked(link));
    task_link_unlink(link);
    Py_DECREF(task);
}

void task_list_clear(TaskList *self) {
    while (!task_list_empty(self)) {
        task_list_pop_front(self);
    }
}

Task *task_create(PyObject *mod, PyObject *name, PyObject *coro) {
    ImplState *state = PyModule_GetState(mod);

    Task *task = (Task *)python_alloc(state->Task_type);
    if (task != NULL) {
        task_link_init(&task->link);
        task->name = Py_XNewRef(name);
        task->coro = Py_XNewRef(coro);
    }

    return task;
}

PyDoc_STRVAR(g_task_doc, "A lightweight, concurrent thread of execution.\n\n"
                         "Tasks are similar to OS threads, but they are managed by the boros "
                         "scheduler\n"
                         "instead of the OS scheduler. This makes them cheap to create and there "
                         "is\n"
                         "little overhead to switching between tasks.\n\n"
                         "Task has no public constructor and appears immutable to Python code.\n"
                         "Its public members are mostly useful for introspection and debugging.");

PyDoc_STRVAR(g_task_name_doc, "A string representation of the task name.");
PyDoc_STRVAR(g_task_coro_doc, "The coroutine object associated with the task.");

static PyObject *task_repr(PyObject *self) {
    Task *task = (Task *)self;
    return PyUnicode_FromFormat("<Task %R at %p>", task->name, task);
}

static int task_traverse(PyObject *self, visitproc visit, void *arg) {
    Task *task = (Task *)self;

    Py_VISIT(Py_TYPE(self));
    Py_VISIT(task->name);
    Py_VISIT(task->coro);
    return 0;
}

static int task_clear(PyObject *self) {
    Task *task = (Task *)self;

    Py_CLEAR(task->name);
    Py_CLEAR(task->coro);
    return 0;
}

static PyObject *task_name_get(PyObject *self, void *Py_UNUSED(closure)) {
    Task *task = (Task *)self;
    assert(task->name != NULL);
    return Py_NewRef(task->name);
}

static PyObject *task_coro_get(PyObject *self, void *Py_UNUSED(closure)) {
    Task *task = (Task *)self;
    assert(task->coro != NULL);
    return Py_NewRef(task->coro);
}

static PyGetSetDef g_task_properties[] = {
    {"name", task_name_get, NULL, g_task_name_doc, NULL},
    {"coro", task_coro_get, NULL, g_task_coro_doc, NULL},
    {NULL, NULL, NULL, NULL, NULL},
};

// clang-format off
// TODO: await frames stackwalking, contextvars
static PyType_Slot g_task_slots[] = {
    {Py_tp_doc, (void *)g_task_doc},
    {Py_tp_repr, task_repr},
    {Py_tp_dealloc, python_tp_dealloc},
    {Py_tp_traverse, task_traverse},
    {Py_tp_clear, task_clear},
    {Py_tp_getset, g_task_properties},
    {0, NULL},
};
// clang-format on

static PyType_Spec g_task_spec = {
    .name      = "_impl.Task",
    .basicsize = sizeof(Task),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .slots     = g_task_slots,
};

PyTypeObject *task_register(PyObject *mod) {
    PyTypeObject *tp = (PyTypeObject *)PyType_FromModuleAndSpec(mod, &g_task_spec, NULL);
    if (tp == NULL) {
        return NULL;
    }

    if (PyModule_AddType(mod, tp) < 0) {
        return NULL;
    }

    return tp;
}
