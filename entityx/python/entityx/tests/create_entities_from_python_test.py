import entityx
from entityx_python_test import Position


class EntityA(entityx.Entity):
    position = entityx.Component(Position, 1, 2)

    def __init__(self, x=None, y=None):
        if x is not None:
            self.position.x = x
        if y is not None:
            self.position.y = y

class EntityB(entityx.Entity):
    pass

def dump(obj):
  for attr in dir(obj):
    print "obj.%s = %s" % (attr, getattr(obj, attr))

def create_entities_from_python_test():
    a = EntityA()
    assert a.id.index == 0
    assert a.entity.id.index == a.id.index
    assert a.entity.id.version == a.id.version
    assert a.position.x == 1.0
    assert a.position.y == 2.0
    assert a.HasComponent(Position) == True

    b = EntityA()
    assert b.id.index == 1
    assert b.entity.id.index == b.id.index
    assert b.HasComponent(Position) == True
    assert b.entity.id != a.entity.id

    a.destroy()
    # Assert B still exists.
    assert b.id.index == 1

    c = EntityA()
    # Reuse destroyed index of "a".
    assert c.entity.id.index == c.id.index
    assert c.id.index == 0
    # However, version is different
    assert a.id.id != c.id.id and c.id.id > a.id.id

    d = EntityA(2.0, 3.0)
    assert d.position.x == 2.0
    assert d.position.y == 3.0

    # Check that created components
    # don't effect other components
    e = EntityB()
    assert e.HasComponent(Position) == False
    e.pos = e.Component(Position, 3,3)
    assert e.HasComponent(Position) == True
    assert e.pos.x == 3.0
    assert e.pos.y == 3.0
    assert d.position.x == 2.0
    assert d.position.y == 3.0