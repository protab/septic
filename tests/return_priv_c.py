with check('class_no_export', AttributeError):
    c = accessor_no_export()
    c.func()

with check('class_method'):
    c = accessor_method()
    ok(c.func())
