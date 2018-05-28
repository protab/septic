@export(name='renamed_func')
def original_func(a, b):
    return 'ok'

@export(name='RenamedClass')
class OriginalClass:
    @export
    def __init__(self):
        pass

    @export
    def func(self):
        return 'ok'

