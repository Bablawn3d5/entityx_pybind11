from entityx import Entity, Component
from entityx_python_test import Position


class AssignTest(Entity):
    position = Component(Position)
    def __init__(self):
        assert self.entity.id.index == 0
        assert self.entity.id.version == 1

    def test_assign_create(self):
        assert self.entity.id.index == 0
        assert self.entity.id.version == 1
        assert self.position.x == 0.0, self.position.x
        assert self.position.y == 0.0, self.position.y
        self.position.x = 1
        self.position.y = 2

    def test_assign_existing(self):
        assert self.entity.id.index == 0
        assert self.entity.id.version == 1
        assert self.position.x == 2, self.position.x
        assert self.position.y == 3, self.position.y
        self.position.x += 1
        self.position.y += 1
