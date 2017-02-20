import entityx
from entityx_python_test import Position


class EntityA(entityx.Entity):
    position = entityx.Component(Position, 1, 2)

    def __init__(self, x=None, y=None, entity=None):
        super(EntityA, self).__init__(entity=entity)
        if x is not None:
            self.position.x = x
        if y is not None:
            self.position.y = y


def create_entities_from_python_test():
    a = EntityA()
    assert a.id.index == 0
    assert a.position.x == 1.0
    assert a.position.y == 2.0

    b = EntityA()
    assert b.id.index == 1

    a.destroy()
    c = EntityA()
    # Reuse destroyed index of "a".
    assert c.id.index == 0
    # However, version is different
    assert a.id.id != c.id.id and c.id.id > a.id.id

    d = EntityA(2.0, 3.0)
    assert d.position.x == 2.0
    assert d.position.y == 3.0
