class ClassNoExport:
    def func(self):
        return 'fail'

@export
class ClassClosed:
    def func(self):
        return 'fail'

@export
class ClassMethod:
    @export
    def func(self):
        return 'ok'

@export
class ClassInst:
    @export
    def __init__(self):
        pass

    @export
    def func(self):
        return 'ok'

@export
class ClassParams:
    @export
    def __init__(self, a):
        self.a = a

    @export
    def func(self, a):
        if a == self.a:
            return 'ok'
        return 'fail'
