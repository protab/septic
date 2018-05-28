class ClassNoExport:
    def func(self):
        return 'fail'

class ClassMethod:
    @export
    def func(self):
        return 'ok'

@export
def accessor_no_export():
    return ClassNoExport()

@export
def accessor_method():
    return ClassMethod()
