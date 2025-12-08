// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "binding/python.hpp"

namespace boros {

    /// Represents a concurrent thread of execution. This wraps
    /// a coroutine object and drives its execution state.
    struct Task {
    private:
        struct ListLink {
            ListLink *prev;
            ListLink *next;

            ListLink();

            auto IsLinked() const -> bool;

            auto LinkPrev(ListLink *link) -> void;
            auto LinkNext(ListLink *link) -> void;
            auto Unlink() -> void;
        };

    public:
        /// An intrusive, doubly-linked list of Tasks. Intended
        /// for single-threaded usage and not threadsafe.
        class List {
        private:
            ListLink m_head;

        private:
            static auto ToLink(Task &v) -> ListLink *;
            static auto ToLink(const Task &v) -> const ListLink *;

            static auto ToTask(ListLink *v) -> Task &;
            static auto ToTask(const ListLink *v) -> const Task &;

        public:
            List() = default;

            auto IsEmpty() const -> bool;

            auto GetBack() -> Task &;
            auto GetBack() const -> const Task &;

            auto GetFront() -> Task &;
            auto GetFront() const -> const Task &;

            auto PushBack(Task &v) -> void;
            auto PushFront(Task &v) -> void;

            auto PopBack() -> void;
            auto PopFront() -> void;

            auto Remove(Task &v) -> void;
            auto Clear() -> void;
        };

    private:
        ListLink link;
        PyObject *name = nullptr;
        PyObject *coro = nullptr;

    public:
        Task() = default;

        /// Allocates a new Task object instance and fills it with
        /// the given metadata.
        static auto Create(python::Module mod, PyObject *name, PyObject *coro) -> python::Object<Task> *;

        /// Gets the name of this Task as a Python string object.
        auto GetName() const -> PyObject *;

        /// Gets the coroutine object associated with this Task.
        auto GetCoro() const -> PyObject *;

    public:
        /// Traverses the Task state during garbage collection.
        auto Traverse(visitproc visit, void *arg) -> int;

        /// Clears all Python references from the Task state.
        auto Clear() -> int;

        /// Registers Task as a Python class onto the module.
        static auto Register(PyObject *mod) -> PyTypeObject *;
    };

}  // namespace boros
