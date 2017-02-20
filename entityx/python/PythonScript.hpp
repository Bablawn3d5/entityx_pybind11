// Copyright 2017 Bablawn3d5

#pragma once

#include <pybind11/pybind11.h>
#include <entityx/entityx.h>
#include <string>

namespace py = pybind11;

namespace entityx {
namespace python {

/**
 * An EntityX component that represents a Python script.
 */
struct PythonScript {
    // Copy Constructor
    // TODO(SMA) : Initalize module and cls from given object.
    explicit PythonScript(py::object object) : object(object) {}
    /**
    * Create a new PythonScript from a Python Entity class.
    *
    * @param module The Python module where the Entity subclass resides.
    * @param cls The Class within the module. Must inherit from entityx.Entity.
    * @param args The *args to pass to the Python constructor.
    */
    explicit PythonScript(const std::string &module = "", const std::string &cls = "") :
      module(module), cls(cls) {}

    template <typename ...Args>
    explicit PythonScript(const std::string &module, const std::string &cls, Args ... args) :
        module(module), cls(cls) {
        unpack_args(args...);
    }

    ~PythonScript() {}

    py::object object;
    py::list args;
    // HACK(SMA): This should be const but we need
    // a copy constructable object.
    // const std::string module, cls;
    std::string module, cls;

    template <typename A, typename ...Args>
    void unpack_args(A &arg, Args ... remainder) { // NOLINT
        args.append(arg);
        unpack_args(remainder...);
    }

    void unpack_args() {}
};

} // python
} // entityx