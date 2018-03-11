import functools

import pytest

from simplelang import ast_tree, tokenizer, objects
from simplelang.run import Interpreter, builtin_context


# run a bunch of code in the same interpreter and context
@pytest.fixture(scope='function')
def run_code():
    interp = Interpreter()

    def run(code):
        tokens = list(tokenizer.tokenize(code))
        ast_statements = list(ast_tree.parse(tokens))
        for statement in ast_statements:
            interp.execute(statement, interp.global_context)

    run.context = interp.global_context
    return run


def test_hello_world(run_code, capsys):
    run_code('print "hello world";')
    assert capsys.readouterr() == ('hello world\n', '')


def test_variables(run_code, capsys):
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


def test_attributes(run_code, capsys):
    run_code.context.namespace.add('d', Dummy())

    run_code('''print d.noconstant;
                d.noconstant = "new hi";
                print d.noconstant;''')
    assert capsys.readouterr() == ('hi\nnew hi\n', '')

    with pytest.raises(ValueError):
        run_code('''print d.constant;
                    d.constant = "wat";
                    print d.constant;''')
    assert capsys.readouterr() == ('hello\n', '')

    run_code('var d.lol = "asd";')
    lol = run_code.context.namespace.get('d').attributes.get('lol')
    assert isinstance(lol, objects.String)
    assert lol.python_string == 'asd'

    run_code('print d.lol; d.lol = "new lol"; print d.lol;')
    assert capsys.readouterr() == ('asd\nnew lol\n', '')

    run_code('const d.wat = "ASD";')
    wat = run_code.context.namespace.get('d').attributes.get('wat')
    assert isinstance(wat, objects.String)
    assert wat.python_string == 'ASD'

    with pytest.raises(ValueError):
        run_code('d.wat = "toot";')

    # make sure that attributes cannot be recreated
    for keyword in ['const', 'var']:
        for attr in ['constant', 'noconstant', 'lol', 'wat']:
            with pytest.raises(ValueError):
                run_code('%s d.%s = "tooooooooooooooooot";' % (keyword, attr))

    run_code.context.namespace.get('d').attributes.can_add = False

    # old attributes must still work...
    run_code('d.lol = "very new lol"; print d.lol;')
    assert capsys.readouterr() == ('very new lol\n', '')

    # ...even though new attributes cannot be added
    with pytest.raises(ValueError):
        run_code('var d.omg = "waaaaaat";')
    with pytest.raises(ValueError):
        run_code('const d.omg2 = "waaaaaat";')


def test_code_objects(run_code, capsys):
    run_code('''
    const hello = {
        print "hello world";
        print "hello again";
    };
    ''')
    assert capsys.readouterr() == ('', '')

    with pytest.raises(AttributeError):     # lol
        run_code('hello;')
    run_code('hello.run;')
    assert capsys.readouterr() == ('hello world\nhello again\n', '')

    run_code('''
    var a = "original a";
    {
        print a;
        a = "new a";
        print a;
    }.run;
    print a;
    ''')
    assert capsys.readouterr() == ('original a\nnew a\nnew a\n', '')

    with pytest.raises(ValueError):
        run_code('var a = "lol";')
    with pytest.raises(ValueError):
        run_code('{ var a = "lol"; }.run;')
