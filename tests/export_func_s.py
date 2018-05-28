def func_noexport():
    return 'fail'

@export
def func_noargs():
    return 'ok'

@export
def func_arg1(a):
    return 'ok'

@export
def func_arg2(a, b):
    return 'ok'

@export
def func_narg(a=None):
    if not a:
        return 'fail'
    return 'ok'

@export
def func_varg(*args, **kwargs):
    if len(args) != 1 or len(kwargs) != 1 or 'b' not in kwargs:
        return 'fail'
    return 'ok'
