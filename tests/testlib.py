import sys

class TestFailure(Exception):
    pass

class check:
    def __init__(self, name, *exceptions):
        self.name = name
        self._exceptions = exceptions

    def __enter__(self):
        pass

    def __exit__(self, exctype, excval, exctb):
        if exctype is None:
            if not self._exceptions:
                self.report_ok()
            else:
                self.report_fail('command succeeded while it should not')
            return True
        if self._exceptions and issubclass(exctype, self._exceptions):
            self.report_ok()
            return True
        if issubclass(exctype, TestFailure):
            self.report_fail(str(excval))
            return True
        self.report_fail('{}: {}'.format(exctype.__name__, str(excval)))
        return True

    def report_ok(self):
        print('TestSuccess: {}: ok'.format(self.name))

    def report_fail(self, msg):
        print('TestFailure: {}: {}'.format(self.name, msg))

def ok(s, result='ok'):
    if s == result:
        return
    raise TestFailure("returned unexpected value '{}'".format(s))

