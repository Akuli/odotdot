import functools

import pytest

from simplelang import ast_tree, tokenizer, objects
from simplelang.run import Context, Interpreter, builtin_context


def run_code(code, interp=None, context=None):
    if interp is None:
        interp = Interpreter()
    if context is None:
        context = Context(interp.global_context)

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


class Dummy(objects.Object):

    def __init__(self):
        super().__init__()
        self.attributes.add('constant', objects.String("hello"), is_const=True)
        self.attributes.add('noconstant', objects.String("hi"), is_const=False)


def test_attributes(capsys):
    interp = Interpreter()
    context = Context(builtin_context)
    context.namespace.add('d', Dummy())
    run = functools.partial(run_code, interp=interp, context=context)

    run('print d.noconstant; d.noconstant = "new hi"; print d.noconstant;')
    assert capsys.readouterr() == ('hi\nnew hi\n', '')

    with pytest.raises(ValueError):
        run('print d.constant; d.constant = "wat"; print d.constant;')
    assert capsys.readouterr() == ('hello\n', '')

    run('var d.lol = "asd";')
    lol = context.namespace.get('d').attributes.get('lol')
    assert isinstance(lol, objects.String)
    assert lol.python_string == 'asd'

    run('print d.lol; d.lol = "new lol"; print d.lol;')
    assert capsys.readouterr() == ('asd\nnew lol\n', '')

    run('const d.wat = "ASD";')
    wat = context.namespace.get('d').attributes.get('wat')
    assert isinstance(wat, objects.String)
    assert wat.python_string == 'ASD'

    with pytest.raises(ValueError):
        run('d.wat = "toot";')

    # make sure that attributes cannot be recreated
    for keyword in ['const', 'var']:
        for attr in ['constant', 'noconstant', 'lol', 'wat']:
            with pytest.raises(ValueError):
                run('%s d.%s = "tooooooooooooooooot";' % (keyword, attr))

    context.namespace.get('d').attributes.can_add = False

    # old variables must still work...
    run('d.lol = "very new lol"; print d.lol;')
    assert capsys.readouterr() == ('very new lol\n', '')

    # ...even though new variables cannot be added
    with pytest.raises(ValueError):
        run('var d.omg = "waaaaaat";')
    with pytest.raises(ValueError):
        run('var d.omg = "waaaaaat";')
