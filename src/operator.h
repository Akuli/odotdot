#ifndef OPERATOR_H
#define OPERATOR_H

struct Interpreter;
struct Object;

// these functions are equivalent to things like (a + b) in ö
// add stuff to array objects in interp->importstuff to customize operator behaviour
struct Object *operator_add(struct Interpreter *interp, struct Object *a, struct Object *b);   // +
struct Object *operator_sub(struct Interpreter *interp, struct Object *a, struct Object *b);   // -
struct Object *operator_mul(struct Interpreter *interp, struct Object *a, struct Object *b);   // *
struct Object *operator_div(struct Interpreter *interp, struct Object *a, struct Object *b);   // /
struct Object *operator_eq(struct Interpreter *interp, struct Object *a, struct Object *b);    // ==
struct Object *operator_ne(struct Interpreter *interp, struct Object *a, struct Object *b);    // !=

// these convenience functions return 1 for yes, 0 for no and -1 for error
int operator_eqint(struct Interpreter *interp, struct Object *a, struct Object *b);
int operator_neint(struct Interpreter *interp, struct Object *a, struct Object *b);

#endif   // OPERATOR_H
