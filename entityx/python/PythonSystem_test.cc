/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#define CATCH_CONFIG_MAIN

 // NOTE: MUST be first include. See http://docs.python.org/2/extending/extending.html
#include <pybind11/pybind11.h>
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include "entityx/python/3rdparty/catch.hpp"
#include "entityx/entityx.h"
#include "entityx/python/PythonScript.hpp"
#include "entityx/python/PythonSystem.h"

namespace py = pybind11;
using std::cerr;
using std::endl;
using namespace entityx;
using namespace entityx::python;

struct Position {
  Position(float x = 0.0, float y = 0.0) : x(x), y(y) {}

  float x, y;
};

struct Direction {
  Direction(float x = 0.0, float y = 0.0) : x(x), y(y) {}

  float x, y;
};

PYBIND11_PLUGIN(entityx_python_test) {
  using namespace pybind11::literals;
  py::module m("entityx_python_test");
  py::class_<Position>(m, "Position")
    .def(py::init<float, float>(), "x"_a = 0.f, "y"_a = 0.f)
    .def("assign_to", &assign_to<Position>)
    .def_static("get_component", &get_component<Position>,
         py::return_value_policy::reference)
    .def_readwrite("x", &Position::x)
    .def_readwrite("y", &Position::y);

  py::class_<Direction>(m, "Direction")
    .def(py::init<float, float>(), "x"_a = 0.f, "y"_a = 0.f)
    .def("assign_to", &assign_to<Direction>)
    .def_static("get_component", &get_component<Position>,
                py::return_value_policy::reference)
    .def_readwrite("x", &Direction::x)
    .def_readwrite("y", &Direction::y);
  return m.ptr();
}

class PythonSystemTest {
protected:
  PythonSystemTest() : python(entity_manager), entity_manager(event_manager) {
    Py_Initialize();
    python.add_path(ENTITYX_PYTHON_TEST_DATA);
    if ( !initialized ) {
      pybind11_init();
      initialized = true;
    }
    python.configure(event_manager);
  }

  PythonSystem python;
  EventManager event_manager;
  EntityManager entity_manager;
  static bool initialized;
};

bool PythonSystemTest::initialized = false;

TEST_CASE_METHOD(PythonSystemTest, "TestSystemUpdateCallsEntityUpdate") {
  try {
    Entity e = entity_manager.create();
    REQUIRE(entity_manager.size() == 1);
    auto script = e.assign<PythonScript>("entityx.tests.update_test", "UpdateTest");
    REQUIRE(entity_manager.size() == 1);
    REQUIRE(!py::cast<bool>(script->object.attr("updated")));
    python.update(entity_manager, event_manager, static_cast<TimeDelta>(0.1));
    REQUIRE(py::cast<bool>(script->object.attr("updated")));
    REQUIRE(entity_manager.size() == 1);
  }
  catch ( py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    REQUIRE(false);
  }
}

TEST_CASE_METHOD(PythonSystemTest, "TestComponentAssignmentCreationInPython") {
  try {
    Entity e = entity_manager.create();
    REQUIRE(entity_manager.size() == 1);
    auto script = e.assign<PythonScript>("entityx.tests.assign_test", "AssignTest");
    REQUIRE(entity_manager.size() == 1);
    REQUIRE(static_cast<bool>(e.component<Position>()));
    REQUIRE(script->object);
    REQUIRE(script->object.attr("test_assign_create").ptr());
    py::function f = script->object.attr("test_assign_create");
    f.call();
    auto position = e.component<Position>();
    REQUIRE(static_cast<bool>(position));
    REQUIRE(position->x == 1.0);
    REQUIRE(position->y == 2.0);
  }
  catch ( py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    REQUIRE(false);
  }
}

TEST_CASE_METHOD(PythonSystemTest, "TestComponentAssignmentCreationInCpp") {
  try {
    Entity e = entity_manager.create();
    e.assign<Position>(2.f, 3.f);
    auto script = e.assign<PythonScript>("entityx.tests.assign_test", "AssignTest");
    REQUIRE(static_cast<bool>(e.component<Position>()));
    REQUIRE(script->object);
    REQUIRE(script->object.attr("test_assign_existing").ptr());
    script->object.attr("test_assign_existing")();
    auto position = e.component<Position>();
    REQUIRE(static_cast<bool>(position));
    REQUIRE(position->x == 3.0);
    REQUIRE(position->y == 4.0);
  }
  catch ( py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    REQUIRE(false);
  }
}

TEST_CASE_METHOD(PythonSystemTest, "TestEntityConstructorArgs") {
  try {
    Entity e = entity_manager.create();
    e.assign<PythonScript>("entityx.tests.constructor_test", "ConstructorTest", 4.0, 5.0);
    auto position = e.component<Position>();
    REQUIRE(static_cast<bool>(position));
    REQUIRE(position->x == 4.0);
    REQUIRE(position->y == 5.0);
  }
  catch ( py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    REQUIRE(false);
  }
}

TEST_CASE_METHOD(PythonSystemTest, "TestDeepEntitySubclass") {
  try {
    Entity e = entity_manager.create();
    auto script = e.assign<PythonScript>("entityx.tests.deep_subclass_test", "DeepSubclassTest");
    REQUIRE(script->object.attr("test_deep_subclass").ptr());
    script->object.attr("test_deep_subclass")();

    Entity e2 = entity_manager.create();
    auto script2 = e2.assign<PythonScript>("entityx.tests.deep_subclass_test", "DeepSubclassTest2");
    REQUIRE(script2->object.attr("test_deeper_subclass").ptr());
    script2->object.attr("test_deeper_subclass")();
  }
  catch ( py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    REQUIRE(false);
  }
}

TEST_CASE_METHOD(PythonSystemTest, "TestEntityCreationFromPython") {
  try {
    py::object test = py::module::import("entityx.tests.create_entities_from_python_test");
    test.attr("create_entities_from_python_test")();
  }
  catch ( py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    REQUIRE(false);
  }
}
