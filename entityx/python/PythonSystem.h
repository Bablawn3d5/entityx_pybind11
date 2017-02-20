/*
 * Copyright (C) 2013 Alec Thomas <alec@swapoff.org>
 * All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.
 *
 * Author: Alec Thomas <alec@swapoff.org>
 */

#pragma once

 // http://docs.python.org/2/extending/extending.html
#include <pybind11/pybind11.h>
#include <list>
#include <vector>
#include <string>
#include "entityx/System.h"
#include "entityx/Entity.h"
#include "entityx/Event.h"

namespace py = pybind11;

namespace entityx {
namespace python {
/**
 * An EntityX component that represents a Python script.
 */
class PythonScript {
public:
  /**
   * Create a new PythonScript from a Python Entity class.
   *
   * @param module The Python module where the Entity subclass resides.
   * @param cls The Class within the module. Must inherit from entityx.Entity.
   * @param args The *args to pass to the Python constructor.
   */
  template <typename ...Args>
  PythonScript(const std::string &module, const std::string &cls, Args ... args) : module(module), cls(cls) {
    unpack_args(args...);
  }

  /**
   * Create a new PythonScript from an existing Python instance.
   */
  explicit PythonScript(py::object object) : object(object) {}

  py::object object;
  py::list args;
  const std::string module, cls;

private:
  template <typename A, typename ...Args>
  void unpack_args(A &arg, Args ... remainder) {
    args.append(arg);
    unpack_args(remainder...);
  }

  void unpack_args() {}
};

class PythonSystem;

/**
 * A helper function for class_ to assign a component to an entity.
 */
template <typename Component>
void assign_to(Component& component, EntityManager& entity_manager, Entity::Id id) {
  entity_manager.assign<Component>(id, component);
}

/**
 * A helper function for retrieving an existing component associated with an
 * entity.
 */
template <typename Component>
static Component* get_component(EntityManager& em, Entity::Id id) {
  auto handle = em.component<Component>(id);
  if ( !handle )
    return NULL;
  return handle.get();
}

/**
 * An entityx::System that bridges EntityX and Python.
 *
 * This system handles exposing entityx functionality to Python. The Python
 * support differs in design from the C++ design in the following ways:
 *
 * - Entities contain logic
 * - Systems and Components can not be implemented in Python.
 */
class PythonSystem : public entityx::System<PythonSystem>, public entityx::Receiver<PythonSystem> {
public:
  typedef std::function<void(const std::string &)> LoggerFunction;

  PythonSystem(EntityManager& entity_manager);  // NOLINT
  virtual ~PythonSystem();

  /**
   * Add system-installed entityx Python path to the interpreter.
   */
  void add_installed_library_path();

  /**
   * Add a Python path to the interpreter.
   */
  void add_path(const std::string &path);

  /**
   * Add a sequence of paths to the interpreter.
   */
  template <typename T>
  void add_paths(const T &paths) {
    for ( auto path : paths ) {
      add_path(path);
    }
  }

  /// Return the Python paths the system is configured with.
  const std::vector<std::string> &python_paths() const {
    return python_paths_;
  }

  virtual void configure(EventManager& event_manager) override;
  virtual void update(EntityManager& entities, EventManager& event_manager, TimeDelta dt) override;

  /**
   * Set line-based (not including \n) logger for stdout and stderr.
   */
  void log_to(LoggerFunction sout, LoggerFunction serr);

  void receive(const ComponentAddedEvent<PythonScript> &event);

private:
  void initialize_python_module();

  EntityManager& em_;
  std::vector<std::string> python_paths_;
  LoggerFunction stdout_, stderr_;
  static bool initialized_;
};
}  // namespace python
}  // namespace entityx
