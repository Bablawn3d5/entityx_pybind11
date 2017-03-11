/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

 // http://docs.python.org/2/extending/extending.html
#include <pybind11/pybind11.h>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include "entityx/python/PythonScript.hpp"
#include "entityx/python/PythonSystem.h"
#include "entityx/python/config.h"

namespace py = pybind11;

namespace entityx {
namespace python {

class PythonEntityXLogger {
public:
  PythonEntityXLogger() {}
  explicit PythonEntityXLogger(PythonSystem::LoggerFunction logger) : logger_(logger) {}
  ~PythonEntityXLogger() { flush(true); }

  void write(const std::string &text) {
    line_ += text;
    flush();
  }

private:
  void flush(bool force = false) {
    size_t offset;
    while ( (offset = line_.find('\n')) != std::string::npos ) {
      std::string text = line_.substr(0, offset);
      logger_(text);
      line_ = line_.substr(offset + 1);
    }
    if ( force && line_.size() ) {
      logger_(line_);
      line_ = "";
    }
  }

  PythonSystem::LoggerFunction logger_;
  std::string line_;
};

/**
 * Base class for Python entities.
 */
	/*FIXME(SMA): Doesn't work :(. I've ported it to Python entityx/__init__.py 
struct PythonEntity {
  explicit PythonEntity(Entity& e) : _entity(e) {
    assert(_entity.valid());
  }
  virtual ~PythonEntity() {}
  explicit PythonEntity(const PythonEntity &) = delete;

  operator Entity () const { return _entity; }

  void destroy() {
    _entity.destroy();
  }

  virtual void update(float dt) {}

  Entity::Id _entity_id() const {
    assert(_entity.valid());
    return _entity.id();
  }

  bool valid() const {
    return _entity.valid();
  }

  Entity _entity;
};/**/

static std::string Entity_Id_repr(Entity::Id id) {
  std::stringstream repr;
  repr << "<Entity::Id " << id.index() << "." << id.version() << ">";
  return repr.str();
}

Entity EntityManager_new_entity(EntityManager& entity_manager, py::object self) {
  Entity entity = entity_manager.create();
  entity.assign<PythonScript>(self);
  return entity;
}
namespace _py_entityx {
PYBIND11_PLUGIN(_entityx) {
  py::module m("_entityx");

  py::class_<PythonEntityXLogger>(m, "Logger") // no init
    .def("write", &PythonEntityXLogger::write);

  py::class_<Entity>(m, "_Entity")
    .def(py::init<EntityManager*, Entity::Id>())
    .def_property_readonly("id", &Entity::id)
    .def("valid", &Entity::valid)
    .def("destroy", &Entity::destroy);

	//FIXME(SMA): I really have no clue none of these are working...
  //py::class_<PythonEntity>(m, "Entity")
    //.def_readwrite("entity", &PythonEntity::_entity, py::return_value_policy::reference);
    //.def_property_readonly("id", &PythonEntity::_entity_id)
    //.def("valid", &PythonEntity::valid)
    //.def("update", &PythonEntity::update)
    //.def("destroy", &PythonEntity::destroy);

  //py::implicitly_convertible<PythonEntity, Entity>();

  py::class_<Entity::Id>(m, "EntityId")  // no init
    .def_property_readonly("id", &Entity::Id::id)
    .def_property_readonly("index", &Entity::Id::index)
    .def_property_readonly("version", &Entity::Id::version)
    .def("__repr__", &Entity_Id_repr);

  py::class_<PythonScript>(m, "PythonScript")
    .def(py::init<py::object>())
    .def("assign_to", &assign_to<PythonScript>)
    .def_static("get_component", &get_component<PythonScript>,
                py::return_value_policy::reference);

  py::class_<EntityManager>(m, "EntityManager") // no init
    .def("new_entity", &EntityManager_new_entity, py::return_value_policy::copy);

  return m.ptr();
}
} // namespace _py_entityx

static void log_to_stderr(const std::string &text) {
  std::cerr << "python stderr: " << text << std::endl;
}

static void log_to_stdout(const std::string &text) {
  std::cout << "python stdout: " << text << std::endl;
}

// PythonSystem below here

bool PythonSystem::initialized_ = false;

PythonSystem::PythonSystem(EntityManager& entity_manager)
  : em_(entity_manager), stdout_(log_to_stdout), stderr_(log_to_stderr) {
  Py_Initialize();
  if ( !initialized_ ) {
    initialize_python_module();
    initialized_ = true;
  }
}


PythonSystem::~PythonSystem() {
  // TODO(SMA): Look into cleaning up our module.
  //try {
  //  py::object entityx = py::module::import("_entityx");
  //  entityx.attr("_entity_manager");
  //  entityx.attr("_event_manager").del();
  //  py::object sys = py::module::import("sys");
  //  sys.attr("stdout").del();
  //  sys.attr("stderr").del();
  //  py::object gc = py::module::import("gc");
  //  gc.attr("collect")();
  //}
  //catch ( ... ) {
  //  PyErr_Print();
  //  PyErr_Clear();
  //  throw;
  //}
  // FIXME: It would be good to do this, but it is not supported by boost::python:
  // http://www.boost.org/doc/libs/1_53_0/libs/python/todo.html#pyfinalize-safety
  // Py_Finalize();
}

void PythonSystem::add_installed_library_path() {
  add_path(ENTITYX_INSTALLED_PYTHON_PACKAGE_DIR);
}

void PythonSystem::add_path(const std::string &path) {
  python_paths_.push_back(path);
}

void PythonSystem::initialize_python_module() {
  _py_entityx::pybind11_init();
}

void PythonSystem::configure(EventManager& ev) {
  ev.subscribe<ComponentAddedEvent<PythonScript>>(*this);

  try {
    py::object main_module = py::module::import("__main__");
    py::object main_namespace = main_module.attr("__dict__");

    // Initialize logging.
    py::object sys = py::module::import("sys");
    sys.attr("stdout") = PythonEntityXLogger(stdout_);
    sys.attr("stderr") = PythonEntityXLogger(stderr_);

    // Add paths to interpreter sys.path
    for ( auto path : python_paths_ ) {
      py::str dir(path.c_str());
      sys.attr("path").attr("insert")(0, dir);
    }

    py::object entityx = py::module::import("_entityx");
    entityx.attr("_entity_manager") = &em_;
  }
  catch ( const py::error_already_set& e ) {
    // TODO(SMA) : Really!? fix this. Should handle execption e better here.
    PyErr_SetString(PyExc_RuntimeError, e.what());
    PyErr_Print();
    PyErr_Clear();
    throw;
  }
}

void PythonSystem::update(EntityManager & em,
                          EventManager & events, TimeDelta dt) {
  em.each<PythonScript>(
    [=](Entity entity, PythonScript& python) {
    try {
      // Access PythonEntity and call Update.
      python.object.attr("update")(dt);
    }
    catch ( const py::error_already_set& e ) {
      // TODO(SMA) : Really!? fix this. Should handle execption e better here.
      PyErr_SetString(PyExc_RuntimeError, e.what());
      PyErr_Print();
      PyErr_Clear();
      throw;
    }
  });
}

void PythonSystem::log_to(LoggerFunction sout, LoggerFunction serr) {
  stdout_ = sout;
  stderr_ = serr;
}

void PythonSystem::receive(const ComponentAddedEvent<PythonScript> &event) {
  // If the component was created in C++ it won't have a Python object
  // associated with it. Create one.
  if ( !event.component->object ) {
    py::object module = py::module::import(event.component->module.c_str());
    py::object cls = module.attr(event.component->cls.c_str());
    py::object from_raw_entity = cls.attr("_from_raw_entity");
    py::list args;
    if ( py::len(event.component->args) != 0 ) {
      args.append(event.component->args);
    }
    py::dict kwargs;
    kwargs["entity"] = &event.entity;
    ComponentHandle<PythonScript> p = event.component;
    try {
      // Access PythonEntity and call Update.
      p->object = from_raw_entity(*args, **py::kwargs(kwargs));
    }
    catch ( const py::error_already_set& e ) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      PyErr_Print();
      PyErr_Clear();
      throw;
    }
  }
}

}  // namespace python
}  // namespace entityx
