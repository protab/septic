with check('renamed_func'):
    ok(renamed_func(1, 2))

with check('original_func', NameError):
    original_func

with check('renamed_class'):
    c = RenamedClass()
    ok(c.func())

with check('original_class', NameError):
    OriginalClass
