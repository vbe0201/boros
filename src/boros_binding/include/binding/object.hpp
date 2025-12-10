// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include <cassert>
#include <cstddef>

namespace boros::python {

    // Utilities for creating Python objects from C++.
    //
    // Python objects must start with the common PyObject_HEAD base
    // structure which represents the root PyObject* you will often
    // see. Since the C language doesn't sport inheritance, this is
    // emulated entirely through pointer casting and the type slots.
    //
    // We provide the Object as an abstraction that can wrap any
    // standard C++ object and make it usable as a Python object.
    //
    // # Creating Types
    //
    // Creating types is done by defining a static TypeSpec for a
    // type and providing appropriate slot values. For this API,
    // it is recommended to use custom tp_new and tp_dealloc slots
    // implemented in DefaultNew and DefaultDealloc.
    //
    // You may omit tp_new when making a type which has the flag
    // Py_TPFLAGS_DISALLOW_INSTANTIATION set.
    //
    // # Garbage Collection
    //
    // When creating new types, they must also implement the garbage
    // collection protocol to avoid memory leaks.
    //
    // This requires providing a tp_traverse slot which visits all
    // Python reference objects that may form GC cycles. For types
    // that cannot form cycles (think strings or integers), this
    // is not strictly required.
    //
    // It is required that the tp_traverse routine always visits
    // `Py_TYPE(self)` of the given object.
    //
    // # Instantiating types
    //
    // In order to instantiate a type and bind it to a definining
    // module, use the AddTypeToModule helper.

    /// A helper that can be used to allocate an ABI compatible
    /// Python object wrapping an arbitrary C++ type.
    struct ObjectStorage {
        PyObject_HEAD
        std::byte body[0];
    };

    namespace impl {

        template <typename T>
        struct SizedObjectStorageForAlloc {
            PyObject_HEAD
            std::byte body[sizeof(T)];
        };

    }  // namespace impl

    /// Provides a typed handle to a Python object allocated as an
    /// ObjectStorage instance. This allows us to access its body
    /// without violating strict aliasing.
    template <typename T>
    struct ObjectRef {
    public:
        ObjectStorage *storage;

    public:
        explicit ObjectRef(std::nullptr_t) : storage(nullptr) {}

        explicit ObjectRef(PyObject *ob) : storage(reinterpret_cast<ObjectStorage *>(ob)) {}

        template <typename U>
        explicit ObjectRef(ObjectRef<U> rhs) : storage(rhs.GetStorage()) {}

        T *Get() { return std::launder(reinterpret_cast<T *>(&storage->body)); }

        T &operator*() { return *this->Get(); }
        T *operator->() { return this->Get(); }

        operator bool() const { return storage != nullptr; }
        operator PyObject *() { return reinterpret_cast<PyObject *>(storage); }
    };

    /// Casts a Python object to a concrete object type. This performs
    /// an isinstance() check with the given type object. When the
    /// cast fails, an exception is set and nullptr is returned.
    template <typename T>
    ObjectRef<T> CastExact(PyObject *ob, PyTypeObject *tp) {
        if (int res = PyObject_IsInstance(ob, reinterpret_cast<PyObject *>(tp)); res != 1) {
            if (res == 0) {
                PyErr_SetString(PyExc_TypeError, "received invalid object type");
            }
            return ObjectRef<T>{nullptr};
        }

        return ObjectRef<T>{ob};
    }

    /// Allocates a new Python object of the given type.
    template <typename T, typename... Args>
        requires(alignof(T) <= alignof(PyObject))
    ObjectRef<T> Alloc(PyTypeObject *tp, Args &&...args) {
#ifdef Py_LIMITED_API
        auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));
#else
        auto *tp_alloc = tp->tp_alloc;
#endif

        assert(tp_alloc != nullptr);
        ObjectRef<T> self{tp_alloc(tp, 0)};
        if (self) [[likely]] {
            std::construct_at(reinterpret_cast<T *>(self.storage->body), std::forward<Args>(args)...);
        }

        return self;
    }

    /// Implements the tp_new slot by allocating an instance and
    /// default-constructing the C++ payload.
    template <typename T>
    PyObject *DefaultNew(PyTypeObject *tp, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds)) {
        return Alloc<T>(tp);
    }

    /// Implements the tp_dealloc slot by calling the C++ destructor
    /// and properly disposing of memory and references.
    template <typename T>
    void DefaultDealloc(PyObject *self) {
        PyTypeObject *tp = Py_TYPE(self);

        // If this object participates in garbage collection, we must
        // untrack it before we can destroy it.
        if (PyType_HasFeature(tp, Py_TPFLAGS_HAVE_GC)) {
            PyObject_GC_UnTrack(self);
        }

        // Clear references from the object. The garbage collector
        // might call this, but it avoids code duplication.
#ifdef Py_LIMITED_API
        auto *tp_clear = reinterpret_cast<inquiry>(PyType_GetSlot(tp, Py_tp_clear));
#else
        auto *tp_clear = tp->tp_clear;
#endif
        if (tp_clear != nullptr && tp_clear(self) < 0) {
            PyErr_WriteUnraisable(self);
        }

        // Now call the C++ destructor for business logic cleanup.
        ObjectRef<T> ref{self};
        std::destroy_at(ref.Get());

        // Deallocate the object memory.
#ifdef Py_LIMITED_API
        auto *tp_free = reinterpret_cast<freefunc>(PyType_GetSlot(tp, Py_tp_free));
#else
        auto *tp_free = tp->tp_free;
#endif
        assert(tp_free != nullptr);
        tp_free(self);

        // Heap types get a reference to the type at allocation,
        // so we must remove it here.
        Py_DECREF(tp);
    }

    /// Provides an implementation of a tp_traverse slot that forwards
    /// to a custom Traverse member function on the type.
    template <typename T>
    int DefaultTraverse(PyObject *ob, visitproc visit, void *arg) {
        Py_VISIT(Py_TYPE(ob));

        ObjectRef<T> self{ob};
        if constexpr (requires { self->Traverse(visit, arg); }) {
            return self->Traverse(visit, arg);
        } else {
            return 0;
        }
    }

    /// Provides an implementation of a tp_clear slot that forwards
    /// to a custom Clear member function on the type.
    template <typename T>
    int DefaultClear(PyObject *ob) {
        ObjectRef<T> self{ob};
        if constexpr (requires { self->Clear(); }) {
            return self->Clear();
        } else {
            return 0;
        }
    }

    /// Creates a Python type slot entry from the given data.
    template <typename Ptr>
        requires std::is_pointer_v<Ptr>
    PyType_Slot TypeSlot(int slot, Ptr ptr) {
        return {slot, reinterpret_cast<void *>(ptr)};
    }

    /// Creates a table of type slots suitable for use with
    /// type object creation.
    template <typename... Ts>
        requires(std::is_same_v<Ts, PyType_Slot> && ...)
    std::array<PyType_Slot, sizeof...(Ts) + 1> TypeSlotTable(Ts... slots) {
        return {{slots..., {0, nullptr}}};
    }

    /// Creates a type specification from the given metadata.
    template <typename T>
    constexpr PyType_Spec TypeSpec(const char *name, PyType_Slot *slots, unsigned int flags = Py_TPFLAGS_DEFAULT) {
        return {name, sizeof(impl::SizedObjectStorageForAlloc<T>), 0, flags, slots};
    }

}  // namespace boros::python
