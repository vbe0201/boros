// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "task.hpp"

#include <cassert>
#include <cstddef>

#include "extension.hpp"

namespace boros {

    Task::ListLink::ListLink() : prev(this), next(this) {}

    auto Task::ListLink::IsLinked() const -> bool {
        return next != this;
    }

    auto Task::ListLink::LinkPrev(ListLink *link) -> void {
        assert(!link->IsLinked());

        auto *last_prev = link->prev;
        link->prev      = prev;
        last_prev->next = this;
        prev->next      = link;
        prev            = last_prev;
    }

    auto Task::ListLink::LinkNext(ListLink *link) -> void {
        assert(!link->IsLinked());

        auto *last_prev = link->prev;
        link->prev      = this;
        last_prev->next = next;
        next->prev      = last_prev;
        next            = link;
    }

    auto Task::ListLink::Unlink() -> void {
        prev->next = next;
        next->prev = prev;
        prev = next = this;
    }

    auto Task::List::ToLink(Task &v) -> ListLink * {
        auto *link = reinterpret_cast<std::byte *>(&v) + offsetof(Task, link);
        return reinterpret_cast<ListLink *>(link);
    }

    auto Task::List::ToLink(const Task &v) -> const ListLink * {
        auto *link = reinterpret_cast<const std::byte *>(&v) + offsetof(Task, link);
        return reinterpret_cast<const ListLink *>(link);
    }

    auto Task::List::ToTask(ListLink *v) -> Task & {
        auto *parent = reinterpret_cast<std::byte *>(v) - offsetof(Task, link);
        return *reinterpret_cast<Task *>(parent);
    }

    auto Task::List::ToTask(const ListLink *v) -> const Task & {
        auto *parent = reinterpret_cast<const std::byte *>(v) - offsetof(Task, link);
        return *reinterpret_cast<const Task *>(parent);
    }

    auto Task::List::IsEmpty() const -> bool {
        return !m_head.IsLinked();
    }

    auto Task::List::GetBack() -> Task & {
        return ToTask(m_head.prev);
    }

    auto Task::List::GetBack() const -> const Task & {
        return ToTask(m_head.prev);
    }

    auto Task::List::GetFront() -> Task & {
        return ToTask(m_head.next);
    }

    auto Task::List::GetFront() const -> const Task & {
        return ToTask(m_head.next);
    }

    auto Task::List::PushBack(Task &v) -> void {
        m_head.LinkPrev(ToLink(v));
    }

    auto Task::List::PushFront(Task &v) -> void {
        m_head.LinkNext(ToLink(v));
    }

    auto Task::List::PopBack() -> void {
        m_head.prev->Unlink();
    }

    auto Task::List::PopFront() -> void {
        m_head.next->Unlink();
    }

    auto Task::List::Remove(Task &v) -> void {
        auto *link = ToLink(v);
        assert(link->IsLinked());
        link->Unlink();
    }

    auto Task::List::Clear() -> void {
        while (!this->IsEmpty()) {
            this->PopFront();
        }
    }

    auto Task::Create(python::Module mod, PyObject *name, PyObject *coro) -> python::ObjectRef<Task> {
        auto &state = python::GetModuleState<ModuleState>(mod.raw);

        auto task = python::Alloc<Task>(state.TaskType);
        if (task) [[likely]] {
            task->name = name;
            task->coro = coro;
        }

        return task;
    }

    auto Task::GetName() const -> PyObject * {
        assert(name != nullptr);
        return Py_NewRef(name);
    }

    auto Task::GetCoro() const -> PyObject * {
        assert(coro != nullptr);
        return Py_NewRef(coro);
    }

    namespace {

        constexpr const char *g_task_doc =
            "A lightweight, concurrent thread of execution.\n\n"
            "Tasks are similar to OS threads, but they are managed by the boros scheduler\n"
            "instead of the OS scheduler. This makes them cheap to create and there is\n"
            "little overhead to switching between tasks.\n\n"
            "Task has no public constructor and appears immutable to Python code.\n"
            "Its public members are mostly useful for introspection and debugging.";

        auto g_task_properties = python::PropertyTable(
            python::ReadOnlyProperty<&Task::GetName>("name", "A string representation of the task name."),
            python::ReadOnlyProperty<&Task::GetCoro>("coro", "The coroutine object associated with the Task."));

        // TODO: repr
        // TODO: await frames stackwalking
        // TODO: contextvars
        auto g_task_slots = python::TypeSlotTable(python::TypeSlot(Py_tp_doc, const_cast<char *>(g_task_doc)),
                                                  python::TypeSlot(Py_tp_dealloc, &python::DefaultDealloc<Task>),
                                                  python::TypeSlot(Py_tp_traverse, &python::DefaultTraverse<Task>),
                                                  python::TypeSlot(Py_tp_clear, &python::DefaultClear<Task>),
                                                  python::TypeSlot(Py_tp_getset, g_task_properties.data()));

        constinit auto g_task_spec = python::TypeSpec<Task>(
            "_low_level.Task", g_task_slots.data(),
            Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION);

    }  // namespace

    auto Task::Traverse(visitproc visit, void *arg) -> int {
        Py_VISIT(name);
        Py_VISIT(coro);
        return 0;
    }

    auto Task::Clear() -> int {
        Py_CLEAR(name);
        Py_CLEAR(coro);
        return 0;
    }

    auto Task::Register(PyObject *mod) -> PyTypeObject * {
        return python::InstantiateType(mod, g_task_spec);
    }

}  // namespace boros
