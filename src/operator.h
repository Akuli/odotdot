#ifndef OPERATOR_H
#define OPERATOR_H

struct Interpreter;
struct Object;

enum Operator {
	OPERATOR_ADD,  // +
	OPERATOR_SUB,  // -
	OPERATOR_MUL,  // *
	OPERATOR_DIV,  // /
	OPERATOR_EQ,   // ==
	OPERATOR_NE    // !=
};

// implementation of "(lhs OPERATOR rhs)"
// returns NULL or a new reference
struct Object *operator_call(struct Interpreter *interp, enum Operator op, struct Object *lhs, struct Object *rhs);

// these convenience functions return 1 for yes, 0 for no and -1 for error
int operator_eqint(struct Interpreter *interp, struct Object *a, struct Object *b);
int operator_neint(struct Interpreter *interp, struct Object *a, struct Object *b);

#endif   // OPERATOR_H
