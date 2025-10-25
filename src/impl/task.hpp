// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

namespace boros {

    /// Represents an asynchronous unit of work. This wraps a coroutine object
    /// and drives its execution. Note that Tasks in other async frameworks
    /// represent coroutines running in the background; this is not the case
    /// for boros. All coroutines are represented as Tasks in the system.
    struct Task {
    private:
        struct ListLink {
            ListLink *Prev;
            ListLink *Next;

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

    private:
        PyObject *m_coro;
        ListLink m_link;

    public:
        explicit Task(PyObject *coro) noexcept;

        ~Task() noexcept;

        /// Resumes execution of a suspended coroutine by injecting a given
        /// value as the result of a suspension point. ret will then point
        /// to a value produced at the suspension point.
        auto Send(PyObject *value, PyObject **ret) const noexcept -> PySendResult;

        /// Injects an exception object into the coroutine at the current
        /// suspension point. Used to report errors from the event loop.
        auto Throw(PyObject *exc) const noexcept -> void;
    };

}
