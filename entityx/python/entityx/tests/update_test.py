import entityx


class UpdateTest(entityx.Entity):
    updated = False
    def update(self, dt):
    	# Should only be called with one entity.
    	assert self.entity.id.index == 0
    	assert self.entity.id.version == 1
        self.updated = True
