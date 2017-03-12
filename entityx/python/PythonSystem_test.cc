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
#include <pybind11/stl.h>
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
using namespace pybind11::literals;

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
    .def_static("get_component", &get_component<Direction>,
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
    e.assign<Direction>(2.f, 3.f);
    auto script = e.assign<PythonScript>("entityx.tests.assign_test", "AssignTest");
    REQUIRE(static_cast<bool>(e.component<Direction>()));
    REQUIRE(script->object);
    REQUIRE(script->object.attr("test_assign_existing").ptr());
    script->object.attr("test_assign_existing")();
    auto position = e.component<Direction>();
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

TEST_CASE_METHOD(PythonSystemTest, "TestJSONOutputCpp") {
    try {
        Entity e = entity_manager.create();
        e.assign<Position>(2.f, 4.f);
        e.assign<Direction>(1.f, -1.f);
        auto script = e.assign<PythonScript>("entityx.tests.json_test", "JsonTest");
        REQUIRE(static_cast<bool>(e.component<Position>()));
        REQUIRE(static_cast<bool>(e.component<Direction>()));
        REQUIRE(script->object);
        REQUIRE(script->object.attr("to_json").ptr());
        py::object o = script->object.attr("to_json")();
        auto position = e.component<Position>();
        REQUIRE(static_cast<bool>(position));
        REQUIRE(position->x == 2.0);
        REQUIRE(position->y == 4.0);
        auto direction = e.component<Direction>();
        REQUIRE(static_cast<bool>(direction));
        REQUIRE(direction->x == 1.0);
        REQUIRE(direction->y == -1.0);
        py::module m("json");
        py::dict parsed = m.attr("loads").call(o);
        // Incase you need to debug.
        //for ( auto item : parsed )
        //    py::print("key: {}, value={}"_s.format(item.first, item.second));

        REQUIRE(py::cast<std::string>(parsed["entity"]["id"]) == "<Entity::Id 0.1>");
        REQUIRE(py::cast<std::string>(parsed["id"]) == "<Entity::Id 0.1>");
        REQUIRE(py::cast<std::string>(parsed["py_array"]) == "[1, 2, 3]");
        py::dict py_pos = parsed["position"];
        REQUIRE((float)py::float_(py_pos["x"]) == 2.0f);
        REQUIRE((float)py::float_(py_pos["y"]) == 4.0f);
        py::dict py_dir = parsed["direction"];
        REQUIRE((float)py::float_(py_dir["x"]) == 1.0f);
        REQUIRE((float)py::float_(py_dir["y"]) == -1.0f);
    }
    catch ( py::error_already_set& e ) {
        // TODO(SMA) : Really!? fix this. Should handle execption e better here.
        PyErr_SetString(PyExc_RuntimeError, e.what());
        PyErr_Print();
        PyErr_Clear();
        REQUIRE(false);
    }
}