#ifndef OPERATOR_H
#define OPERATOR_H

#include "interpreter.h"   // IWYU pragma: keep
#include "objectsystem.h"  // IWYU pragma: keep

enum Operator {
	// name      Ö code     name is short for
	// --------------------------------------
	OPERATOR_ADD,  // +     ADD
	OPERATOR_SUB,  // -     SUBstract
	OPERATOR_MUL,  // *     MULtiply
	OPERATOR_DIV,  // /     DIVide
	OPERATOR_EQ,   // ==    EQuals
	OPERATOR_NE,   // !=    Not Equal
	OPERATOR_GT,   // >     Greater Than
	OPERATOR_LT,   // <     Less Than
	OPERATOR_GE,   // >=    Greater than or Equal
	OPERATOR_LE    // <=    Less than or Equal
};

// implementation of "(lhs OPERATOR rhs)"
// returns NULL or a new reference
struct Object *operator_call(struct Interpreter *interp, enum Operator op, struct Object *lhs, struct Object *rhs);

// these convenience functions return 1 for yes, 0 for no and -1 for error
int operator_eqint(struct Interpreter *interp, struct Object *lhs, struct Object *rhs);
int operator_ltint(struct Interpreter *interp, struct Object *lhs, struct Object *rhs);

#endif   // OPERATOR_H
