// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <Python.h>

#include "conversion.hpp"
#include "func_traits.hpp"

namespace boros::python {

    // This machinery enables native C++ functions to be called from
    // Python with the least amount of boilerplate.
    //
    // # Breakdown
    //
    // The inner workings can be broken down into these steps:
    //
    //   * Analyze the argument and return types of a C++ function
    //   * Select the most efficient Python call ABI for it
    //   * Convert Python arguments into C++ function arguments
    //   * Call the function
    //   * Translate the return value into a Python value
    //
    // In order to disambiguate between different types of Python
    // functions and methods, some simple rules are applied:
    //
    //   * Member function pointers are turned into instance methods
    //     expecting their defining class type as self.
    //
    //   * Static functions with a first parameter of type Module,
    //     Class, or Static are turned into freestanding module
    //     functions, classmethods, or staticmethods, respectively.
    //
    //   * Failure to comply with any of these rules will cause the
    //     function to be rejected with a compile error.
    //
    // When your C++ function must raise an error, it should simply
    // set the Python exception and return. Depending on the return
    // type, this may or may not be easy so it's possible to return
    // a PyObject* directly also (with nullptr in the error case).
    //
    // # Defining functions
    //
    // Make a MethodTable of Method()s, then store a pointer to it
    // either directly in the ModuleDefinition or as a Py_tp_methods
    // type slot.

    /// A marker for Python extension methods.
    struct Module {
        PyObject *raw;
    };

    /// A marker for Python classmethods.
    struct Class {
        PyTypeObject *tp;
    };

    /// A marker for Python staticmethods.
    struct Static {};

    inline bool FromPython(Module *out, PyObject *obj) {
        assert(PyModule_Check(obj));
        out->raw = obj;
        return true;
    }

    inline PyObject *ToPython(Module mod) {
        return mod.raw;
    }

    inline bool FromPython(Class *out, PyObject *obj) {
        assert(PyType_Check(obj));
        out->tp = reinterpret_cast<PyTypeObject *>(obj);
        return true;
    }

    inline PyObject *ToPython(Class cls) {
        return reinterpret_cast<PyObject *>(cls.tp);
    }

    inline bool FromPython(Static *, PyObject *obj) {
        static_cast<void>(obj);
        assert(obj == nullptr);
        return true;
    }

    namespace impl {

        // First, we need some machinery for parsing arguments from the
        // canonical Python function signature into what a C++ function
        // expects.
        //
        // The C++ parameters are represented as a tuple here and the
        // logic tries to select an appropriate parsing routine for
        // PyObject* depending on the type of argument.

        template <typename Cls, typename Tuple>
        struct ArgParser {
            static constexpr bool InstanceMethod = !std::is_void_v<Cls>;
            static constexpr bool ClassMethod    = false;
            static constexpr bool ModuleMethod   = false;
            static constexpr bool StaticMethod   = false;

            static constexpr std::size_t Arity = 0;

            static Tuple Parse(PyObject *Py_UNUSED(self), PyObject *const *Py_UNUSED(args),
                               Py_ssize_t Py_UNUSED(nargs)) {
                static_assert(InstanceMethod, "Only instance methods can have no arguments");
                return Tuple{};
            }
        };

        template <typename Cls, typename First, typename... Rest>
        struct ArgParser<Cls, std::tuple<First, Rest...>> {
            using Tuple = std::tuple<First, Rest...>;

            static constexpr bool InstanceMethod = !std::is_void_v<Cls>;
            static constexpr bool ClassMethod    = !InstanceMethod && std::is_same_v<First, Class>;
            static constexpr bool ModuleMethod   = !InstanceMethod && std::is_same_v<First, Module>;
            static constexpr bool StaticMethod   = !InstanceMethod && std::is_same_v<First, Static>;

            // Arity differs depending on whether we wrap an instance method:
            //
            //   * For instance methods, the implicit this is the first parameter.
            //     This means the arity is exactly the number of function arguments.
            //
            //   * For everything else, the first parameter has to be either Module,
            //     Class, or Static. Those are given as the PyObject *self pointer
            //     and have special handling. Therefore, we only want to parse Rest.
            static constexpr std::size_t Arity = (1 + sizeof...(Rest)) - !InstanceMethod;

            static Tuple Parse(PyObject *self, PyObject *const *args, Py_ssize_t nargs) {
                Tuple res;

                if (nargs != Arity) {
                    PyErr_Format(PyExc_TypeError, "Expected %zu arguments, got %zu instead", Arity, nargs);
                    return res;
                }

                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    if constexpr (InstanceMethod) {
                        (FromPython(&std::get<Is>(res), args[Is]) && ...);
                    } else {
                        FromPython(&std::get<0>(res), self);
                        (FromPython(&std::get<Is + 1>(res), args[Is]) && ...);
                    }
                }(std::make_index_sequence<Arity>{});

                return res;
            }
        };

        // With argument parsing out of the way, we need some machinery
        // to wrap a C++ function into the canonical Python signature.
        //
        // There's actually three different kinds of signatures we must
        // respect for optimal call performance. But we can implement
        // those with the same underlying machinery so not a big deal.

        template <auto Fn>
        struct Method {
            using Traits = FunctionTraits<decltype(Fn)>;
            using Cls    = typename Traits::ClassType;
            using Args   = typename Traits::ArgsType;
            using Ret    = typename Traits::RetType;
            using Parser = ArgParser<Cls, Args>;

            static constexpr int Flags = [] {
                int flags = 0;

                // Select the optimal call ABI to use.
                if constexpr (Parser::Arity == 0) {
                    flags |= METH_NOARGS;
                } else if constexpr (Parser::Arity == 1) {
                    flags |= METH_O;
                } else {
                    flags |= METH_FASTCALL;
                }

                // Validate the function type properly.
                if constexpr (Parser::ClassMethod) {
                    flags |= METH_CLASS;
                } else if constexpr (Parser::StaticMethod) {
                    flags |= METH_STATIC;
                } else {
                    static_assert(Parser::InstanceMethod || Parser::ModuleMethod,
                                  "Failed to determine the desired method type");
                }

                return flags;
            }();

        private:
            static decltype(auto) CallFreeFunctionImpl(auto &&args) {
                return std::apply(Fn, std::forward<decltype(args)>(args));
            }

            static decltype(auto) CallMemberFunctionImpl(PyObject *self, auto &&args) {
                auto Wrapper = [self](auto &&...unpacked_args) -> decltype(auto) {
                    ObjectRef<Cls> obj{self};
                    return ((*obj).*Fn)(std::forward<decltype(unpacked_args)>(unpacked_args)...);
                };

                return std::apply(Wrapper, std::forward<decltype(args)>(args));
            }

            // This is the backbone to wrap a C++ call and return its
            // result as a Python value.
            static PyObject *CallImpl(PyObject *self, auto &&args) {
                if constexpr (std::is_void_v<Ret>) {
                    if constexpr (Parser::InstanceMethod) {
                        CallMemberFunctionImpl(self, std::move(args));
                    } else {
                        CallFreeFunctionImpl(std::move(args));
                    }

                    if (PyErr_Occurred() != nullptr) {
                        return nullptr;
                    }

                    Py_RETURN_NONE;
                } else {
                    Ret ret;
                    if constexpr (Parser::InstanceMethod) {
                        ret = CallMemberFunctionImpl(self, std::move(args));
                    } else {
                        ret = CallFreeFunctionImpl(std::move(args));
                    }

                    if (PyErr_Occurred() != nullptr) {
                        return nullptr;
                    }

                    return ToPython(std::move(ret));
                }
            }

            // Now we implement the 3 types of call ABIs mentioned
            // earlier based on this scaffolding.

            static PyObject *Fastcall(PyObject *self, PyObject *const *args, Py_ssize_t nargsf) {
                auto cpp_args = Parser::Parse(self, args, PyVectorcall_NARGS(nargsf));
                if (PyErr_Occurred() != nullptr) {
                    return nullptr;
                }

                return CallImpl(self, std::move(cpp_args));
            }

            static PyObject *Fastcall1(PyObject *self, PyObject *arg) {
                PyObject *args[1] = {arg};
                return Fastcall(self, args, 1);
            }

            static PyObject *Fastcall0(PyObject *self, PyObject *Py_UNUSED(arg)) { return Fastcall(self, nullptr, 0); }

        public:
            // Lastly a public function to select the best function
            // pointer out of the three options.
            static PyCFunction GetCallback() {
                if constexpr (Parser::Arity == 0) {
                    return reinterpret_cast<PyCFunction>(&Fastcall0);
                } else if constexpr (Parser::Arity == 1) {
                    return reinterpret_cast<PyCFunction>(&Fastcall1);
                } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
                    return reinterpret_cast<PyCFunction>(&Fastcall);
#pragma GCC diagnostic pop
                }
            }
        };

    }  // namespace impl

    /// Creates a new Python method wrapping a given C++ function.
    /// Argument and return value conversion is done internally.
    template <auto Fn>
    PyMethodDef Method(const char *name, const char *doc = nullptr) {
        using Helper = impl::Method<Fn>;
        return {name, Helper::GetCallback(), Helper::Flags, doc};
    }

    /// Creates a table of methods suitable for use with module
    /// or type object creation.
    template <typename... Ms>
        requires(std::is_same_v<Ms, PyMethodDef> && ...)
    constexpr std::array<PyMethodDef, sizeof...(Ms) + 1> MethodTable(Ms... methods) {
        return {{methods..., {nullptr, nullptr, 0, nullptr}}};
    }

}  // namespace boros::python
