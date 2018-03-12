import argparse

from simplelang import tokenizer, ast_tree, run, objects


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file')
    args = parser.parse_args()

    with open(args.file, 'r') as file:
        content = file.read()

    tokens = tokenizer.tokenize(content)
    ast_statements = ast_tree.parse(tokens)

    interpreter = run.Interpreter()
    try:
        for statement in ast_statements:
            interpreter.execute(statement, interpreter.global_context)
    except objects.ReturnAValue:
        raise ValueError("unexpected return")


if __name__ == '__main__':     # pragma: no cover
    main()
