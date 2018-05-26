import contextlib
import importlib.util
import io
import json
import os
import sys
import traceback
import weakref

class CodecError(Exception):
    pass

class XferCodec:
    max_depth = 10

    def r_encode(self, data, depth):
        if depth >= self.max_depth:
            raise CodecError('Too deep nesting')
        t = type(data)
        if t in (bool, int, float, str, type(None)):
            return { 't': '.', 'd': data }
        if t in (list, tuple):
            return { 't': 'list', 'd': [ self.r_encode(p, depth + 1) for p in data ] }
        if t == dict:
            return { 't': 'dict', 'd':
                [ (self.r_encode(k, depth + 1), self.r_encode(v, depth + 1)) for k, v in data.items() ] }
        return { 't': 'obj', 'd': self.encode_obj(data) }

    def encode(self, data):
        return self.r_encode(data, 0)

    def r_decode(self, data, depth):
        if depth >= self.max_depth:
            raise CodecError('Too deep nesting')
        if data['t'] == '.':
            return data['d']
        if data['t'] == 'list':
            return [ self.r_decode(p, depth + 1) for p in data['d'] ]
        if data['t'] == 'dict':
            return { self.r_decode(k, depth + 1): self.r_decode(v, depth + 1) for k, v in data['d'] }
        if data['t'] == 'obj':
            return self.decode_obj(data['d'])
        raise CodecError('Unknown type')

    def decode(self, data):
        return self.r_decode(data, 0)

class XferClientCodec(XferCodec):
    def __init__(self):
        self.objs = RemoteCache()

    def encode_obj(self, data):
        if not isinstance(data, RemoteObject):
            raise CodecError('Unsupported type')
        return data.remote_id

    def decode_obj(self, data):
        return self.objs.get_or_add(data)

class RemoteObject:
    def __init__(self, remote_id):
        self.remote_id = remote_id

    def __del__(self):
        try:
            conn.communicate('del', obj=self.remote_id)
        except (NameError, AttributeError, ValueError):
            # on intepreter shutdown, globals may have been already deleted
            # and sockets may have been closed
            pass

    def __getattribute__(self, name):
        try:
            return super().__getattribute__(name)
        except AttributeError:
            return conn.communicate('get', obj=self, name=name)

    def __str__(self):
        try:
            return conn.call(self, '__str__')
        except Exception:
            return super().__str__()
    def __len__(self):
        return conn.call(self, '__len__')
    def __lt__(self, other):
        return conn.call(self, '__lt__', other)
    def __le__(self, other):
        return conn.call(self, '__le__', other)
    def __eq__(self, other):
        return conn.call(self, '__eq__', other)
    def __ne__(self, other):
        return conn.call(self, '__ne__', other)
    def __gt__(self, other):
        return conn.call(self, '__gt__', other)
    def __ge__(self, other):
        return conn.call(self, '__ge__', other)
    def __getitem__(self, key):
        return conn.call(self, '__getitem__', key)
    def __delitem__(self, key):
        return conn.call(self, '__delitem__', key)
    def __setitem__(self, key, value):
        return conn.call(self, '__setitem__', key, value)
    def __iter__(self):
        return conn.call(self, '__iter__')
    def __call__(self, *args, **kwargs):
        return conn.call(self, '__call__', *args, **kwargs)

class RemoteCache:
    def __init__(self):
        self.objs = weakref.WeakValueDictionary()

    def get_or_add(self, remote_id):
        try:
            return self.objs[remote_id]
        except KeyError:
            pass
        obj = RemoteObject(remote_id)
        self.objs[remote_id] = obj
        return obj

    def __getitem__(self, remote_id):
        return self.objs[remote_id]

class UnknownException(Exception):
    pass

class XferConnection:
    XFER_LIMIT = 128 * 1024

    def __init__(self, codec):
        self.codec = codec
        self.pipe_in = io.open(3, 'rb', 0)
        self.pipe_out = io.open(4, 'wb', 0)

    def send(self, t, data):
        d = { 'type': t, 'data': self.codec.encode(data) }
        data = json.dumps(d).encode()
        if len(data) > self.XFER_LIMIT:
            raise ValueError("Too large data")
        try:
            self.pipe_out.write(data)
            self.pipe_out.flush()
        except BrokenPipeError:
            sys.exit(1)

    def rcv(self):
        data = self.pipe_in.read(self.XFER_LIMIT)
        if not data:
            return (None, None)
        d = json.loads(data.decode())
        return (d['type'], self.codec.decode(d['data']))

class XferClientConnection(XferConnection):
    def __init__(self):
        super().__init__(XferClientCodec())

    def rcv(self):
        t, data = super().rcv()
        if t == 'exception':
            cls = data['exception']
            try:
                E = getattr(__builtins__, cls)
                if not issubclass(E, Exception):
                    E = UnknownException
            except AttributeError:
                E = UnknownException
            raise E(*data['args'])
        return (t, data)

    def start(self):
        t, data = self.rcv()
        if t != 'install':
            raise ValueError('Got unexpected type {}'.format(t))
        return data

    def communicate(self, t, **kwargs):
        self.send(t, kwargs)
        t, data = self.rcv()
        if t != 'ok':
            raise ValueError('Got unknown type {}'.format(t))
        return data

    def call(self, obj, name, *args, **kwargs):
        return self.communicate('call', obj=obj, name=name, args=args, kwargs=kwargs)

class XferIO(io.TextIOBase):
    def write(self, s):
        s = str(s)
        conn.communicate('print', message=s)
        return len(s)

def xfer_input(prompt=''):
    return conn.communicate('input', message=str(prompt))

if __name__ == '__main__':
    conn = XferClientConnection()

    mod_name = '/box/program.py'

    spec = importlib.util.spec_from_file_location('__main__', mod_name)
    m = importlib.util.module_from_spec(spec)
    sys.argv[0] = mod_name

    m.__dict__['input'] = xfer_input
    for k, v in conn.start().items():
        m.__dict__[k] = v

    old_stdout = io.open(1, 'w')
    xfer = XferIO()
    with contextlib.redirect_stdout(xfer):
        with contextlib.redirect_stderr(xfer):
            try:
                spec.loader.exec_module(m)
            except Exception as e:
                # Do not print the first 3 lines of the stack trace to hide the
                # machinery above. This unfortunately means we don't print chained
                # exceptions.
                et, ev, etb = sys.exc_info()
                sys.stderr.write(''.join(traceback.format_list(traceback.extract_tb(etb)[3:])))
                sys.stderr.write(''.join(traceback.format_exception_only(et, ev)))
