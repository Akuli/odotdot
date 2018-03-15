import functools

import pytest

from simplelang import ast_tree, tokenizer, objects
from simplelang.run import Interpreter


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
    var b = print;
    b a;

    a = "wat";
    b a;
    ''')
    assert capsys.readouterr() == ('value of a\nwat\n', '')

    with pytest.raises(ValueError):
        run_code('undefined_var = "toot";')
    with pytest.raises(ValueError):
        run_code('print undefined_var;')


def _setup_dummy(this, *args):
    print("setting up")
    assert args == (objects.new_string('a'),
                    objects.new_string('b'))
    this.attributes['x'] = objects.new_string('y')
    return objects.null


def _dummy_toot(this):
    print("toot toot")
    return objects.null


dummy_info = objects.ClassInfo(objects.object_info, {
    'setup': _setup_dummy,
    'toot': _dummy_toot,
})


def test_object_stuff(run_code, capsys):
    run_code.context.local_vars['Dummy'] = objects.class_objects[dummy_info]

    run_code.context.local_vars['d1'] = objects.Object(dummy_info)
    assert capsys.readouterr() == ('', '')
    run_code('d1.setup "a" "b";')
    assert capsys.readouterr() == ('setting up\n', '')

    run_code('var d2 = (new Dummy "a" "b");')
    assert capsys.readouterr() == ('setting up\n', '')

    run_code('d1.toot; d2.toot;')
    assert capsys.readouterr() == ('toot toot\ntoot toot\n', '')


def test_attributes(run_code, capsys):
    run_code.context.local_vars['d'] = objects.Object(objects.object_info)
    run_code.context.local_vars['d'].attributes['a'] = objects.new_string("hi")

    run_code('''print d.a;
                d.a = "new hi";
                print d.a;''')
    assert capsys.readouterr() == ('hi\nnew hi\n', '')

    run_code('d.lol = "asd";')
    lol = run_code.context.local_vars['d'].attributes['lol']
    assert objects.is_instance_of(lol, objects.string_info)
    assert lol.python_string == 'asd'

    run_code('print d.lol; d.lol = "new lol"; print d.lol;')
    assert capsys.readouterr() == ('asd\nnew lol\n', '')

    # TODO: some way to prevent setting any attributes


def test_blocks(run_code, capsys):
    run_code('''
    var hello = {
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
    var firstblock = {
        print a;
        a = "new a";
        print a;
    };
    firstblock.run;
    print a;
    ''')
    assert capsys.readouterr() == ('original a\nnew a\nnew a\n', '')

    run_code('''
    {
        var a = "even newer a";
        print a;
    }.run;
    print a;    # it didn't change this
    ''')
    assert capsys.readouterr() == ('even newer a\nnew a\n', '')

    run_code('''
    var a = "damn new a";
    firstblock.run;      # it changed everything that uses this context
    ''')
    assert capsys.readouterr() == ('damn new a\nnew a\n', '')


# a friend of mine says that this is an idiom... i have never seen it
# being actually used though
def test_funny_idiom(run_code, capsys):
    run_code('''
    var a = "global a";
    {
        var a=a;    # localize a
        a = "local a";
        print a;
    }.run;
    print a;
    ''')
    assert capsys.readouterr() == ('local a\nglobal a\n', '')
