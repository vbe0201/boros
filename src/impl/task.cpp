// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "task.hpp"

#include <cassert>
#include <cstddef>

namespace boros::impl {

    Task::ListLink::ListLink() noexcept : Prev(this), Next(this) {}

    auto Task::ListLink::IsLinked() const noexcept -> bool {
        return Next != this;
    }

    auto Task::ListLink::LinkPrevious(ListLink* link) noexcept -> void {
        assert(!link->IsLinked());

        auto *last_prev = link->Prev;
        link->Prev = Prev;
        last_prev->Next = this;
        Prev->Next = link;
        Prev = last_prev;
    }

    auto Task::ListLink::LinkNext(ListLink* link) noexcept -> void {
        assert(!link->IsLinked());

        auto *last_prev = link->Prev;
        link->Prev = this;
        last_prev->Next = Next;
        Next->Prev = last_prev;
        Next = link;
    }

    auto Task::ListLink::Unlink() noexcept -> void {
        Prev->Next = Next;
        Next->Prev = Prev;
        Prev = Next = this;
    }

    auto Task::List::ToLink(Task& v) noexcept -> ListLink* {
        auto *link = reinterpret_cast<std::byte*>(&v) + offsetof(Task, m_link);
        return reinterpret_cast<ListLink*>(link);
    }

    auto Task::List::ToLink(const Task& v) noexcept -> const ListLink* {
        auto *link = reinterpret_cast<const std::byte*>(&v) + offsetof(Task, m_link);
        return reinterpret_cast<const ListLink*>(link);
    }

    auto Task::List::ToTask(ListLink* v) noexcept -> Task& {
        auto *parent = reinterpret_cast<std::byte*>(v) - offsetof(Task, m_link);
        return *reinterpret_cast<Task*>(parent);
    }

    auto Task::List::ToTask(const ListLink* v) noexcept -> const Task& {
        auto *parent = reinterpret_cast<const std::byte*>(v) - offsetof(Task, m_link);
        return *reinterpret_cast<const Task*>(parent);
    }

    auto Task::List::IsEmpty() const noexcept -> bool {
        return !m_head.IsLinked();
    }

    auto Task::List::GetBack() const noexcept -> const Task& {
        return ToTask(m_head.Prev);
    }

    auto Task::List::GetBack() noexcept -> Task& {
        return ToTask(m_head.Prev);
    }

    auto Task::List::GetFront() const noexcept -> const Task& {
        return ToTask(m_head.Next);
    }

    auto Task::List::GetFront() noexcept -> Task& {
        return ToTask(m_head.Next);
    }

    auto Task::List::PushBack(Task& v) noexcept -> void {
        m_head.LinkPrevious(ToLink(v));
    }

    auto Task::List::PushFront(Task& v) noexcept -> void {
        m_head.LinkNext(ToLink(v));
    }

    auto Task::List::PopBack() noexcept -> void {
        m_head.Prev->Unlink();
    }

    auto Task::List::PopFront() noexcept -> void {
        m_head.Next->Unlink();
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

    Task::Task(PyObject* coro) noexcept : m_coro(coro) {
        assert(coro != nullptr);
        Py_INCREF(coro);
    }

    Task::~Task() noexcept {
        Py_DECREF(m_coro);
    }

    auto Task::Send(PyObject* value, PyObject** ret) const noexcept -> PySendResult {
        return PyIter_Send(m_coro, value, ret);
    }

    auto Task::Throw(PyObject *exc) const noexcept -> void {
        assert(exc != nullptr);

        PyObject *name     = PyUnicode_InternFromString("throw");
        PyObject *args[2]  = {m_coro, exc};
        std::size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
        PyObject_VectorcallMethod(name, args, nargsf, nullptr);
    }

}
