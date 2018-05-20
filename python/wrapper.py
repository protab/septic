import importlib.util
import traceback
import sys

mod_name = '/box/program.py'

spec = importlib.util.spec_from_file_location('__main__', mod_name)
m = importlib.util.module_from_spec(spec)
sys.argv[0] = mod_name

m.__dict__['ahoj'] = 'nazdar'

try:
    spec.loader.exec_module(m)
except Exception as e:
    # Do not print the first 3 lines of the stack trace to hide the
    # machinery above. This unfortunately means we don't print chained
    # exceptions.
    et, ev, etb = sys.exc_info()
    sys.stderr.write(''.join(traceback.format_list(traceback.extract_tb(etb)[3:])))
    sys.stderr.write(''.join(traceback.format_exception_only(et, ev)))
