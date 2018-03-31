import sys


def _get_line(filename, lineno):
    try:
        with open(filename, 'rb') as file:
            # read useless lines until we arrive at the correct line
            for useless_lineno in range(1, lineno):
                file.readline()
            return file.readline().strip().decode('utf-8')
    except (OSError, UnicodeError):
        return ''


def print_stack_trace(error):
    assert error.stack is not None, "the stack attribute wasn't set"

    print("stack trace:", file=sys.stderr)
    for ast_statement in error.stack:
        filename, lineno = ast_statement.location
        print("  %s:%s:" % (filename, lineno))
        line = _get_line(filename, lineno)
        if line:
            print("    " + line, file=sys.stderr)

    print("error:", str(error), file=sys.stderr)
