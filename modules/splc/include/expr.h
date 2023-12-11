#ifndef EXPR_H
#define EXPR_H

#include "ast.h"
#include "splcdef.h"

/* expression type enumeration */
typedef enum expr_type expr_t;
enum expr_type
{
    EXPR_NULL = 0x00000000,
    EXPR_VOID,

    EXPR_INT,
    EXPR_UNSIGNED_INT,
    EXPR_LONG,
    EXPR_UNSIGNED_LONG,
    EXPR_FLOAT,
    EXPR_DOUBLE,
    EXPR_CHAR,

    EXPR_STRUCT,
    EXPR_ARRAY, /* Two args: size of array and type expression of the declarator */
    EXPR_FUNC, /*  */
    EXPT_POINTER
};

/* type expresssion node struct */
typedef struct expr_node_struct *expr_node;
typedef struct expr_node_struct
{
    expr_t type; /* type of the expression */
    char *id; /* id of node, if any */
    int lvalue; /* whether the node can be lvalue */

    size_t num_arg; /* number of arguments */
    expr_node *args; /* args of type expression */

} expr_node_struct;

/* Function deefinitions */
/* Create an empty node that has all its fields initialized to empty/NULL. */
expr_node expr_create_empty_node();

/* Create a node given type. */
expr_node expr_create_node(const expr_t type, const char *name);


/* Create a node given type with args. */
expr_node expr_create_node_with_args(const expr_t type, const char *name, size_t num_arg, ...);

/* Recusively copy. */
expr_node expr_deep_copy(const expr_node node);

/* Do type checking */
int expr_type_checking(const expr_node node1, const expr_node node2);

int expr_get_conv(splc_token_t t1, splc_token_t t2);

#endif /* EXPR_H */