with check('obj_export'):
    class_no_export
    class_export

with check('class_no_export', AttributeError):
    class_no_export.func()

with check('class_export'):
    ok(class_export.func())
