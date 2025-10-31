// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "wrapper/python.hpp"

#include <linux/io_uring.h>

#include "implmodule.hpp"

namespace boros {

    /// Represents an asynchronous unit of work. This wraps a coroutine object
    /// and drives its execution. Note that Tasks in other async frameworks
    /// represent coroutines running in the background; this is not the case
    /// for boros. All coroutines are represented as Tasks in the system.
    struct Task {
    private:
        struct ListLink {
            ListLink *prev;
            ListLink *next;

            ListLink() noexcept;

            auto IsLinked() const noexcept -> bool;

            auto LinkPrevious(ListLink *link) noexcept -> void;

            auto LinkNext(ListLink *link) noexcept -> void;

            auto Unlink() noexcept -> void;
        };

    public:
        /// An intrusive linked list of Task objects.
        class List {
        private:
            ListLink m_head;

        private:
            static auto ToLink(Task &v) noexcept -> ListLink*;
            static auto ToLink(const Task &v) noexcept -> const ListLink*;

            static auto ToTask(ListLink *v) noexcept -> Task&;
            static auto ToTask(const ListLink *v) noexcept -> const Task&;

        public:
            List() noexcept = default;

            auto IsEmpty() const noexcept -> bool;

            auto GetBack() const noexcept -> const Task&;
            auto GetBack() noexcept -> Task&;

            auto GetFront() const noexcept -> const Task&;
            auto GetFront() noexcept -> Task&;

            auto PushBack(Task &v) noexcept -> void;
            auto PushFront(Task &v) noexcept -> void;

            auto PopBack() noexcept -> void;
            auto PopFront() noexcept -> void;

            auto Remove(Task &v) noexcept -> void;

            auto Clear() noexcept -> void;
        };

    public:
        PyObject_HEAD

        /// Intrusive link that connects this Task with other tasks
        /// to form the runtime's run queue. This allows us to track
        /// tasks that should be unblocked without allocation.
        ListLink link;

        /// A weak reference to the coroutine that awaits this task.
        /// When the I/O is done, we will provide it with the result.
        python::WeakReference awaiter;

        /// The current state of the task. This is used to implement
        /// the iterator state machine which allows awaiting Tasks
        /// on the Python side.
        enum {
            State_Pending,
            State_Finished,
        } state = State_Pending;

    public:
        /// Allocates a new Task object.
        static auto New() noexcept -> Task*;

        /// Implements the __next__ iterator method for this type. This is
        /// the underlying implementation of the awaitable functionality.
        static auto IterNext(PyObject *self) noexcept -> PyObject*;

        /// Unblocks the waiting coroutine with the result of the I/O
        /// operation given through the completion queue entry.
        auto Unblock() noexcept -> PyObject*;

        /// Exposes the Task class to a given Python module.
        static auto Register(ImplModule mod) noexcept -> PyObject*;
    };

}
