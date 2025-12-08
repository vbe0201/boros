// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include <array>
#include <type_traits>

#include "conversion.hpp"
#include "func_traits.hpp"
#include "object.hpp"

namespace boros::python {

    // Implements Python class properties based on simple C++
    // getter and setter member function pointers.
    //
    // The getter must be a function taking no arguments and
    // returning a value type that can be converted to Python.
    //
    // The setter must be a function returning nothing and
    // taking the value to store. When clearing the property,
    // the C++ default value for the detected type is passed.
    //
    // # Defining properties
    //
    // Make a PropertyTable consisting of ReadOnlyProperty()
    // and Property(), then add a Py_tp_getset type slot with
    // a pointer to that table.

    /// Wraps a C++ getter function to define a read-only Python
    /// property for the type.
    template <auto Get>
    PyGetSetDef ReadOnlyProperty(const char *name, const char *doc = nullptr) {
        using GetTraits = impl::FunctionTraits<decltype(Get)>;
        using GetCls    = typename GetTraits::ClassType;

        static_assert(!std::is_void_v<GetCls>, "Getter must be a member function pointer");

        constexpr auto Getter = [](PyObject *ob, void *closure) -> PyObject * {
            static_cast<void>(closure);

            auto &unwrapped = reinterpret_cast<Object<GetCls> *>(ob)->inner;
            return ToPython((unwrapped.*Get)());
        };

        return {name, Getter, nullptr, doc, nullptr};
    }

    /// Wraps C++ getter and setter functions to define a writable
    /// Python property for the type.
    template <auto Get, auto Set>
    PyGetSetDef Property(const char *name, const char *doc = nullptr) {
        using GetTraits = impl::FunctionTraits<decltype(Get)>;
        using GetCls    = typename GetTraits::ClassType;

        using SetTraits = impl::FunctionTraits<decltype(Set)>;
        using SetCls    = typename SetTraits::ClassType;
        using SetArg    = std::tuple_element_t<0, typename SetTraits::ArgsType>;

        static_assert(!std::is_void_v<GetCls> && std::is_same_v<GetCls, SetCls>,
                      "Getter and setter must be member function pointers on the same object");
        static_assert(!std::is_void_v<typename GetTraits::RetType> && std::is_void_v<typename SetTraits::RetType>,
                      "Getter must return non-void and setter must return void");

        constexpr auto Getter = [](PyObject *ob, void *closure) -> PyObject * {
            static_cast<void>(closure);

            auto &unwrapped = reinterpret_cast<Object<GetCls> *>(ob)->inner;
            return ToPython((unwrapped.*Get)());
        };

        constexpr auto Setter = [](PyObject *ob, PyObject *v, void *closure) -> int {
            static_cast<void>(closure);

            auto &unwrapped = reinterpret_cast<Object<SetCls> *>(ob)->inner;
            if (v != nullptr) {
                // Setting the property to a non-null value, so try to
                // parse the expected value type first.
                SetArg arg;
                if (!FromPython(&arg, v)) {
                    return -1;
                }

                (unwrapped.*Set)(std::move(arg));
            } else {
                // Here we must clear the property, so we just pass a
                // default value and let the setter do the rest.
                (unwrapped.*Set)(SetArg{});
            }

            return 0;
        };

        return {name, Getter, Setter, doc, nullptr};
    }

    /// Creates a table of properties suitable for use with type
    /// object creation.
    template <typename... Ps>
        requires(std::is_same_v<Ps, PyGetSetDef> && ...)
    constexpr std::array<PyGetSetDef, sizeof...(Ps) + 1> PropertyTable(Ps... properties) {
        return {{properties..., {nullptr, nullptr, nullptr, nullptr, nullptr}}};
    }

}  // namespace boros::python
