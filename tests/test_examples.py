import contextlib
import glob
import os
import sys

import pytest

import ö.__main__


@contextlib.contextmanager
def boring_context():
    yield


tested_filenames = set()


def create_tester_function(filename, expected_output,
                           should_raise=None):
    if should_raise is None:
        should_raise = boring_context()

    def result(capsys):
        old_argv = sys.argv[1:]
        try:
            sys.argv[1:] = [os.path.join('examples', filename)]
            with should_raise:
                # pytest catches SystemExits raised in tests
                ö.__main__.main()
        finally:
            sys.argv[1:] = old_argv

        assert capsys.readouterr() == (expected_output, ''), filename

    result.__name__ = 'test_' + filename.replace('.', '_dot_')
    result.__qualname__ = result.__name__
    globals()[result.__name__] = result     # lol
    tested_filenames.add(filename)


create_tester_function('hello.simple', 'hello world\n')
create_tester_function('block.simple', 'hello\n' * 3)
create_tester_function('scopes.simple', 'hello\n', pytest.raises(ValueError))
create_tester_function('mapping.simple', 'hello\nhi\n')
create_tester_function('if.simple', 'everything ok\n')
create_tester_function('foreach.simple', 'one\ntwo\nthree\n')
create_tester_function('while.simple', 'hello\n')
create_tester_function('zip.simple', 'a\nd\nb\ne\nc\nf\n')
create_tester_function(
    'func.simple', 'toot toot!\nabc\nxyz\n\ntoot returned:\nbam\n')
create_tester_function(
    'oop.simple', 'tooting:\nToot Toot!\n----------\n'
    'tooting the fancy toot:\n*** Extra Fanciness ***\nToot Toot!\n')


def test_everything_tested(monkeypatch):
    monkeypatch.chdir('examples')
    untested = set(glob.glob('*.simple')) - tested_filenames
    assert not untested, "untested examples: " + repr(untested)
