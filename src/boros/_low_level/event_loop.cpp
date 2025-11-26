// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "event_loop.hpp"

#include <cstdio>

#include "extension.hpp"

namespace boros {

    namespace {

        auto RaiseOsErrorFromRing(int res) -> void {
            errno = -res;
            PyErr_SetFromErrno(PyExc_OSError);
        }

    }

    auto EventLoop::Create(python::Module mod, python::Object<EventLoopPolicy> *policy_) -> python::Object<EventLoop>* {
        auto &state    = python::GetModuleState<ModuleState>(mod.raw);
        auto &policy   = policy_->Get();
        auto *loop_key = state.local_event_loop;

        assert(PyThread_tss_is_created(loop_key));

        auto *loop = reinterpret_cast<python::Object<EventLoop>*>(PyThread_tss_get(loop_key));
        if (loop == nullptr) [[likely]] {
            loop = python::Alloc<EventLoop>(state.EventLoopType);
            auto &ring = loop->Get().m_ring;

            if (int res = ring.Initialize(policy.sq_entries, policy.cq_entries); res < 0) [[unlikely]] {
                RaiseOsErrorFromRing(res);
                return nullptr;
            }

            if (int res = ring.Enable(); res < 0) [[unlikely]] {
                RaiseOsErrorFromRing(res);
                return nullptr;
            }

            PyThread_tss_set(state.local_event_loop, loop);
        }

        Py_INCREF(reinterpret_cast<PyObject*>(loop));
        return loop;
    }

    auto EventLoop::Destroy(python::Module mod) -> void {
        auto &state    = python::GetModuleState<ModuleState>(mod.raw);
        auto *loop_key = state.local_event_loop;

        assert(PyThread_tss_is_created(loop_key));

        auto *loop = reinterpret_cast<PyObject*>(PyThread_tss_get(loop_key));
        if (loop != nullptr) {
            python::DefaultDealloc<EventLoop>(loop);
            PyThread_tss_set(state.local_event_loop, nullptr);
        }
    }

    auto EventLoop::Get(python::Module mod) -> python::Object<EventLoop>* {
        auto &state    = python::GetModuleState<ModuleState>(mod.raw);
        auto *loop_key = state.local_event_loop;

        assert(PyThread_tss_is_created(loop_key));

        auto *loop = PyThread_tss_get(loop_key);
        if (loop == nullptr) [[unlikely]] {
            PyErr_SetString(PyExc_RuntimeError,
                "no running event loop on the current thread");
            return nullptr;
        }

        Py_INCREF(reinterpret_cast<PyObject*>(loop));
        return reinterpret_cast<python::Object<EventLoop>*>(loop);
    }

    auto EventLoop::Tick() -> void {
        int res = m_ring.SubmitAndWait(1);
        if (res < 0) [[unlikely]] {
            RaiseOsErrorFromRing(res);
            return;
        }

        for (auto completion : m_ring.GetCompletionQueue()) {
            std::fprintf(stderr, "res: %d\n", completion.GetResult());
        }
    }

    auto EventLoop::Nop(python::Module mod, int res) -> void {
        auto &state = python::GetModuleState<ModuleState>(mod.raw);
        auto *loop = reinterpret_cast<python::Object<EventLoop>*>(PyThread_tss_get(state.local_event_loop));
        if (loop != nullptr) {
            auto &queue = loop->Get().m_ring.GetSubmissionQueue();
            auto entry = queue.PushUnchecked();
            entry.Prepare(IORING_OP_NOP, -1, nullptr, res, 0);
            entry.sqe->nop_flags |= IORING_NOP_INJECT_RESULT;
        }
    }

    namespace {

        auto g_event_loop_policy_members = python::MemberTable(
            PythonMember(EventLoopPolicy, sq_entries),
            PythonMember(EventLoopPolicy, cq_entries),
            PythonMember(EventLoopPolicy, wqfd)
        );

        auto g_event_loop_policy_slots = python::TypeSlotTable(
            python::TypeSlot(Py_tp_new, python::DefaultNew<EventLoopPolicy>),
            python::TypeSlot(Py_tp_dealloc, python::DefaultDealloc<EventLoopPolicy>),
            python::TypeSlot(Py_tp_members, g_event_loop_policy_members.data())
        );

        constinit auto g_event_loop_policy_spec = python::TypeSpec<EventLoopPolicy>(
            "EventLoopPolicy",
            g_event_loop_policy_slots.data()
        );

        auto g_event_loop_methods = python::MethodTable(
            python::Method<&EventLoop::Tick>("tick")
        );

        auto g_event_loop_slots = python::TypeSlotTable(
            python::TypeSlot(Py_tp_new, python::DefaultNew<EventLoop>),
            python::TypeSlot(Py_tp_dealloc, python::DefaultDealloc<EventLoop>),
            python::TypeSlot(Py_tp_methods, g_event_loop_methods.data())
        );

        constinit auto g_event_loop_spec = python::TypeSpec<EventLoop>(
            "EventLoop",
            g_event_loop_slots.data()
        );

    }

    auto EventLoopPolicy::Register(PyObject *mod) -> PyTypeObject* {
        return python::AddTypeToModule(mod, g_event_loop_policy_spec);
    }

    auto EventLoop::Register(PyObject *mod) -> PyTypeObject* {
        return python::AddTypeToModule(mod, g_event_loop_spec);
    }

}
