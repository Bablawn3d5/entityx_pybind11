from entityx import Entity, Component
from entityx_python_test import Position, Direction


class AssignTest(Entity):
    position = Component(Position)
    def __init__(self):
        assert self.entity.id.index == 0
        assert self.entity.id.version == 1

    def test_assign_create(self):
        assert self.entity.id.index == 0
        assert self.entity.id.version == 1
        assert self.position.x == 0.0, ("%d != 0.0") % self.position.x
        assert self.position.y == 0.0, ("%d != 0.0") % self.position.y
        self.position.x = 1
        self.position.y = 2

    def test_assign_existing(self):
        direction = self.Component(Direction)
        assert self.entity.id.index == 0
        assert self.entity.id.version == 1
        assert direction.x == 2, ("%d != 2") % direction.x 
        assert direction.y == 3, ("%d != 3") % direction.y
        direction.x += 1
        direction.y += 1
