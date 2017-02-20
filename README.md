# Python Bindings using pybind11 for [EntityX](https://github.com/alecthomas/entityx) (α Alpha)

This system adds the ability to extend entity logic with Python scripts. The goal is to allow ad-hoc behaviour to be assigned to entities through scripts, in contrast to the more strictly pure entity-component system approach.

## Example

```python
from entityx import Entity, Component, emit
from mygame import Position, Health, Dead


class Player(Entity):
    position = Component(Position, 0, 0)
    health = Component(Health, 100)

    def on_collision(self, event):
        self.health.health -= 10
        if self.health.health <= 0:
            self.dead = Component(Dead, self)

```

## Building and installing

EntityX Pybind11 has the following build and runtime requirements:

- [EntityX](https://github.com/alecthomas/entityx)
- [CMake](http://cmake.org/)
- A C++11 Compliant Compiler or Visual Stuido Update 3

### CMake Options:

- `ENTITYX_PYTHON_BUILD_TESTING` : Enable building of tests
- `ENTITYX_ROOT` : Set path to EntityX root if CMake did not find it
- `PYTHON_ROOT` : Set path to Python root if CMake did not find it

Check out the source to entityx_pybind11, and run:

```bash
mkdir build && cd build
cmake  ..
make
make install
```

## Design

- Python scripts are attached to entities via `PythonScript`.
- Systems and components can not be created from Python, primarily for performance reasons.
- ~~Events are proxied directly to Python entities via `PythonEventProxy` objects.~~ (Removed for now to keep it simple)
- `PythonSystem` manages scripted entity lifecycle and event delivery.

## Summary

To add scripting support to your system, something like the following steps should be followed:

1. Expose C++ `Component` and `Event` classes to Python with `PYBIND_PLUGIN`.
2. Initialize the module with `pybind11_init()`.
3. Create a Python package.
4. Add classes to the package, inheriting from `entityx.Entity` and using the `entityx.Component` descriptor to assign components.
5. Create a `PythonSystem`, passing in the list of paths to add to Python's import search path.
6. Optionally attach any event proxies.
7. Create an entity and associate it with a Python script by assigning `PythonScript`, passing it the package name, class name, and any constructor arguments.
8. When finished, call `EntityManager::destroy_all()`.

## Interfacing with Python

`entityx::python` primarily uses standard `pybind11` to interface with Python, with some helper classes and functions.

### Exposing C++ Components to Python

In most cases, this should be pretty simple. Given a component, provide a `pybind11` class definition with two extra methods defined with EntityX::Python helper functions `assign_to<Component>` and `get_component<Component>`. These are used from Python to assign Python-created components to an entity and to retrieve existing components from an entity, respectively.

Here's an example:

```c++
namespace py = pybind11;
using namespace pybind11::literals;

struct Position : public entityx::Component<Position> {
  Position(float x = 0.0, float y = 0.0) : x(x), y(y) {}

  float x, y;
};

void export_position_to_python() {
  py::class_<Position>("Position")
    //Defines the initalizer to have default values for x and y
    .def(py::init<py::optional<float, float>>(), "x"_a = 0.f, "y"_a = 0.f)
    // Allows this component to be assigned to an entity
    .def("assign_to", &entityx::python::assign_to<Position>)
    // Allows this component to be retrieved from an entity.
    // Set return_value_policy to reference raw component pointer
    .def_static("get_component", &entityx::python::get_component<Position>,
         py::return_value_policy::reference)
    .def_readwrite("x", &Position::x)
    .def_readwrite("y", &Position::y);
}

// Namespace to avoid collision other pybind11_init();
namespace python {
PYBIND11_PLUGIN(mygame) {
  export_position_to_python();
} 
}
```

### Using C++ Components from Python

Use the `entityx.Component` class descriptor to associate components and provide default constructor arguments:

```python
import entityx
from mygame import Position  # C++ Component

class MyEntity(entityx.Entity):
    # Ensures MyEntity has an associated Position component,
    # constructed with the given arguments.
    position = entityx.Component(Position, 1, 2)

    def __init__(self, entity = None):
        # Overiding __init__ you must call super()'s init 
        # passing in entity
        super(MyEntity, self).__init__(entity=entity)
        assert self.position.x == 1
        assert self.position.y == 2
```

### Initialization

Finally, initialize the `mygame` module once, before using `PythonSystem`, with something like this:

```c++
// This should only be performed once, at application initialization time.
python::pybind11_init();
```

Then create a `PythonSystem` as necessary:

```c++
// Initialize the PythonSystem.
vector<string> paths;
// Ensure that MYGAME_PYTHON_PATH includes entityx.py from this distribution.
paths.push_back(MYGAME_PYTHON_PATH);
// +any other Python paths...
entityx::python::PythonSystem python(paths);
```
