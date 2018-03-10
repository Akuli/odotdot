import argparse

from simplelang import tokenizer, ast_tree, run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('file')
    args = parser.parse_args()

    with open(args.file, 'r') as file:
        content = file.read()

    tokens = tokenizer.tokenize(content)
    ast_statements = ast_tree.parse(tokens)

    interpreter = run.Interpreter()
    interpreter.execute(ast_statements, interpreter.global_context)


if __name__ == '__main__':
    main()
