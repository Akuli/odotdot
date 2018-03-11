import contextlib
import glob
import sys

import pytest

import simplelang.__main__

expected_output = {
    'hello.simple': 'hello world\n',
    'code.simple': 'hello\n' * 3,
    'scopes.simple': 'hello\n',
    'mapping.simple': 'hello\nhi\n',
    'if.simple': 'everything ok\n',
    'foreach.simple': 'one\ntwo\nthree\n',
}
should_raise = {'scopes.simple': pytest.raises(ValueError)}


@contextlib.contextmanager
def nothing_special():
    yield


# i don't know why chdirs are done with a fixture called monkeypatch in pytest
# TODO: write this stuff in the language itself :D
def test_examples(monkeypatch, capsys):
    monkeypatch.chdir('examples')

    for filename, output_should_be in expected_output.items():
        context_manager = should_raise.get(filename, nothing_special())

        old_argv = sys.argv[1:]
        try:
            sys.argv[1:] = [filename]
            with context_manager:
                # pytest catches SystemExits raised in tests
                simplelang.__main__.main()
        finally:
            sys.argv[1:] = old_argv

        assert capsys.readouterr() == (output_should_be, ''), filename

    # if there's something that is in expected_output but not in the
    # examples directory, running it has already failed above
    untested = set(glob.glob('*.simple')) - expected_output.keys()
    assert not untested, "untested examples: " + repr(untested)
