from entityx import Entity, Component
from entityx_python_test import Position, Direction


class JsonTest(Entity):
    position = Component(Position)
    direction = Component(Direction)
    py_array = [1,2,3]