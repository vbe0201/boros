// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "event_loop.hpp"

#include <cstdio>

#include "extension.hpp"

namespace boros {

    // TODO: Redesign submissions into the queue

    namespace {

        auto RaiseOsErrorFromRing(int res) -> void {
            errno = -res;
            PyErr_SetFromErrno(PyExc_OSError);
        }

    }  // namespace

    auto EventLoop::Create(python::Module mod, PyObject *policy_) -> PyObject * {
        auto &state    = python::GetModuleState<ModuleState>(mod.raw);
        auto *loop_key = state.local_event_loop;

        auto policy = python::CastExact<EventLoopPolicy>(policy_, state.EventLoopPolicyType);
        if (!policy) [[unlikely]] {
            return nullptr;
        }

        assert(PyThread_tss_is_created(loop_key));

        python::ObjectRef<EventLoop> loop{static_cast<PyObject*>(PyThread_tss_get(loop_key))};
        if (loop == nullptr) [[likely]] {
            loop = python::Alloc<EventLoop>(state.EventLoopType);
            if (loop == nullptr) [[unlikely]] {
            }
            auto &ring = loop->m_ring;
            auto &conf = *policy;

            if (int res = ring.Initialize(conf.sq_entries, conf.cq_entries); res < 0) [[unlikely]] {
                Py_DECREF(loop);
                RaiseOsErrorFromRing(res);
                return nullptr;
            }

            if (int res = ring.Enable(); res < 0) [[unlikely]] {
                Py_DECREF(loop);
                RaiseOsErrorFromRing(res);
                return nullptr;
            }

            PyThread_tss_set(state.local_event_loop, loop);
        }

        Py_INCREF(loop);
        return loop;
    }

    auto EventLoop::Destroy(python::Module mod) -> void {
        auto &state    = python::GetModuleState<ModuleState>(mod.raw);
        auto *loop_key = state.local_event_loop;

        assert(PyThread_tss_is_created(loop_key));

        auto *loop = reinterpret_cast<PyObject *>(PyThread_tss_get(loop_key));
        if (loop != nullptr) {
            Py_DECREF(loop);
            PyThread_tss_set(state.local_event_loop, nullptr);
        }
    }

    auto EventLoop::Get(python::Module mod) -> PyObject * {
        auto &state    = python::GetModuleState<ModuleState>(mod.raw);
        auto *loop_key = state.local_event_loop;

        assert(PyThread_tss_is_created(loop_key));

        auto *loop = reinterpret_cast<PyObject *>(PyThread_tss_get(loop_key));
        if (loop == nullptr) [[unlikely]] {
            PyErr_SetString(PyExc_RuntimeError, "no running event loop on the current thread");
            return nullptr;
        }

        Py_INCREF(loop);
        return loop;
    }

    auto EventLoop::Tick() -> void {
        int res = m_ring.Wait(1);
        if (res < 0) [[unlikely]] {
            RaiseOsErrorFromRing(res);
            return;
        }

        for (auto completion : m_ring.GetCompletionQueue()) {
            std::fprintf(stderr, "res: %d\n", completion.GetResult());
        }
    }

    auto EventLoop::Nop(python::Module mod, int res) -> void {
        python::ObjectRef<EventLoop> loop{EventLoop::Get(mod)};
        if (loop == nullptr) [[unlikely]] {
            return;
        }

        auto &sq = loop->m_ring.GetSubmissionQueue();
        if (sq.HasCapacityFor(1)) {
            auto entry = sq.Push();
            entry.Prepare(IORING_OP_NOP, -1, nullptr, res, 0);
            entry.sqe->nop_flags |= IORING_NOP_INJECT_RESULT;
        } else {
            PyErr_SetString(PyExc_OverflowError, "SQ is full");
        }
    }

    namespace {

        auto g_event_loop_policy_properties = python::PropertyTable(
            python::Property<&EventLoopPolicy::GetSqEntries, &EventLoopPolicy::SetSqEntries>("sq_entries"),
            python::Property<&EventLoopPolicy::GetCqEntries, &EventLoopPolicy::SetCqEntries>("cq_entries"),
            python::Property<&EventLoopPolicy::GetWqFd, &EventLoopPolicy::SetWqFd>("wqfd"));

        auto g_event_loop_policy_slots =
            python::TypeSlotTable(python::TypeSlot(Py_tp_new, python::DefaultNew<EventLoopPolicy>),
                                  python::TypeSlot(Py_tp_dealloc, python::DefaultDealloc<EventLoopPolicy>),
                                  python::TypeSlot(Py_tp_getset, g_event_loop_policy_properties.data()));

        constinit auto g_event_loop_policy_spec = python::TypeSpec<EventLoopPolicy>(
            "_low_level.EventLoopPolicy", g_event_loop_policy_slots.data(), Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE);

        auto g_event_loop_methods = python::MethodTable(python::Method<&EventLoop::Tick>("tick"));

        auto g_event_loop_slots =
            python::TypeSlotTable(python::TypeSlot(Py_tp_dealloc, python::DefaultDealloc<EventLoop>),
                                  python::TypeSlot(Py_tp_methods, g_event_loop_methods.data()));

        constinit auto g_event_loop_spec = python::TypeSpec<EventLoop>(
            "_low_level.EventLoop", g_event_loop_slots.data(), Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION);

    }  // namespace

    auto EventLoopPolicy::Register(PyObject *mod) -> PyTypeObject * {
        return python::InstantiateType(mod, g_event_loop_policy_spec);
    }

    auto EventLoop::Register(PyObject *mod) -> PyTypeObject * {
        return python::InstantiateType(mod, g_event_loop_spec);
    }

}  // namespace boros
