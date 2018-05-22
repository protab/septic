import importlib.util
import io
import json
import os
import sys
import traceback

XMIT_LIMIT = 128 * 1024
pipe_in = io.open(3, 'rb', 0)
pipe_out = io.open(4, 'wb', 0)

def communicate(data):
    data = json.dumps(data).encode()
    if len(data) > XMIT_LIMIT:
        raise ValueError("Too large data")
    pipe_out.write(data)
    pipe_out.flush()
    data = json.loads(pipe_in.read(XMIT_LIMIT).decode())
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

def send_output(msg):
    communicate({ 'type': 'print', 'message': str(msg) })

mod_name = '/box/program.py'

spec = importlib.util.spec_from_file_location('__main__', mod_name)
m = importlib.util.module_from_spec(spec)
sys.argv[0] = mod_name

m.__dict__['ahoj'] = send_output

try:
    spec.loader.exec_module(m)
except Exception as e:
    # Do not print the first 3 lines of the stack trace to hide the
    # machinery above. This unfortunately means we don't print chained
    # exceptions.
    et, ev, etb = sys.exc_info()
    sys.stderr.write(''.join(traceback.format_list(traceback.extract_tb(etb)[3:])))
    sys.stderr.write(''.join(traceback.format_exception_only(et, ev)))
