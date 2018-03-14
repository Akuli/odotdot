import argparse

from simplelang import tokenizer, ast_tree, run, objects


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file', type=argparse.FileType('r'),
                        nargs=argparse.OPTIONAL)
    args = parser.parse_args()

    interpreter = run.Interpreter()

    if args.file is None:
        # REPL
        while True:
            code = input('>> ')
            tokens = tokenizer.tokenize(code)
            ast_statements = ast_tree.parse(tokens)

            try:
                for statement in ast_statements:
                    interpreter.execute(statement, interpreter.global_context)
            except Exception as e:
                print(type(e).__name__, e, sep=': ')
    else:
        with args.file:
            content = args.file.read()

        tokens = tokenizer.tokenize(content)
        ast_statements = ast_tree.parse(tokens)

        try:
            for statement in ast_statements:
                interpreter.execute(statement, interpreter.global_context)
        except objects.ReturnAValue:
            raise ValueError("unexpected return")


if __name__ == '__main__':     # pragma: no cover
    main()
