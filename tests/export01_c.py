with check('func_noexport', NameError):
    func_noexport()

with check('func_noargs'):
    ok(func_noargs())

with check('func_noargs with arg', TypeError):
    func_noargs(1)

with check('func_arg1'):
    ok(func_arg1(1))

with check('func_arg1 with no args', TypeError):
    func_arg1()

with check('func_arg1 with two args', TypeError):
    func_arg1(1, 2)

with check('func_narg'):
    ok(func_narg(a=1))

with check('func_narg with bad arg', TypeError):
    func_narg(b=1)

with check('func_varg'):
    ok(func_varg(1, b=2))
    ok(func_varg(1, b=2, c=3), 'fail')
    ok(func_varg(2), 'fail')
    ok(func_varg(1, 2), 'fail')
    ok(func_varg(b=1), 'fail')
