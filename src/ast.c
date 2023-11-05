#include "ast.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int splc_enable_colored_ast = 0;

ast_node create_empty_node()
{
    ast_node node = (ast_node)malloc(sizeof(ast_node_struct));
    node->type = SPLT_NULL;
    node->entry = NULL;
    node->children = NULL;
    node->num_child = 0;
    memset(&node->location, 0, sizeof(splc_loc));
    node->location.fid = -1;
    return node;
}

ast_node create_leaf_node(const splc_token_t type, const splc_loc location)
{
    ast_node node = create_empty_node();
    node->type = type;
    node->location = location;
    return node;
}

void add_child(ast_node parent, ast_node child)
{
    parent->children = (ast_node *)realloc(parent->children, (parent->num_child + 1) * sizeof(ast_node_struct));
    parent->children[parent->num_child] = child;
    ++(parent->num_child);
}

ast_node create_parent_node(const splc_token_t type, size_t num_child, ...)
{
    ast_node node = create_empty_node();
    node->type = type;
    node->location = SPLC_INVALID_LOC;

    va_list args;
    va_start(args, num_child);

    for (size_t i = 0; i < num_child; ++i)
    {
        ast_node child = va_arg(args, ast_node);
        if (SPLC_IS_LOC_INVALID(node->location))
            node->location = child->location;
        if (child == NULL || child->type == SPLT_NULL)
            continue;
        add_child(node, child);
    }
    va_end(args);
    return node;
}

static void _builtin_print_single_node(const ast_node node)
{
    if (splc_enable_colored_ast)
        printf("%s", splc_get_token_color_code(node->type));

    const char *tokenstr = splc_token2str(node->type); /* do not free this string, as it is a constant */
    printf("%s", tokenstr);

    if (splc_enable_colored_ast)
        printf("\033[0m");

    switch (node->type)
    {
    case SPLT_ID:
        printf(": %s", (char *)node->val);
        break;
    case SPLT_LTR_INT:
        printf(": %lu", node->ulong_val);
        break;
    case SPLT_LTR_FLOAT:
        printf(": %g", node->float_val);
        break;
    case SPLT_LTR_CHAR:
        printf(": %s", (char *)node->val);
        break;
    case SPLT_STR:
        printf(": \"%s\"", (char *)node->val);
        break;

    default:
        break;
    }

    if (splc_enable_colored_ast && SPLC_IS_LOC_INVALID(node->location))
        printf("\033[33m");
    if (node->num_child == 0)
    {
        char *location = splc_loc2str(node->location);
        printf(" %s", location);
        free(location);
    }
    if (splc_enable_colored_ast && SPLC_IS_LOC_INVALID(node->location))
        printf("\033[0m");
}

static void _builtin_print_ast(const ast_node node, const char *prefix)
{
    const char *indicator = NULL;
    const char *segment = NULL;

    for (int i = 0; i < node->num_child; i++)
    {
        if (i == node->num_child - 1)
        {
            indicator = "└── ";
            segment = "    ";
        }
        else
        {
            indicator = "├── ";
            segment = "│   ";
        }
        printf("%s%s", prefix, indicator);
        _builtin_print_single_node(node->children[i]);
        printf("\n");

        if (node->children[i]->num_child > 0)
        {
            char *next_prefix = (char *)malloc((strlen(prefix) + strlen(segment) + 1) * sizeof(char));
            sprintf(next_prefix, "%s%s", prefix, segment);
            _builtin_print_ast(node->children[i], next_prefix);
            free(next_prefix);
        }
    }
}

void print_ast(ast_node root)
{
    _builtin_print_single_node(root);
    printf("\n");
    _builtin_print_ast(root, "");
}