import contextlib
import importlib.util
import io
import json
import os
import sys
import traceback

XFER_LIMIT = 128 * 1024
pipe_in = io.open(3, 'rb', 0)
pipe_out = io.open(4, 'wb', 0)

def communicate(data):
    data = json.dumps(data).encode()
    if len(data) > XFER_LIMIT:
        raise ValueError("Too large data")
    try:
        pipe_out.write(data)
        pipe_out.flush()
    except BrokenPipeError:
        sys.exit(1)
    data = json.loads(pipe_in.read(XFER_LIMIT).decode())
    if data['type'] == 'ok':
        return data;
    if data['type'] == 'exception':
        cls = data['exception']
        try:
            E = globals()[cls]
        except KeyError:
            E = UnknownException
        raise E(*data['args'])
    raise ValueError('Got unknown type {}'.format(data['type']))

class UnknownException(Exception):
    pass

class XferIO(io.TextIOBase):
    def write(self, s):
        s = str(s)
        communicate({ 'type': 'print', 'message': s })
        return len(s)

def xfer_input(prompt=''):
    data = communicate({ 'type': 'input', 'message': str(prompt) })
    return data['data']

mod_name = '/box/program.py'

spec = importlib.util.spec_from_file_location('__main__', mod_name)
m = importlib.util.module_from_spec(spec)
sys.argv[0] = mod_name

m.__dict__['input'] = xfer_input

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
