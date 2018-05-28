class ClassNoExport:
    def func(self):
        return 'fail'

class ClassExport:
    @export
    def func(self):
        return 'ok'

export('class_no_export', ClassNoExport())
export('class_export', ClassExport())
