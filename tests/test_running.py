import pytest

from simplelang import tokenizer, ast_tree
from simplelang.run import Interpreter


def run_code(code, interp=None, context=None):
    if interp is None:
        interp = Interpreter()
    if context is None:
        context = interp.global_context

    # list() calls are for fail-fast debuggability
    tokens = list(tokenizer.tokenize(code))
    ast_statements = list(ast_tree.parse(tokens))
    interp.execute(ast_statements, context)


def test_hello_world(capsys):
    run_code('print "hello world";')
    assert capsys.readouterr() == ('hello world\n', '')


def test_variables(capsys):
    run_code('''
    var a = "value of a";
    const b = print;
    b a;

    a = "wat";
    b a;
    ''')
    assert capsys.readouterr() == ('value of a\nwat\n', '')

    with pytest.raises(ValueError):
        run_code('''
        const a = "hello";
        a = "lol";
        ''')

    with pytest.raises(ValueError):
        run_code('undefined_var = "toot";')
    with pytest.raises(ValueError):
        run_code('const a = "toot"; const a = "lol";')
    with pytest.raises(ValueError):
        run_code('print undefined_var;')
