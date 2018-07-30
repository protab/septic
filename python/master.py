import importlib
import io
import json
import os
import os.path
import selectors
import sys
import time
from wrapper import XferCodec, CodecError, XferConnection
from exporter import export, Export

def is_hashable(obj):
    try:
        hash(obj)
    except TypeError:
        return False
    return True

class XferDict:
    MAX_OBJECTS = 4096

    def __init__(self):
        self.objs_by_id = {}
        self.objs_by_obj = {}
        self.last_id = 0

    def get_or_add_obj(self, obj):
        hashable = is_hashable(obj)
        if hashable and obj in self.objs_by_obj:
            return self.objs_by_obj[obj]
        if len(self.objs_by_id) >= self.MAX_OBJECTS:
            raise CodecError('Too many objects allocated')
        self.last_id += 1
        if hashable:
            self.objs_by_obj[obj] = self.last_id
        self.objs_by_id[self.last_id] = obj
        return self.last_id

    def __getitem__(self, key):
        return self.objs_by_id[key]

    def __delitem__(self, key):
        try:
            obj = self.objs_by_id[key]
            del self.objs_by_id[key]
            if is_hashable(obj):
                del self.objs_by_obj[obj]
        except KeyError:
            pass

class XferMasterCodec(XferCodec):
    def __init__(self):
        self.remotes = XferDict()

    def encode_obj(self, data):
        return self.remotes.get_or_add_obj(data)

    def decode_obj(self, data):
        return self.remotes[data]

    def del_obj(self, data):
        del self.remotes[data]

class PassThruException(Exception):
    pass

class XferMasterConnection(XferConnection):
    LIMIT_OUT = 65536

    def __init__(self):
        os.set_blocking(3, False)
        super().__init__(XferMasterCodec())
        self.limit_out = self.LIMIT_OUT
        self.out = open(os.path.join(meta, 'output'), 'w')

    def install(self, nobjs):
        self.send('install', nobjs)

    def format_exception(self, e):
        return ('exception', {
            'exception': e.__class__.__name__,
            'args': e.args,
        })

    def loop(self):
        sel = selectors.DefaultSelector()
        sel.register(self.pipe_in, selectors.EVENT_READ)
        while True:
            again = True
            for key, events in sel.select():
                if key.fileobj == self.pipe_in:
                    t, data = self.rcv()
                    again = False
            if again:
                continue
            if t is None:
                # EOF
                return

            try:
                t, data = self.process(t, data)
            except PassThruException as e:
                raise e.args[0] from None
            except Exception as e:
                t, data = self.format_exception(e)
            self.send(t, data)

    def process_print(self, msg):
        chars = len(msg)
        if chars > self.limit_out:
            msg = msg[:self.limit_out]
            chars = self.limit_out
        self.out.write(msg)
        self.out.flush()
        self.limit_out -= chars

        return None

    def process_input(self, msg):
        tmp_name = os.path.join(meta, 'input.tsk')
        in_name = os.path.join(meta, 'input')
        f = open(tmp_name, 'w')
        f.write(msg)
        f.close()
        os.rename(tmp_name, in_name)

        in_name = os.path.join(meta, 'input.ret')
        while not os.path.exists(in_name):
            time.sleep(1)

        f = open(in_name, 'r')
        data = f.read()
        f.close()
        os.unlink(in_name)

        return data

    def check_access(self, obj, name, msg=None):
        # for C (builtin) objects exported with large=True, we need to allow
        # access unconditionally, as we can't set attributes:
        if type(obj) in (tuple, list, dict, type(iter([])), type(iter({})), type(iter(()))):
            return

        f = getattr(obj, name)
        if getattr(f, 'exported_method', False):
            return
        if msg:
            raise AttributeError(msg)
        if type(obj) == type:
            raise AttributeError("type object '{}' has no attribute '{}'".format(obj.__name__, name))
        raise AttributeError("'{}' object has no attribute '{}'".format(type(obj).__name__, name))

    def process_get(self, obj, name):
        self.check_access(obj, name)
        return getattr(obj, name)

    def process_call(self, obj, name, args, kwargs):
        if name == '__call__':
            # special case
            if type(obj) == type:
                # it's a class; check whether we can make an instance
                self.check_access(obj, '__init__', "type object '{}' cannot be instantiated".format(obj.__name__))
            return obj(*args, **kwargs)
        self.check_access(obj, name)
        return getattr(obj, name)(*args, **kwargs)

    def process(self, t, data):
        if t == 'print':
            msg = data['message']
            try:
                answer = self.process_print(msg)
            except Exception as e:
                raise PassThruException(e)

        elif t == 'input':
            msg = data['message']
            try:
                answer = self.process_input(msg)
            except Exception as e:
                raise PassThruException(e)

        elif t == 'get':
            obj = data['obj']
            name = data['name']
            answer = self.process_get(obj, name)

        elif t == 'del':
            obj = data['obj']
            try:
                self.codec.del_obj(obj)
            except Exception as e:
                raise PassThruException(e)
            answer = None

        elif t == 'call':
            obj = data['obj']
            name = data['name']
            args = data.get('args', ())
            kwargs = data.get('kwargs', {})
            answer = self.process_call(obj, name, args, kwargs)

        else:
            raise ValueError('Unknown type {}'.format(t))

        return ('ok', answer)

def uprint(*args, sep=' ', end='\n'):
    conn.process_print(sep.join((str(s) for s in args)) + end)

sys.path.insert(1, sys.argv[1])
meta = sys.argv[2]
mod_name = sys.argv[3]

spec = importlib.util.find_spec(mod_name)
if not spec:
    raise ImportError("No module named '{}'".format(mod_name))
task = importlib.util.module_from_spec(spec)
task.__dict__['export'] = export
task.__dict__['uprint'] = uprint
spec.loader.exec_module(task)

conn = XferMasterConnection()
conn.install(Export.exported)
conn.loop()
