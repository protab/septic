import inspect
from wrapper import LargeObject

class ExportError(Exception):
    pass

class Export:
    exported = {}

    def __init__(self, wrapper, *args, **kwargs):
        self.wrapper = wrapper
        self.name = None
        if len(args) == 2 and type(args[0]) == str:
            # direct call to export an object:
            self.kind = 0
            self.name = args[0]
            self.add_exported(args[1])
        elif len(args) == 1 and callable(args[0]):
            # decorator without arguments
            self.kind = 1
            self.f = self.decorate(args[0])
        else:
            # decorator with arguments
            self.kind = 2
            if args:
                raise TypeError('export() takes 0 positional arguments')
        for k, v in kwargs.items():
            if k == 'name' and self.kind == 2:
                self.name = v
            elif k == 'large' and v and self.kind == 0:
                self.set_large()
            else:
                raise TypeError("export() got an unexpected keyword argument '{}'".format(k))

    def __call__(self, *args, **kwargs):
        if self.kind == 0:
            raise TypeError("'export' object is not callable")
        if self.kind == 1:
            return self.f(*args, **kwargs)
        return self.decorate(args[0])

    def add_exported(self, obj):
        if self.name is None:
            self.name = obj.__name__
        self.exported[self.name] = obj

    def set_large(self):
        self.exported[self.name] = LargeObject(self.exported[self.name])

    def decorate(self, obj):
        if inspect.isclass(obj):
            return self.decorate_class(obj)
        if inspect.isfunction(obj):
            if obj.__name__ != obj.__qualname__:
                return self.decorate_method(obj)
            return self.decorate_func(obj)
        raise ExportError('export decorator cannot be used on {}'.format(type(obj)))

    def decorate_func(self, f):
        self.add_exported(f)
        return f

    def decorate_class(self, cls):
        self.add_exported(cls)
        return cls

    def decorate_method(self, f):
        if f.__name__ == '__call__':
            raise ExportError('__call__ cannot be decorated')
        if self.name is not None:
            raise ExportError('methods cannot be exported under a different name')
        # The 'exported_method' must be set on the wrapper as it replaces
        # the original method by itself.
        self.wrapper.exported_method = True
        return f

def export(*args, **kwargs):
    def wrapper(*args, **kwarg):
        return e(*args, **kwargs)
    e = Export(wrapper, *args, **kwargs)
    return wrapper
