import importlib
import io
import json
import os
import os.path
import selectors
import sys
import time
import wrapper

limit_out = 65536
XFER_LIMIT = 128 * 1024

class PassThruException(Exception):
    pass

def format_exception(e):
    return {
        'type': 'exception',
        'exception': e.__class__name,
        'args': e.args,
    }

def process_request(data):
    if data['type'] == 'print':
        msg = data['message']
        try:
            answer = process_print(msg)
        except Exception as e:
            raise PassThruException(e)

    elif data['type'] == 'input':
        msg = data['message']
        try:
            answer = process_input(msg)
        except Exception as e:
            raise PassThruException(e)

    else:
        raise ValueError('Unknown type {}'.format(data['type']))

    answer['type'] = 'ok'
    return answer

def process_print(msg):
    global limit_out

    chars = len(msg)
    if chars > limit_out:
        msg = msg[:limit_out]
        chars = limit_out
    out.write(msg)
    out.flush()
    limit_out -= chars

    return {}

def process_input(msg):
    tmp_name = os.path.join(meta, 'input.tmp')
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

    return { 'data': data }

def xmit(data):
    if len(data) > XFER_LIMIT:
        raise ValueError("Too large data")
    try:
        pipe_out.write(data)
        pipe_out.flush()
    except BrokenPipeError:
        sys.exit(1)

meta = sys.argv[1]
master = importlib.import_module(sys.argv[2])
os.set_blocking(3, False)
pipe_in = io.open(3, 'rb', 0)
pipe_out = io.open(4, 'wb', 0)
out = open(os.path.join(meta, 'output'), 'w')

sel = selectors.DefaultSelector()
sel.register(pipe_in, selectors.EVENT_READ)
finish = False
while not finish:
    for key, events in sel.select():
        if key.fileobj == pipe_in:
            data = pipe_in.read(XFER_LIMIT)
            if len(data) == 0:
                finish = True
                break
            try:
                answer = process_request(json.loads(data.decode()))
            except PassThruException as e:
                raise e.args[0] from None
            except Exception as e:
                answer = format_exception(e)
            xmit(json.dumps(answer).encode())
