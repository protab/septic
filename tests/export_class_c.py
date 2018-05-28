with check('class_no_export', NameError):
    ClassNoExport

with check('class_closed', AttributeError):
    ClassClosed.func(None)

with check('class_closed object', AttributeError):
    c = ClassClosed()

with check('class_method'):
    ok(ClassMethod.func(None))

with check('class_method object', AttributeError):
    c = ClassMethod()

with check('class_inst'):
    c = ClassInst()
    ok(c.func())

with check('class_params'):
    c = ClassParams(1)
    ok(c.func(1))
