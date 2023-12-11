#include "semantics.h"
#include "limits.h"
#include "type.h"
#include "utils.h"



static void experimental_analyze_dispatch(splc_trans_unit tunit, ast_node node, int root_env_idx);

// EXPERIMENTAL
static ast_node find_typedef(const ast_node node)
{
    if (node->type == SPLT_TYPEDEF)
    {
        return node;
    }
    for (int i = 0; i < node->num_child; ++i)
    {
        ast_node tmp = NULL;
        switch (node->children[i]->type)
        {
        case SPLT_DECLTN_SPEC:
        case SPLT_STRG_SPEC:
            if ((tmp = find_typedef(node->children[i])) != NULL)
                return tmp;
            break;
        case SPLT_TYPEDEF:
            return node;
        default:
            break;
        }
    }
    return NULL;
}

// EXPERIMENTAL
static void register_typedef(const ast_node node)
{
    if (node->type == SPLT_ID)
    {
        SPLC_ASSERT(current_trans_unit->nenvs > 0);
        lut_insert(current_trans_unit->global_symtable, (const char *)node->val, SPLE_TYPEDEF, SPLE_NULL, NULL, NULL,
                   node, node->location);
    }
    for (int i = 0; i < node->num_child; ++i)
    {
        switch (node->children[i]->type)
        {
        case SPLT_INIT_DEC_LIST:
        case SPLT_INIT_DEC:
        case SPLT_FUNC_DEC:
        case SPLT_DIR_FUNC_DEC:
        case SPLT_DEC:
        case SPLT_DIR_DEC:
            register_typedef(node->children[i]);
            break;
        case SPLT_ID:
            lut_insert(current_trans_unit->global_symtable, (const char *)node->children[i]->val, SPLE_TYPEDEF,
                       SPLE_NULL, NULL, NULL, node, node->location);
            break;
        default:
            break;
        }
    }
}

void sem_register_typedef(const ast_node node)
{
    if (node == NULL || find_typedef(node) == NULL)
        return;
    register_typedef(node);
}

int sem_test_typedef_name(const char *name)
{
    lut_entry ent = NULL;
    SPLC_ASSERT(current_trans_unit->nenvs > 0);
    // TODO(sem): fix typedef name scope check
    return (ent = lut_find(current_trans_unit->global_symtable, name, SPLE_TYPEDEF)) != NULL &&
           ent->type == SPLE_TYPEDEF;
}

// ============================================
// ============= Post Processing ==============

/* Register a declaration of type SPLT_DIR_DECLTN */
void register_direct_declaration(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    // TODO: register direct declarations
}

/* Register a declaration of type SPLT_DECLTN */
void register_declaration(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    SPLC_ASSERT(node->num_child > 0 && node->children[0]->type == SPLT_DIR_DECLTN);
    register_direct_declaration(tunit, node->children[0], root_env_idx);
}

/* Get a string of format "anonymous{id}". User must free it after use. */
static inline char *get_anonymous_name(int id)
{
    SPLC_ASSERT(id > 0);
    const char *name = "anonymous";
    size_t namelen = strlen(name);
    size_t needed = namelen + 1 + 10; // 1 for '\0' and 10 for id
    char *buffer = malloc(needed);
    SPLC_ALLOC_PTR_CHECK(buffer, "failed to create anonymous name buffer");
    memcpy(buffer, name, namelen);
    snprintf(buffer + namelen, 10, "%d", id);
    return buffer;
}

static void register_struct_specifiers(splc_trans_unit tunit, ast_node node, int root_env_idx, int struct_decl_env)
{
    // TODO: recursively register structure specifiers
}

static void register_struct_decltn(splc_trans_unit tunit, ast_node node, int root_env_idx, int struct_decl_env)
{
    // TODO: register struct
    // BONUS: support bit field
    // Just append and leave it to the type system
}

/* Register a declaration of type SPLT_STRUCT_UNION_SPEC.
   `root_env_idx` is where the struct declaration is placed.
   `struct_decl_env` is where the declaration body of struct is placed. */
static void register_struct_spec(splc_trans_unit tunit, ast_node node, int root_env_idx, int struct_decl_env)
{
    SPLC_ASSERT(node->type == SPLT_STRUCT_UNION_SPEC);

    lut_entry existing = NULL;
    ast_node type_children = node->children[0];
    ast_node id_children = NULL;
    char *decl_name = NULL; // allocated and should be freed
    splc_entry_t tmp_decl_entry_type = SPLE_NULL;
    ast_node decl_body = NULL;
    int is_defining = 0;

    if (type_children->type == SPLT_KWD_STRUCT)
        tmp_decl_entry_type = SPLE_STRUCT_DEC;
    else
        tmp_decl_entry_type = SPLE_UNION_DEC;

    if (node->children[1]->type == SPLT_ID)
        id_children = node->children[1];

    if (node->children[1]->type == SPLT_STRUCT_DECLTN_BODY)
    {
        is_defining = 1;
        decl_body = node->children[1];
    }
    else if (node->num_child == 3 && node->children[2]->type == SPLT_STRUCT_DECLTN_BODY)
    {
        is_defining = 1;
        decl_body = node->children[2];
    }

    if (id_children != NULL)
    {
        decl_name = strdup((char *)(id_children->val)); // name of struct/union
        SPLC_ALLOC_PTR_CHECK(decl_name, "failed to allocate name for struct declaration.");
        existing = lut_find(tunit->envs[root_env_idx], decl_name, tmp_decl_entry_type);
    }
    else
    {
        char *new_name = NULL;
        int id;
        for (id = 0; id < INT_MAX; ++id)
        {
            new_name = get_anonymous_name(id);
            if (!lut_exists(tunit->envs[root_env_idx], decl_name, SPLE_STRUCT_DEC))
            {
                break;
            }
            free(new_name);
        }
        if (id == INT_MAX)
            SPLC_FFAIL("number of anonymous struct reached maximum limit: \033[1m%d\033[0m.", INT_MAX);

        decl_name = new_name;
    }

    if (existing != NULL && is_defining && existing->is_defined) // Only if it is redefined should we do this
    {
        SPLC_FMSG(SPLM_ERR_SEM_15, node->location, "redefinition of struct/union %s", decl_name);
        SPLC_MSG(SPLM_NOTE, existing->first_occur, "previously defined here.");
    }
    else
    {
        lut_entry ent = NULL;
        // Override the previous entry only if we are defining this entry
        if (is_defining)
            ent = lut_insert(tunit->envs[root_env_idx], decl_name, tmp_decl_entry_type, SPLE_NULL, NULL, NULL, node,
                             node->location);
        ent->is_defined = is_defining;

        SPLC_FDIAG("registered struct declaration: %s %d", decl_name, tmp_decl_entry_type);
        // Register internal declarations
        if (is_defining)
        {
            splc_push_new_symtable(tunit, 1);
            int current_env_idx = tunit->nenvs - 1;
            if (decl_body->num_child > 0) // If a body indeed exists.
            {
                ast_node decltn_list = decl_body->children[1];
                SPLC_ASSERT(decltn_list->type == SPLT_STRUCT_DECLTN_LIST);
                for (size_t i = 0; i < decltn_list->num_child; ++i)
                {
                    // Register each single struct-declaration
                    register_struct_decltn(tunit, decltn_list->children[i], root_env_idx, current_env_idx);
                }
            }
            node->symtable = splc_pop_symtable(tunit); // Linked
            SPLC_FDIAG("registered struct definition: %s %d", decl_name, tmp_decl_entry_type);
        }
    }

    // cleanup
    free(decl_name);
}

/* Register SPLT_COMP_STMT without creating a new environment */
static void register_comp_stmt_no_new_env(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    SPLC_ASSERT(node->type == SPLT_COMP_STMT);
    if (node->num_child == 0)
        return;
    ast_node gen_stmt_list = node->children[0];
    for (size_t i = 0; i < gen_stmt_list->num_child; ++i)
    {
        experimental_analyze_dispatch(tunit, gen_stmt_list->children[i], root_env_idx);
    }
}

/* Wrapper function for registering SPLT_COMP_STMT as root.
   This wrapper creates a new environment and calls
   `register_comp_stmt_no_new_env` internally. */
static void register_simple_comp_stmt(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    splc_push_new_symtable(tunit, 1);
    int current_env_idx = tunit->nenvs - 1;

    register_comp_stmt_no_new_env(tunit, node, current_env_idx);

    lut_table top_sym_table = splc_pop_symtable(tunit);
    node->symtable = top_sym_table; // Linked
}

/* Wrapper function for register self-contained statements such as
   SPLT_ITER_STMT,  */
static void register_self_contained_stmt(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    splc_push_new_symtable(tunit, 1);
    int current_env_idx = tunit->nenvs - 1;

    // DONE: register declarations if it is a for loop
    if (node->type == SPLT_ITER_STMT &&
        node->children[0]->type == SPLT_FOR && // for-loop-body is always placed at third (no punctuators)
        node->children[1]->num_child > 0 && node->children[1]->children[0]->type == SPLT_INIT_EXPR)
    {
        ast_node init_expr = node->children[1]->children[0];
        register_direct_declaration(tunit, init_expr, current_env_idx);
    }
    // DONE: find the compound statement and register; if not, dispatch the simple statement
    for (size_t i = 0; i < node->num_child; ++i)
    {
        if (node->children[i]->type == SPLT_STMT)
        {
            ast_node stmt = node->children[i];
            if (stmt->children[0]->type == SPLT_COMP_STMT)
            {
                register_comp_stmt_no_new_env(tunit, stmt->children[0], current_env_idx);
            }
            else
            {
                experimental_analyze_dispatch(tunit, stmt, current_env_idx);
            }
            break;
        }
    }

    lut_table top_sym_table = splc_pop_symtable(tunit);
    node->symtable = top_sym_table; // Linked
}

static void register_function_def(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    const char *func_name = NULL;
    {
        ast_node func_decltr = node->children[1];                                     // GUARANTEED
        ast_node dir_func_decltr = func_decltr->children[func_decltr->num_child - 1]; // GUARANTEED
        func_name = (char *)dir_func_decltr->val;
    }
    lut_entry existing = NULL;
    int is_defining = 0;

    if (node->num_child == 3)
        is_defining = 1; // GUARANTEED

    if ((existing = lut_find(tunit->envs[root_env_idx], func_name, SPLE_FUNC)) != NULL && is_defining &&
        existing->is_defined)
    {
        SPLC_FMSG(SPLM_ERR_SEM_4, node->location, "redefinition of function '\033[1m%s\033[0m'", func_name);
    }
    else
    {
        if (is_defining)
        {
            splc_push_new_symtable(tunit, 1);
            int current_env_idx = tunit->nenvs - 1;
            // TODO: register all parameters

            register_comp_stmt_no_new_env(tunit, node->children[2], current_env_idx);
            node->symtable = splc_pop_symtable(tunit);
        }
    }
}

static void experimental_expr_dispatch(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    // TODO: finish experimental expression dispatch
}

/* The main function to dispatch semantic analyses.
   When called, this function tries to analyze the given node.

   Parameters
   ----------
   - `tunit` the root translation unit
   - `node` the node to be analyzed
   - `root_env_idx` the index of environment in the translation unit

   Allowed types
   -------------
   - `SPLT_TRANS_UNIT`
   - `SPLT_EXT_DECLTN_LIST`
   - `SPLT_EXT_DECLTN`
   - `SPLT_FUNC_DEF`
   - `SPLT_ITER_STMT` for handling extra variable outside of `SPLT_COMP_STMT`
   - `SPLT_COMP_STMT` for handling compound statements, creating a new environment.
     If you do not need to create a separate environment, do not call this to dispatch it.
   - `SPLT_STMT` for handling general statements
   - `SPLT_EXPR` for handling expressions
  */
static void experimental_analyze_dispatch(splc_trans_unit tunit, ast_node node, int root_env_idx)
{
    SPLC_ASSERT(node->type != SPLT_NULL);
    SPLC_ASSERT(!SPLT_IS_PUNCTUATOR(node->type));
    if (node->type == SPLT_TRANS_UNIT)
    {
        experimental_analyze_dispatch(tunit, node, root_env_idx);
    }
    else if (node->type == SPLT_EXT_DECLTN_LIST)
    {
        experimental_analyze_dispatch(tunit, node, root_env_idx);
    }
    else if (node->type == SPLT_EXT_DECLTN)
    {
        experimental_analyze_dispatch(tunit, node, root_env_idx);
    }
    else if (node->type == SPLT_DECLTN)
    {
        register_declaration(tunit, node, root_env_idx);
    }
    else if (node->type == SPLT_FUNC_DEF)
    {
        register_function_def(tunit, node, root_env_idx);
    }
    else if (SPLT_IS_STMT(node->type))
    {
        if (node->children[0]->type == SPLT_ITER_STMT)
        {
            register_self_contained_stmt(tunit, node, root_env_idx);
        }
        else if (node->children[0]->type == SPLT_COMP_STMT)
        {
            register_simple_comp_stmt(tunit, node, root_env_idx);
        }
        // TODO: dispatch other statements
        if (node->num_child > 0)
        {
            for (size_t i = 0; i < node->num_child; ++i)
            {
                experimental_analyze_dispatch(tunit, node->children[i], root_env_idx);
            }
        }
    }
    else if (SPLT_IS_EXPR(node->type))
    {
        experimental_expr_dispatch(tunit, node, root_env_idx);
    }
    else
    {
        SPLC_FFAIL("Unsupported type during id registration: %s.", splc_token2str(node->type));
    }
}

static void legacy_ast_search(ast_node node, ast_node fa_node, splc_trans_unit tunit, int new_sym_table,
                              splc_entry_t decl_entry_type, splc_entry_t decl_extra_type, const char *decl_spec_type,
                              int in_struct, int in_expr)
{
    // new table construction
    int find_stmt = 0;
    if ((node->type == SPLT_STRUCT_UNION_SPEC && node->num_child == 3) || node->type == SPLT_FUNC_DEF)
        new_sym_table = 1; // Update to avoid pushing a global symbol table when encountering
                           // SPLT_TRANS_UNIT, as there is already one at scope 0.
    if (new_sym_table)
        splc_push_new_symtable(tunit, 1);

    int copy_new_sym_table = new_sym_table;
    if (node->type == SPLT_SEL_STMT || node->type == SPLT_ITER_STMT)
        find_stmt = 1;

    // definition
    // definition of struct/union
    if (node->type == SPLT_STRUCT_UNION_SPEC && node->num_child == 3)
    {
        ast_node type_children = node->children[0];
        ast_node id_children = node->children[1];
        splc_entry_t tmp_decl_entry_type;
        if (type_children->type == SPLT_KWD_STRUCT)
        {
            tmp_decl_entry_type = SPLE_STRUCT_DEC;
        }
        else
        {
            tmp_decl_entry_type = SPLE_UNION_DEC;
        }
        char *struct_union_name = (char *)(id_children->val); // name of struct/union
        // FINISHED: check if there is a struct(or function) with the same name
        int struct_union_undefined = 0;
        for (int i = 0; i < tunit->nenvs; i++)
        {
            if (lut_exists(tunit->envs[i], struct_union_name, SPLE_STRUCT_DEC))
                struct_union_undefined = 1;
        }
        if (struct_union_undefined)
        {
            SPLC_FMSG(SPLM_ERR_SEM_15, node->location, "redefinition of struct/union %s", struct_union_name);
            // SPLC_FDIAG("Error type 15 at line %d: redefinition of %s\n", node->location.linebegin,
            // struct_union_name);
        }
        SPLC_FDIAG("struct: %s %d", struct_union_name, tmp_decl_entry_type);
        lut_insert(tunit->envs[(tunit->nenvs) - 1], struct_union_name, tmp_decl_entry_type, SPLE_NULL, NULL, NULL, node,
                   node->location);
        lut_insert(tunit->envs[(tunit->nenvs) - 2], struct_union_name, tmp_decl_entry_type, SPLE_NULL, NULL, NULL, node,
                   node->location);
        in_struct = 1;
    }
    // definition in struct/union
    if (node->type == SPLT_STRUCT_DECLTN) // Struct/Union-Decl
    {
        decl_extra_type = SPLE_NULL;
        ast_node typespec_child_node = ((node->children[0])->children[0])->children[0]; // son of TypeSpec
        if (typespec_child_node->num_child == 0)                                        // variable type
        {
            decl_entry_type = SPLE_VAR;
            decl_spec_type = splc_token2str(typespec_child_node->type);
        }
        else
        { // struct/union type
            // pass two parameter into his brothers for inserting into table
            decl_entry_type = SPLE_VAR;
            ast_node type_children = typespec_child_node->children[0];
            ast_node id_children = typespec_child_node->children[1];
            if (type_children->type == SPLT_KWD_STRUCT)
                decl_extra_type = SPLE_STRUCT_DEC;
            else
            {
                decl_extra_type = SPLE_UNION_DEC;
            }
            decl_spec_type = (char *)(id_children->val);
        }
    }

    // definition of function
    if (node->type == SPLT_FUNC_DEF) // FunctionDef
    {
        decl_entry_type = SPLE_FUNC;
        ast_node typespec_child_node = (((node->children[0])->children[0])->children[0]); // Typespec's child
        if (typespec_child_node->num_child == 0)                                          // variable type
        {
            decl_extra_type = SPLE_VAR;
            decl_spec_type = splc_token2str(typespec_child_node->type);
        }
        else
        { // struct/union type
            ast_node type_children = typespec_child_node->children[0];
            ast_node id_children = typespec_child_node->children[1];
            if (type_children->type == SPLT_KWD_STRUCT)
                decl_extra_type = SPLE_STRUCT_DEC;
            else
            {
                decl_extra_type = SPLE_UNION_DEC;
            }
            decl_spec_type = (char *)(id_children->val);
        }
        // passing the params to the declaration below
    }

    // definition in function
    if (node->type == SPLT_PARAM_DEC || node->type == SPLT_DIR_DECLTN) // ParamDecltr(for each parameter of function) /
                                                                       // DirectDecl(declaration in the functionbody)
    {
        decl_extra_type = SPLE_NULL;
        ast_node typespec_child_node = (((node->children[0])->children[0])->children[0]);
        if (typespec_child_node->num_child == 0) // variable type
        {
            decl_entry_type = SPLE_VAR;
            decl_spec_type = splc_token2str(typespec_child_node->type);
        }
        else
        { // struct/union type
            decl_entry_type = SPLE_VAR;
            ast_node type_children = typespec_child_node->children[0];
            ast_node id_children = typespec_child_node->children[1];
            if (type_children->type == SPLT_KWD_STRUCT)
                decl_extra_type = SPLE_STRUCT_DEC;
            else
            {
                decl_extra_type = SPLE_UNION_DEC;
            }
            decl_spec_type = (char *)(id_children->val);
        }
        // pass the two params to the declaration below
    }

    // add declare variables into table:
    if (node->type == SPLT_DIR_DEC) // DirectDecltr (for variables)
    {
        char *var_name = (char *)((node->children[0])->val);
        // SPLC_FDIAG("%s %d %d %s",var_name, decl_entry_type, decl_extra_type, decl_spec_type);
        //  Type 3 --- redefined variable
        int var_is_redefined = 0;
        if (!in_struct)
        {
            for (int i = 0; i < tunit->nenvs; i++)
            {
                if (lut_exists(tunit->envs[i], var_name, SPLE_VAR))
                    var_is_redefined = 1;
            }
        }
        else
        {
            var_is_redefined = lut_exists(tunit->envs[tunit->nenvs - 1], var_name, SPLE_VAR);
        }
        if (var_is_redefined)
        {
            SPLC_FMSG(SPLM_ERR_SEM_3, node->location, "redefinition of variable `%s`", var_name);
        }
        else
        {
            SPLC_FDIAG("variable: %s %d %d %s", var_name, decl_entry_type, decl_extra_type, decl_spec_type);
            lut_insert(tunit->envs[(tunit->nenvs) - 1], var_name, decl_entry_type, decl_extra_type, decl_spec_type,
                       NULL, node, node->location);
        }
        return;
    }
    if (node->type == SPLT_DIR_FUNC_DEC) // DirectFunctionDecltr (for functions)
    {
        char *func_name = (char *)(((node->children[0])->children[0])->val);
        int func_is_redefined = 0;
        for (int i = 0; i < tunit->nenvs; i++)
        {
            if (lut_exists(tunit->envs[i], func_name, SPLE_FUNC))
                func_is_redefined = 1;
        }
        if (func_is_redefined)
        {
            SPLC_FMSG(SPLM_ERR_SEM_4, node->location, "redefinition of function '\033[1m%s\033[0m'", func_name);
        }
        else
        {
            SPLC_FDIAG("function: %s %d %d %s", func_name, decl_entry_type, decl_extra_type, decl_spec_type);
            lut_insert(tunit->envs[(tunit->nenvs) - 1], func_name, decl_entry_type, decl_extra_type, decl_spec_type,
                       NULL, node, node->location);
            lut_insert(tunit->envs[0], func_name, decl_entry_type, decl_extra_type, decl_spec_type, NULL, node,
                       node->location);
        }
        if (node->num_child == 2)
            legacy_ast_search(node->children[1], node, tunit, 0, decl_entry_type, decl_extra_type, decl_spec_type,
                              in_struct, in_expr);
        return;
    }

    // check expr
    if (node->type == SPLT_EXPR)
    {
        in_expr = 1;
    }

    // Type1 --- check variable is undefined
    if (node->type == SPLT_ID && in_expr)
    {
        char *var_name = (char *)(node->val);
        // check whether the variable is defined or not
        int var_is_defined = 0;
        lut_entry var_entry;
        for (int i = 0; i < tunit->nenvs; i++)
        {
            if (lut_exists(tunit->envs[i], var_name, SPLE_VAR))
            {
                var_is_defined = 1;
                var_entry = lut_find(tunit->envs[i], var_name, SPLE_VAR);
            }
        }
        if (!var_is_defined)
        {
            SPLC_FMSG(SPLM_ERR_SEM_1, node->location, "variable `%s` is undefined", var_name);
        }
    }

    // check errors when using functions
    if (node->type == SPLT_CALL_EXPR) // CallExpr
    {
        char *func_name = ((char *)(node->children[0])->val);
        int func_is_defined = 0;
        for (int i = 0; i < tunit->nenvs; i++)
        {
            if (lut_exists(tunit->envs[i], func_name, SPLE_FUNC))
                func_is_defined = 1;
        }
        int func_name_is_defined = 0;
        for (int i = 0; i < tunit->nenvs; i++)
        {
            if (lut_name_exists(tunit->envs[i], func_name))
                func_name_is_defined = 1;
        }
        if (!func_is_defined && !func_name_is_defined)
        {
            SPLC_FMSG(SPLM_ERR_SEM_2, node->location, "function %s is undefined", func_name);
        }
        else if (!func_is_defined && func_name_is_defined)
        {
            SPLC_FMSG(SPLM_ERR_SEM_11, node->location, "applying function invocation operator on name %s\n", func_name);
        }
    }

    // search children
    for (int i = 0; i < node->num_child; i++)
    {
        // get new_sym_table
        ast_node child = node->children[i];
        if (child->type == SPLT_STMT && find_stmt)
        {
            new_sym_table = 1;
            find_stmt = 0;
        }
        else
        {
            new_sym_table = 0;
        }

        // if the child is a Dot node, we will stop variable undefinition checking
        if (child->type == SPLT_DOT)
            in_expr = 0;

        if (node->type == SPLT_CALL_EXPR && child->type == SPLT_ID)
        {
            legacy_ast_search(child, node, tunit, new_sym_table, decl_entry_type, decl_extra_type, decl_spec_type,
                              in_struct, 0);
        }
        else
        {
            legacy_ast_search(child, node, tunit, new_sym_table, decl_entry_type, decl_extra_type, decl_spec_type,
                              in_struct, in_expr);
        }
    }

    // pop symbol table and link it to the node
    if (copy_new_sym_table)
    {
        lut_table top_sym_table = splc_pop_symtable(tunit);
        node->symtable = top_sym_table;
    }
}

expr_node expr_process_id(const ast_node node, splc_trans_unit tunit)
{
    // TODO: The id of lut_entry should be put in expr
    lut_entry ent = lut_find_in_tables(tunit->nenvs, tunit->envs, (char *)(node->val), SPLE_VAR);
    if (ent)
    {
        // TODO: construct expr_node of lut_entry correctly
        return ent->expr;
    }
    return NULL;
}

expr_node expr_process_literal(const ast_node node, splc_trans_unit tunit)
{
    expr_node result = expr_create_empty_node();
    splc_token_t type = node->children[0]->type;
    if (type == SPLT_LTR_INT)
    {
        result->type = EXPR_INT;
    }
    else if (type == SPLT_LTR_FLOAT)
    {
        result->type = EXPR_FLOAT;
    }
    else if (type == SPLT_LTR_CHAR)
    {
        result->type = EXPR_CHAR;
    }
    result->lvalue = 0; // set to non-lvalue
    return result;
}

expr_node expr_process_call_expr(const ast_node node, splc_trans_unit tunit)
{
    lut_entry func_ent = lut_find(tunit->envs[0], (char *)(node->children[0]->val), SPLE_FUNC);
    if (func_ent == NULL || func_ent->expr->type != EXPR_FUNC)
    {
        return NULL;
    }

    expr_node func_expr = func_ent->expr;

    ast_node arg_list = node->children[1];

    // minus 1 to skip the return value
    if (func_expr->num_arg - 1 != arg_list->num_child)
    {
        SPLC_FMSG(SPLM_ERR_SEM_9, arg_list->location, "invalid argument number, except %ld, got %ld",
                  func_expr->num_arg - 1, arg_list->num_child);
        return NULL;
    }

    for (int i = 0; i < arg_list->num_child; i++)
    {
        expr_node arg_expr = expr_process(arg_list->children[i], tunit);
        if (arg_expr != NULL)
        {
            if (!expr_type_checking(func_expr->args[i + 1], arg_expr))
            {
                SPLC_MSG(SPLM_ERR_SEM_9, arg_list->children[i]->location,
                          "invalid argument type");
            }
        }
    }

    return func_expr;
}

expr_node expr_process_dot(const ast_node node, splc_trans_unit tunit)
{
    expr_node struct_var_expr = expr_process(node->children[0], tunit);
    if (struct_var_expr && struct_var_expr->type == EXPR_STRUCT)
    {
        lut_entry struct_ent = lut_find_in_tables(tunit->nenvs, tunit->envs, struct_var_expr->id, SPLE_STRUCT_DEC);
        if (struct_ent)
        {
            lut_entry member_ent = lut_find(struct_ent->root->symtable, (char *)(node->children[2]->val), SPLE_VAR);
            if (member_ent)
            {
                return expr_deep_copy(member_ent->expr);
            }
            else
            {
                // no specific member in the struct declaration
                SPLC_MSG(SPLM_ERR_SEM_14, node->children[2]->location, "accessing an undefined structure member");
                return NULL;
            }
        }
        else
        {
            SPLC_MSG(SPLM_ERR_SEM_13, node->children[1]->location, "accessing members of a non-structure variable");
            return NULL;
        }
    }
    return NULL;
}

expr_node expr_process_eq(const ast_node node, splc_trans_unit tunit)
{
    // TODO type checking
    expr_node result = expr_create_empty_node();
    result->lvalue = 0;
    result->type = EXPR_INT;
    return result;
}

expr_node expr_process_array(const ast_node node, splc_trans_unit tunit)
{
    expr_node postfix = expr_process(node->children[0], tunit);
    if (postfix->type != EXPR_ARRAY)
    {
        SPLC_MSG(SPLM_ERR_SEM_10, node->children[1]->location, "cannot index on non-array variable");
        return NULL;
    }

    expr_node index = expr_process(node->children[2], tunit);
    if (index)
    {
        if (index->type != EXPR_INT)
        {
            SPLC_MSG(SPLM_ERR_SEM_12, node->children[2]->location, "array indexing with a non-integer type expression");
            return NULL;
        }
    }
    // The second argument is the type expression.
    return postfix->args[1];
}

expr_node expr_process_expr(const ast_node node, splc_trans_unit tunit)
{
    if (node->num_child == 1)
    {
        return expr_process(node->children[0], tunit);
    }
    else if (node->num_child == 2)
    {
        // unary
        int expr_idx = node->children[0]->type == SPLT_ID || node->children[0]->type == SPLT_EXPR ? 0 : 1;
        expr_node expr = expr_process_expr(node->children[expr_idx], tunit);
        if (!(expr->type == EXPR_INT || expr->type == EXPR_CHAR || expr->type == EXPR_FLOAT))
        {
            SPLC_MSG(SPLM_ERR_SEM_7, node->location, "unmatching operands");
        }
        expr_node result = expr_deep_copy(expr);
        result->lvalue = 0; // set to non-lvalue
        return result;
    }
    else if (node->num_child == 3)
    {
        if (node->children[1]->type == SPLT_DOT)
        {
            return expr_process_dot(node, tunit);
        }
        else if (node->children[1]->type == SPLT_EQ)
        {
            return expr_process_eq(node, tunit);
        }
        else
        {
            expr_node left = expr_process(node->children[0], tunit);
            expr_node right = expr_process(node->children[2], tunit);

            if (left == NULL || right == NULL)
            {
                return NULL;
            }

            // Process assign
            if (node->children[1]->type == SPLT_ASSIGN)
            {
                if (!left->lvalue)
                {
                    SPLC_MSG(SPLM_ERR_SEM_6, node->children[0]->location,
                             "rvalue appears on the left-hand side of the assignment operator");
                    return NULL;
                }
                else if (!expr_type_checking(left, right))
                {
                    SPLC_MSG(SPLM_ERR_SEM_5, node->location, "unmatching type on both sides of assignment");
                    return NULL;
                }
            }

            // For other binary operator
            if (!expr_type_checking(left, right))
            {

                SPLC_MSG(SPLM_ERR_SEM_7, node->location, "unmatching operand");
                return NULL;
            }

            if (!EXPR_IS_NUMERICAL(left->type))
            {
                SPLC_MSG(SPLM_ERR_UNIV, node->location, "unsupport operand");
                return NULL;
            }

            expr_node result = expr_deep_copy(left);
            result->lvalue = 0;
            return result;
        }
    }
    else if (node->num_child == 4)
    {
        if (node->children[1]->type == SPLT_LSB)
        {
            return expr_process_array(node, tunit);
        }
    }
}

expr_node expr_process(const ast_node node, splc_trans_unit tunit)
{
    if (node->symtable)
    {
        splc_push_existing_symtable(tunit, node->symtable);
    }

    else if (SPLT_IS_ID(node->type))
    {
        return expr_process_id(node, tunit);
    }
    else if (SPLT_IS_LITERAL(node->type))
    {
        return expr_process_literal(node, tunit);
    }

    // CallExpr
    else if (node->type == SPLT_CALL_EXPR)
    {
        return expr_process_call_expr(node, tunit);
    }

    else if (node->type == SPLT_EXPR)
    {
        // TODO
        return expr_process_expr(node, tunit);
    }
    else if (node->type == SPLT_INIT_DEC)
    {
        // TODO
    }

    for (int i = 0; i < node->num_child; i++)
    {
        expr_process(node->children[i], tunit);
    }

    if (node->symtable)
    {
        splc_pop_symtable(tunit);
    }

    return NULL;
}


void sem_analyze(splc_trans_unit tunit)
{
    // TODO(semantics): finish semantic analysis part
    // splcdiag("Semantic Analysis should be performed there.");
    legacy_ast_search(tunit->root, NULL, tunit, 0, SPLE_NULL, SPLE_NULL, NULL, 0, 0);
    expr_process(tunit->root, tunit);
}