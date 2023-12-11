#include "expr.h"
#include "splcdef.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>

expr_node expr_create_empty_node()
{
    expr_node node = (expr_node)malloc(sizeof(expr_node_struct));
    SPLC_ALLOC_PTR_CHECK(node, "out of memory creating empty expression node");
    node->type = EXPR_NULL;
    node->id = NULL;
    node->lvalue = 1; /* default 1 */
    node->num_arg = 0;
    node->args = NULL;
    
    return node;
}

expr_node expr_create_node(const expr_t type, const char *name)
{
    expr_node node = expr_create_empty_node();
    node->type = type;
    node->id = name ? strdup(name) : NULL;
    return node;
}

expr_node expr_create_node_with_args(const expr_t type, const char *name, size_t num_arg, ...)
{
    expr_node node = expr_create_empty_node();
    node->type = type;
    node->id = name ? strdup(name) : NULL;

    if (num_arg > 0)
    {
        node->num_arg = num_arg;
        node->args = (expr_node *)malloc(num_arg * sizeof(expr_node));
        SPLC_ALLOC_PTR_CHECK(node->args, "out of memory");

        va_list args;
        va_start(args, num_arg);

        for (size_t i = 0; i < num_arg; i++)
        {
            expr_node arg = va_arg(args, expr_node);
            node->args[i] = expr_deep_copy(arg);
        }

        va_end(args);
    }
    return node;
}

expr_node expr_deep_copy(const expr_node node)
{
    if (node == NULL)
    {
        return NULL;
    }

    expr_node result = expr_create_empty_node();
    result->type = node->type;
    result->id = node->id ? strdup(node->id) : NULL;
    
    /* Copy arguments */
    if (node->num_arg > 0)
    {
        result->num_arg = node->num_arg;
        result->args = (expr_node *)malloc(result->num_arg * sizeof(expr_node));
        SPLC_ALLOC_PTR_CHECK(result->args, "failed to copy node: out of memory");
        for (int i = 0; i < result->num_arg; i++)
        {
            result->args[i] = expr_deep_copy(node->args[i]);
        }
    }

    return result;
}

int expr_type_checking(const expr_node node1, const expr_node node2)
{
    // TODO: Type conversion
    if (node1->type != node2->type)
    {
        return 0;
    }

    if (node1->num_arg != node2->num_arg)
    {
        return 0;
    }

    for (int i = 0; i < node1->num_arg; i++)
    {
        // Recursive checking
        if (expr_type_checking(node1->args[i], node2->args[i]) == 0)
        {
            return 0;
        }
    }

    return 1;
}


