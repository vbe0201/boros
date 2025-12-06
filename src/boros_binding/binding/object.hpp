// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include <cassert>

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

    /// Represents an ABI compatible Python object with an inner C++
    /// value payload. This decouples the ABI requirements from the
    /// business logic of a class.
    template <typename T>
    struct Object {
        PyObject_HEAD
        union {
            T inner;
        };

        T &Get() { return inner; }

        T &operator*() { return inner; }
        T *operator->() { return &inner; }

        operator PyObject *() { return reinterpret_cast<PyObject *>(this); }
    };

    /// Casts a Python object to a concrete object type. This performs
    /// an isinstance() check with the given type object. When the
    /// cast fails, an exception is set and nullptr is returned.
    template <typename T>
    Object<T> *CastExact(PyObject *ob, PyTypeObject *tp) {
        if (int res = PyObject_IsInstance(ob, reinterpret_cast<PyObject *>(tp)); res != 1) {
            if (res == 0) {
                PyErr_SetString(PyExc_TypeError, "received invalid object type");
            }
            return nullptr;
        }

        return reinterpret_cast<Object<T> *>(ob);
    }

    /// Allocates a new Python object of the given type.
    template <typename T>
    Object<T> *Alloc(PyTypeObject *tp) {
#ifdef Py_LIMITED_API
        auto *tp_alloc = reinterpret_cast<allocfunc>(PyType_GetSlot(tp, Py_tp_alloc));
#else
        auto *tp_alloc = tp->tp_alloc;
#endif

        assert(tp_alloc != nullptr);
        auto *self = reinterpret_cast<Object<T> *>(tp_alloc(tp, 0));
        if (self != nullptr) [[likely]] {
            std::construct_at(&self->inner);
        }

        return self;
    }

    /// Implements the tp_new slot by allocating an instance and
    /// default-constructing the C++ payload.
    template <typename T>
    PyObject *DefaultNew(PyTypeObject *tp, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds)) {
        return *Alloc<T>(tp);
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
        std::destroy_at(&reinterpret_cast<Object<T> *>(self)->inner);

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
        if (PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
            Py_DECREF(tp);
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
        return {name, sizeof(Object<T>), 0, flags, slots};
    }

}  // namespace boros::python
