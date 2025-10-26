// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "task.hpp"

#include <cassert>
#include <cstddef>

namespace boros {

    namespace {

        // The byte offset from Task to its list link. Used for conversions
        // from a ListLink pointer to a Task pointer and vice versa.
        constexpr inline std::size_t LinkOffset = offsetof(Task, link);

    }

    Task::ListLink::ListLink() noexcept : prev(this), next(this) {}

    auto Task::ListLink::IsLinked() const noexcept -> bool {
        return next != this;
    }

    auto Task::ListLink::LinkPrevious(ListLink* link) noexcept -> void {
        assert(!link->IsLinked());

        auto *last_prev = link->prev;
        link->prev = prev;
        last_prev->next = this;
        prev->next = link;
        prev = last_prev;
    }

    auto Task::ListLink::LinkNext(ListLink* link) noexcept -> void {
        assert(!link->IsLinked());

        auto *last_prev = link->prev;
        link->prev = this;
        last_prev->next = next;
        next->prev = last_prev;
        next = link;
    }

    auto Task::ListLink::Unlink() noexcept -> void {
        prev->next = next;
        next->prev = prev;
        prev = next = this;
    }

    auto Task::List::ToLink(Task& v) noexcept -> ListLink* {
        auto *link = reinterpret_cast<std::byte*>(&v) + LinkOffset;
        return reinterpret_cast<ListLink*>(link);
    }

    auto Task::List::ToLink(const Task& v) noexcept -> const ListLink* {
        auto *link = reinterpret_cast<const std::byte*>(&v) + LinkOffset;
        return reinterpret_cast<const ListLink*>(link);
    }

    auto Task::List::ToTask(ListLink* v) noexcept -> Task& {
        auto *parent = reinterpret_cast<std::byte*>(v) - LinkOffset;
        return *reinterpret_cast<Task*>(parent);
    }

    auto Task::List::ToTask(const ListLink* v) noexcept -> const Task& {
        auto *parent = reinterpret_cast<const std::byte*>(v) - LinkOffset;
        return *reinterpret_cast<const Task*>(parent);
    }

    auto Task::List::IsEmpty() const noexcept -> bool {
        return !m_head.IsLinked();
    }

    auto Task::List::GetBack() const noexcept -> const Task& {
        return ToTask(m_head.prev);
    }

    auto Task::List::GetBack() noexcept -> Task& {
        return ToTask(m_head.prev);
    }

    auto Task::List::GetFront() const noexcept -> const Task& {
        return ToTask(m_head.next);
    }

    auto Task::List::GetFront() noexcept -> Task& {
        return ToTask(m_head.next);
    }

    auto Task::List::PushBack(Task& v) noexcept -> void {
        m_head.LinkPrevious(ToLink(v));
    }

    auto Task::List::PushFront(Task& v) noexcept -> void {
        m_head.LinkNext(ToLink(v));
    }

    auto Task::List::PopBack() noexcept -> void {
        m_head.prev->Unlink();
    }

    auto Task::List::PopFront() noexcept -> void {
        m_head.next->Unlink();
    }

    auto Task::List::Remove(Task& v) noexcept -> void {
        auto *link = ToLink(v);
        assert(link->IsLinked());
        link->Unlink();
    }

    auto Task::List::Clear() noexcept -> void {
        while (!this->IsEmpty()) {
            this->PopFront();
        }
    }

    auto Task::IterNext(PyObject *self_) noexcept -> PyObject* {
        auto *self = reinterpret_cast<Task*>(self_);

        switch (self->state) {
        case State_Pending:
            // XXX: Event loop must check for Task instance yields
            // TODO: Submit operation to io_uring.
            return Py_NewRef(self_);

        case State_Finished:
            // TODO: Return something more useful than None.
            PyErr_SetObject(PyExc_StopIteration, Py_None);
            return nullptr;

        default:
            // TODO: Custom exception type?
            PyErr_SetString(PyExc_RuntimeError, "invalid state");
            return nullptr;
        }
    }

    // TODO: Call this from C++ with io_uring_cqe*.
    auto Task::Unblock() noexcept -> PyObject* {
        if (state != State_Pending) {
            // TODO: Custom exception type?
            PyErr_SetString(PyExc_RuntimeError, "invalid state");
            return nullptr;
        }

        state = State_Finished;
        Py_RETURN_NONE;
    }

    namespace {

        auto g_task_methods = python::MethodTable(
            python::Method<&Task::Unblock>("unblock")
        );

        auto g_task_slots = python::TypeSlotTable(
            python::TypeSlot(Py_tp_new, &python::DefaultNew<Task>),
            python::TypeSlot(Py_tp_dealloc, &python::Dealloc<Task>),
            python::TypeSlot(Py_tp_methods, g_task_methods.data()),
            python::TypeSlot(Py_tp_iter, &PyObject_SelfIter),
            python::TypeSlot(Py_tp_iternext, &Task::IterNext),
            python::TypeSlot(Py_am_await, &PyObject_SelfIter)
        );

        auto g_task_spec = python::TypeSpec<Task>(
            "Task",
            g_task_slots.data(),
            Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_IMMUTABLETYPE
        );

    }

    auto Task::Register(python::Module mod) noexcept -> PyObject* {
        return mod.Add(g_task_spec);
    }

}
