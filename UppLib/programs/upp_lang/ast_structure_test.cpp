#include "ast_structure_test.hpp"

int ast_parser_get_next_node_index_no_parent(AST_Parser* parser)
{
    while (parser->next_free_node >= parser->nodes.size) {
        AST_Node node;
        node.type = AST_Node_Type::UNDEFINED;
        node.children = dynamic_array_create_empty<AST_Node_Index>(2);
        node.parent = -1;
        dynamic_array_push_back(&parser->nodes, node);
    }

    AST_Node* node = &parser->nodes[parser->next_free_node];
    parser->next_free_node++;
    node->parent = -1;
    dynamic_array_reset(&node->children);
    return parser->next_free_node - 1;
}

int ast_parser_get_next_node_index(AST_Parser* parser, AST_Node_Index parent_index)
{
    int index = ast_parser_get_next_node_index_no_parent(parser);
    AST_Node* node = &parser->nodes[index];
    node->parent = parent_index;
    if (parent_index != -1) {
        dynamic_array_push_back(&parser->nodes[parent_index].children, parser->next_free_node - 1);
    }
    return index;
}


AST_Parser_Checkpoint ast_parser_checkpoint_make(AST_Parser* parser, AST_Node_Index parent_index)
{
    AST_Parser_Checkpoint result;
    result.parser = parser;
    result.parent_index = parent_index;
    result.parent_child_count = parser->nodes.data[parent_index].children.size;
    result.rewind_token_index = parser->index;
    result.next_free_node_index = parser->next_free_node;
    return result;
}

void ast_parser_checkpoint_reset(AST_Parser_Checkpoint checkpoint) 
{
    checkpoint.parser->index = checkpoint.rewind_token_index;
    checkpoint.parser->next_free_node = checkpoint.next_free_node_index;
    if (checkpoint.parent_index != -1) { // This is the case if root
        DynamicArray<AST_Node_Index>* parent_childs = &checkpoint.parser->nodes.data[checkpoint.parent_index].children;
        dynamic_array_remove_range_ordered(parent_childs, checkpoint.parent_child_count, parent_childs->size); 
    }
}

bool ast_parser_test_next_token(AST_Parser* parser, Token_Type::ENUM type)
{
    if (parser->index >= parser->lexer->tokens.size) {
        return false;
    }
    if (parser->lexer->tokens[parser->index].type == type) {
        return true;
    }
    return false;
}

bool ast_parser_test_next_2_tokens(AST_Parser* parser, Token_Type::ENUM type1, Token_Type::ENUM type2)
{
    if (parser->index + 1 >= parser->lexer->tokens.size) {
        return false;
    }
    if (parser->lexer->tokens[parser->index].type == type1 && parser->lexer->tokens[parser->index + 1].type == type2) {
        return true;
    }
    return false;
}

bool ast_parser_test_next_3_tokens(AST_Parser* parser, Token_Type::ENUM type1, Token_Type::ENUM type2, Token_Type::ENUM type3)
{
    if (parser->index + 2 >= parser->lexer->tokens.size) {
        return false;
    }
    if (parser->lexer->tokens[parser->index].type == type1 &&
        parser->lexer->tokens[parser->index + 1].type == type2 &&
        parser->lexer->tokens[parser->index + 2].type == type3) {
        return true;
    }
    return false;
}

bool ast_parser_test_next_4_tokens(AST_Parser* parser, Token_Type::ENUM type1, Token_Type::ENUM type2, Token_Type::ENUM type3,
    Token_Type::ENUM type4)
{
    if (parser->index + 3 >= parser->lexer->tokens.size) {
        return false;
    }
    if (parser->lexer->tokens[parser->index].type == type1 &&
        parser->lexer->tokens[parser->index + 1].type == type2 &&
        parser->lexer->tokens[parser->index + 2].type == type3 &&
        parser->lexer->tokens[parser->index + 3].type == type4) 
    {
        return true;
    }
    return false;
}

bool ast_parser_test_next_5_tokens(AST_Parser* parser, Token_Type::ENUM type1, Token_Type::ENUM type2, Token_Type::ENUM type3,
    Token_Type::ENUM type4, Token_Type::ENUM type5)
{
    if (parser->index + 4 >= parser->lexer->tokens.size) {
        return false;
    }
    if (parser->lexer->tokens[parser->index].type == type1 &&
        parser->lexer->tokens[parser->index + 1].type == type2 &&
        parser->lexer->tokens[parser->index + 2].type == type3 &&
        parser->lexer->tokens[parser->index + 3].type == type4 &&
        parser->lexer->tokens[parser->index + 4].type == type5) {
        return true;
    }
    return false;
}

namespace Parse_Object_Type
{
    enum ENUM 
    {
        STATEMENT,
        STATEMENT_BLOCK,
        SINGLE_STATEMENT_OR_BLOCK,
        EXPRESSION,
        FUNCTION,
        NODE_NAME_ID,
        NODE_TYPE_ID,
        TOKEN,
        CHOICE,
        COMPLEX,
    };
}

struct Parse_Object
{
    Parse_Object_Type::ENUM object_type;
    Token_Type::ENUM token_type;
    AST_Node_Type::ENUM node_type;
    Array<Parse_Object> children;
};

Parse_Object parse_object_make(Parse_Object_Type::ENUM type) {
    Parse_Object result;
    result.object_type = type;
    return result;
}

Parse_Object parse_object_make_token(Token_Type::ENUM token_type) {
    Parse_Object result;
    result.object_type = Parse_Object_Type::TOKEN;
    result.token_type = token_type;
    return result;
}

Parse_Object parse_object_make_complex(AST_Node_Type::ENUM node_type, Array<Parse_Object> components)
{
    Parse_Object result;
    result.object_type = Parse_Object_Type::COMPLEX;
    result.node_type = node_type;
    result.children = components;
    return result;
}

Parse_Object parse_object_make_choice(Array<Parse_Object> components) {
    Parse_Object result;
    result.object_type = Parse_Object_Type::CHOICE;
    result.children = components;
    return result;
}

void parse_object_destroy(Parse_Object* object) {
    switch (object->object_type)
    {
    case Parse_Object_Type::CHOICE:
    case Parse_Object_Type::COMPLEX:
        array_destroy(&object->children);
        break;
    }
    return;
}

bool ast_parser_parse_statement(AST_Parser* parser, AST_Node_Index parent_index);
bool ast_parser_parse_statement_block(AST_Parser* parser, AST_Node_Index parent_index);
bool ast_parser_parse_single_statement_or_block(AST_Parser* parser, AST_Node_Index parent_index);
bool ast_parser_parse_expression(AST_Parser* parser, int parent_index);
bool ast_parser_parse_function(AST_Parser* parser, AST_Node_Index parent_index);
bool ast_parser_parse_objects(AST_Parser* parser, int parent_index, Parse_Object* object)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parent_index);
    bool success = false;

    switch (object->object_type)
    {
    case Parse_Object_Type::STATEMENT:
        success = !ast_parser_parse_statement(parser, parent_index);
        break;
    case Parse_Object_Type::STATEMENT_BLOCK:
        success = !ast_parser_parse_statement_block(parser, parent_index);
        break;
    case Parse_Object_Type::SINGLE_STATEMENT_OR_BLOCK:
        success = !ast_parser_parse_single_statement_or_block(parser, parent_index);
        break;
    case Parse_Object_Type::EXPRESSION:
        success = !ast_parser_parse_expression(parser, parent_index);
        break;
    case Parse_Object_Type::FUNCTION:
        success = !ast_parser_parse_function(parser, parent_index);
        break;
    case Parse_Object_Type::NODE_NAME_ID: {
        success = ast_parser_test_next_token(parser, Token_Type::IDENTIFIER);
        if (success) {
            parser->nodes[parent_index].name_id = parser->lexer->tokens[parser->index].attribute.identifier_number;
            parser->index++;
        }
        break;
    }
    case Parse_Object_Type::NODE_TYPE_ID: {
        success = ast_parser_test_next_token(parser, Token_Type::IDENTIFIER);
        if (success) {
            parser->nodes[parent_index].type_id = parser->lexer->tokens[parser->index].attribute.identifier_number;
            parser->index++;
        }
        break;
    }
    case Parse_Object_Type::TOKEN: {
        success = ast_parser_test_next_token(parser, object->token_type);
        if (success) {
            parser->index++;
        }
        break;
    }
    case Parse_Object_Type::CHOICE: 
    {
        success = false;
        for (int i = 0; i < object->children.size; i++) {
            if (ast_parser_parse_objects(parser, parent_index, &object->children[i])) {
                success = true;
                break;
            }
        }
        break;
    }
    case Parse_Object_Type::COMPLEX: {
        success = true;
        AST_Node_Index new_node_index = ast_parser_get_next_node_index(parser, parent_index);
        for (int i = 0; i < object->children.size; i++) {
            if (!ast_parser_parse_objects(parser, new_node_index, &object->children[i])) {
                success = false;
                break;
            }
        }
        if (success) {
            parser->nodes[new_node_index].type = object->node_type;
        }
        break;
    }
    }

    if (!success) {
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    }

    return true;
}

bool ast_parser_parse_expression(AST_Parser* parser, int parent_index);
bool ast_parser_parse_argument_block(AST_Parser* parser, AST_Node_Index parent_index)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parent_index);
    if (!ast_parser_test_next_token(parser, Token_Type::OPEN_PARENTHESIS)) {
        return false;
    }
    parser->index++;

    // TODO: Better error handling
    while (!ast_parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS))
    {
        if (!ast_parser_parse_expression(parser, parent_index)) {
            ast_parser_checkpoint_reset(checkpoint);
            return false;
        }
        if (ast_parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS)) {
            parser->index++;
            return true;
        }
        if (ast_parser_test_next_token(parser, Token_Type::COMMA)) {
            parser->index++;
            continue;
        }
        // Error
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    }
    parser->index++; // Skip )
    return true;
}

bool ast_parser_parse_expression(AST_Parser* parser, AST_Node_Index parent_index);
AST_Node_Index ast_parser_parse_expression_no_parents(AST_Parser* parser);
AST_Node_Index ast_parser_parse_expression_single_value(AST_Parser* parser)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, -1);

    // Cases: Function Call, Variable read, Literal Value, Unary Operation
    if (ast_parser_test_next_token(parser, Token_Type::OPEN_PARENTHESIS))
    {
        parser->index++;
        AST_Node_Index expr_index = ast_parser_parse_expression_no_parents(parser);
        if (expr_index == -1 || !ast_parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS)) {
            ast_parser_checkpoint_reset(checkpoint);
            return -1;
        }
        parser->index++;
        return expr_index;
    }

    int node_index = ast_parser_get_next_node_index_no_parent(parser);
    if (ast_parser_test_next_token(parser, Token_Type::IDENTIFIER))
    {
        parser->nodes[node_index].type = AST_Node_Type::EXPRESSION_VARIABLE_READ;
        parser->nodes[node_index].name_id = parser->lexer->tokens[parser->index].attribute.identifier_number;
        parser->index++;
        if (ast_parser_parse_argument_block(parser, node_index)) {
            parser->nodes[node_index].type = AST_Node_Type::EXPRESSION_FUNCTION_CALL;
        }
        return node_index;
    }
    else if (ast_parser_test_next_token(parser, Token_Type::INTEGER_LITERAL) ||
        ast_parser_test_next_token(parser, Token_Type::FLOAT_LITERAL) ||
        ast_parser_test_next_token(parser, Token_Type::BOOLEAN_LITERAL))
    {
        parser->nodes[node_index].type = AST_Node_Type::EXPRESSION_LITERAL;
        parser->index++;
        return node_index;
    }
    else if (ast_parser_test_next_token(parser, Token_Type::OP_MINUS))
    {
        parser->nodes[node_index].type = AST_Node_Type::EXPRESSION_UNARY_OPERATION_NEGATE;
        parser->index++;
        AST_Node_Index child_index = ast_parser_parse_expression_single_value(parser);
        if (child_index == -1) {
            ast_parser_checkpoint_reset(checkpoint);
            return -1;
        }
        dynamic_array_push_back(&parser->nodes[node_index].children, child_index);
        parser->nodes[child_index].parent = node_index;
        return true;
    }
    else if (ast_parser_test_next_token(parser, Token_Type::LOGICAL_NOT))
    {
        parser->nodes[node_index].type = AST_Node_Type::EXPRESSION_UNARY_OPERATION_NOT;
        parser->index++;
        AST_Node_Index child_index = ast_parser_parse_expression_single_value(parser);
        if (child_index == -1) {
            ast_parser_checkpoint_reset(checkpoint);
            return -1;
        }
        dynamic_array_push_back(&parser->nodes[node_index].children, child_index);
        parser->nodes[child_index].parent = node_index;
        return true;
    }

    ast_parser_checkpoint_reset(checkpoint);
    return -1;
}

bool ast_parser_parse_binary_operation(AST_Parser* parser, AST_Node_Type::ENUM* op_type, int* op_priority)
{
    /*
        Priority tree:
            0       ---     &&
            1       ---     ||
            2       ---     ==, !=
            3       ---     <, >, <=, >=
            4       ---     +, -
            5       ---     *, /
            6       ---     %
    */
    if (parser->index + 1 >= parser->lexer->tokens.size) return false;
    switch (parser->lexer->tokens[parser->index].type)
    {
    case Token_Type::LOGICAL_AND: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_AND;
        *op_priority = 0;
        break;
    }
    case Token_Type::LOGICAL_OR: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_OR;
        *op_priority = 1;
        break;
    }
    case Token_Type::COMPARISON_EQUAL: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_EQUAL;
        *op_priority = 2;
        break;
    }
    case Token_Type::COMPARISON_NOT_EQUAL: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_NOT_EQUAL;
        *op_priority = 2;
        break;
    }
    case Token_Type::COMPARISON_GREATER: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_GREATER;
        *op_priority = 3;
        break;
    }
    case Token_Type::COMPARISON_GREATER_EQUAL: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_GREATER_OR_EQUAL;
        *op_priority = 3;
        break;
    }
    case Token_Type::COMPARISON_LESS: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_LESS;
        *op_priority = 3;
        break;
    }
    case Token_Type::COMPARISON_LESS_EQUAL: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_LESS_OR_EQUAL;
        *op_priority = 3;
        break;
    }
    case Token_Type::OP_PLUS: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_ADDITION;
        *op_priority = 4;
        break;
    }
    case Token_Type::OP_MINUS: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_SUBTRACTION;
        *op_priority = 4;
        break;
    }
    case Token_Type::OP_STAR: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_MULTIPLICATION;
        *op_priority = 5;
        break;
    }
    case Token_Type::OP_SLASH: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_DIVISION;
        *op_priority = 5;
        break;
    }
    case Token_Type::OP_PERCENT: {
        *op_type = AST_Node_Type::EXPRESSION_BINARY_OPERATION_MODULO;
        *op_priority = 6;
        break;
    }
    default: {
        return false;
    }
    }

    parser->index++;
    return true;
}

int ast_parser_parse_expression_priority(AST_Parser* parser, AST_Node_Index node_index, int min_priority)
{
    int start_point = parser->index;
    int rewind_point = parser->index;

    bool first_run = true;
    int max_priority = 999;
    while (true)
    {
        AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parser->nodes[node_index].parent);

        int first_op_priority;
        AST_Node_Type::ENUM first_op_type;
        if (!ast_parser_parse_binary_operation(parser, &first_op_type, &first_op_priority)) {
            break;
        }
        if (first_op_priority < max_priority) {
            max_priority = first_op_priority;
        }
        if (first_op_priority < min_priority) {
            parser->index = rewind_point; // Undo the binary operation, maybe just do -1
            break;
        }

        AST_Node_Index operator_node = ast_parser_get_next_node_index_no_parent(parser);
        AST_Node_Index right_operand_index = ast_parser_parse_expression_single_value(parser);
        if (right_operand_index == -1) {
            ast_parser_checkpoint_reset(checkpoint);
            break;
        }
        rewind_point = parser->index;

        int second_op_priority;
        AST_Node_Type::ENUM second_op_type;
        bool second_op_exists = ast_parser_parse_binary_operation(parser, &second_op_type, &second_op_priority);
        if (second_op_exists)
        {
            parser->index--;
            if (second_op_priority > max_priority) {
                right_operand_index = ast_parser_parse_expression_priority(parser, right_operand_index, second_op_priority);
            }
        }

        //
        parser->nodes[node_index].parent = operator_node;
        dynamic_array_push_back(&parser->nodes[operator_node].children, node_index);
        parser->nodes[right_operand_index].parent = operator_node;
        dynamic_array_push_back(&parser->nodes[operator_node].children, right_operand_index);
        parser->nodes[operator_node].type = first_op_type;

        node_index = operator_node;
        if (!second_op_exists) break;
    }

    return node_index;
}

AST_Node_Index ast_parser_parse_expression_no_parents(AST_Parser* parser)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, -1);
    int single_value_index = ast_parser_parse_expression_single_value(parser);
    if (single_value_index == -1) {
        ast_parser_checkpoint_reset(checkpoint);
        return -1;
    }
    AST_Node_Index op_tree_root_index = ast_parser_parse_expression_priority(parser, single_value_index, 0);
    return op_tree_root_index;
}

bool ast_parser_parse_expression(AST_Parser* parser, int parent_index)
{
    AST_Node_Index op_tree_root_index = ast_parser_parse_expression_no_parents(parser);
    if (op_tree_root_index == -1) return false;

    dynamic_array_push_back(&parser->nodes[parent_index].children, op_tree_root_index);
    parser->nodes[op_tree_root_index].parent = parent_index;
    return true;
}

bool ast_parser_parse_statement_block(AST_Parser* parser, AST_Node_Index parent_index);
bool ast_parser_parse_statement(AST_Parser* parser, AST_Node_Index parent_index);

bool ast_parser_parse_single_statement_or_block(AST_Parser* parser, AST_Node_Index parent_index)
{
    if (ast_parser_parse_statement_block(parser, parent_index)) {
        return true;
    }

    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parent_index);
    int node_index = ast_parser_get_next_node_index(parser, parent_index);
    if (!ast_parser_parse_statement(parser, node_index)) {
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    }

    return true;
}

bool ast_parser_parse_statement(AST_Parser* parser, AST_Node_Index parent_index)
{
    Parse_Object statement_parse_tree = parse_object_make_choice(
        array_create_from_list({
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_VARIABLE_DEFINITION,
                array_create_from_list({
                    parse_object_make(Parse_Object_Type::NODE_NAME_ID),
                    parse_object_make_token(Token_Type::COLON),
                    parse_object_make(Parse_Object_Type::NODE_TYPE_ID),
                    parse_object_make_token(Token_Type::SEMICOLON)
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_VARIABLE_DEFINE_ASSIGN,
                array_create_from_list({
                     parse_object_make(Parse_Object_Type::NODE_NAME_ID),
                     parse_object_make_token(Token_Type::COLON),
                     parse_object_make(Parse_Object_Type::NODE_TYPE_ID),
                     parse_object_make_token(Token_Type::OP_ASSIGNMENT),
                     parse_object_make(Parse_Object_Type::EXPRESSION),
                     parse_object_make_token(Token_Type::SEMICOLON)
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_VARIABLE_DEFINE_INFER,
                array_create_from_list({
                     parse_object_make(Parse_Object_Type::NODE_NAME_ID),
                     parse_object_make_token(Token_Type::INFER_ASSIGN),
                     parse_object_make(Parse_Object_Type::EXPRESSION),
                     parse_object_make_token(Token_Type::SEMICOLON)
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_VARIABLE_ASSIGNMENT,
                array_create_from_list({
                    parse_object_make(Parse_Object_Type::NODE_NAME_ID),
                    parse_object_make_token(Token_Type::OP_ASSIGNMENT),
                    parse_object_make(Parse_Object_Type::EXPRESSION),
                    parse_object_make_token(Token_Type::SEMICOLON)
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_EXPRESSION,
                array_create_from_list({
                    parse_object_make(Parse_Object_Type::EXPRESSION),
                    parse_object_make_token(Token_Type::SEMICOLON)
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_BREAK,
                array_create_from_list({
                    parse_object_make_token(Token_Type::BREAK),
                    parse_object_make_token(Token_Type::SEMICOLON),
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_CONTINUE,
                array_create_from_list({
                    parse_object_make_token(Token_Type::CONTINUE),
                    parse_object_make_token(Token_Type::SEMICOLON),
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_RETURN,
                array_create_from_list({
                    parse_object_make_token(Token_Type::RETURN),
                    parse_object_make(Parse_Object_Type::EXPRESSION),
                    parse_object_make_token(Token_Type::SEMICOLON),
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_WHILE,
                array_create_from_list({
                    parse_object_make_token(Token_Type::WHILE),
                    parse_object_make(Parse_Object_Type::EXPRESSION),
                    parse_object_make(Parse_Object_Type::SINGLE_STATEMENT_OR_BLOCK),
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_IF_ELSE,
                array_create_from_list({
                    parse_object_make_token(Token_Type::IF),
                    parse_object_make(Parse_Object_Type::EXPRESSION),
                    parse_object_make(Parse_Object_Type::SINGLE_STATEMENT_OR_BLOCK),
                    parse_object_make_token(Token_Type::ELSE),
                    parse_object_make(Parse_Object_Type::SINGLE_STATEMENT_OR_BLOCK),
                })
            ),
            parse_object_make_complex(
                AST_Node_Type::STATEMENT_IF,
                array_create_from_list({
                    parse_object_make_token(Token_Type::IF),
                    parse_object_make(Parse_Object_Type::EXPRESSION),
                    parse_object_make(Parse_Object_Type::SINGLE_STATEMENT_OR_BLOCK),
                })
            ),
            })
    );
    SCOPE_EXIT(parse_object_destroy(&statement_parse_tree));

    return ast_parser_parse_objects(parser, parent_index, &statement_parse_tree);
}

bool ast_parser_parse_statement_block(AST_Parser* parser, AST_Node_Index parent_index)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parent_index);
    int node_index = ast_parser_get_next_node_index(parser, parent_index);

    parser->nodes[node_index].type = AST_Node_Type::STATEMENT_BLOCK;
    if (!ast_parser_test_next_token(parser, Token_Type::OPEN_BRACES)) {
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    }
    parser->index++;

    while (!ast_parser_test_next_token(parser, Token_Type::CLOSED_BRACES))
    {
        if (!ast_parser_parse_statement(parser, node_index))
        {
            ast_parser_checkpoint_reset(checkpoint);
            return false;
        }
    }
    parser->index++;

    return true;
}

bool ast_parser_parse_parameter_block(AST_Parser* parser, AST_Node_Index parent_index)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parent_index);
    int node_index = ast_parser_get_next_node_index(parser, parent_index);

    parser->nodes[node_index].type = AST_Node_Type::PARAMETER_BLOCK;
    if (!ast_parser_test_next_token(parser, Token_Type::OPEN_PARENTHESIS)) {
        return false;
    }
    parser->index++;
    if (ast_parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS)) {
        parser->index++;
        return true;
    }

    // TODO: Better error handling
    while (true)
    {
        if (!ast_parser_test_next_3_tokens(parser, Token_Type::IDENTIFIER, Token_Type::COLON, Token_Type::IDENTIFIER)) {
            ast_parser_checkpoint_reset(checkpoint);
            return false;
        }
        AST_Node_Index parameter_index = ast_parser_get_next_node_index(parser, node_index);
        AST_Node* node = &parser->nodes[parameter_index];
        node->type = AST_Node_Type::PARAMETER;
        node->name_id = parser->lexer->tokens[parser->index].attribute.identifier_number;
        node->type_id = parser->lexer->tokens[parser->index + 2].attribute.identifier_number;
        parser->index += 3;

        if (ast_parser_test_next_token(parser, Token_Type::COMMA)) {
            parser->index++;
            continue;
        }
        else if (ast_parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS)) {
            parser->index++;
            return true;
        }
        else {
            ast_parser_checkpoint_reset(checkpoint);
            return false;
        }
    }

    panic("Should not happen!\n");
    ast_parser_checkpoint_reset(checkpoint);
    return false;
}

bool ast_parser_parse_function(AST_Parser* parser, AST_Node_Index parent_index)
{
    AST_Parser_Checkpoint checkpoint = ast_parser_checkpoint_make(parser, parent_index);
    int node_index = ast_parser_get_next_node_index(parser, parent_index);
    parser->nodes[node_index].type = AST_Node_Type::FUNCTION;

    // Parse Function start
    if (!ast_parser_test_next_2_tokens(parser, Token_Type::IDENTIFIER, Token_Type::DOUBLE_COLON)) {
        return false;
    }
    parser->nodes[node_index].name_id = parser->lexer->tokens[parser->index].attribute.identifier_number;
    parser->index += 2;

    // Parse paramenters
    if (!ast_parser_parse_parameter_block(parser, node_index)) {
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    }

    // Parse Return type
    if (!ast_parser_test_next_2_tokens(parser, Token_Type::ARROW, Token_Type::IDENTIFIER)) {
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    }
    parser->nodes[node_index].type_id = parser->lexer->tokens[parser->index + 1].attribute.identifier_number;
    parser->index += 2;

    // Parse statements
    if (!ast_parser_parse_statement_block(parser, node_index)) {
        ast_parser_checkpoint_reset(checkpoint);
        return false;
    };

    return true;
}

// @IDEA Something along those lines for error handling
void ast_parser_handle_function_header_parsing_error() {

}

// @IDEA Maybe i want one function that calls itself recursively, taking an AST_Node_Type
void ast_parser_parse_root(AST_Parser* parser)
{
    // TODO: Do error handling if this function parsing fails
    int root_index = ast_parser_get_next_node_index(parser, -1);
    parser->nodes[root_index].type = AST_Node_Type::ROOT;
    while (ast_parser_parse_function(parser, root_index) && parser->index < parser->lexer->tokens.size) {}
}

AST_Parser ast_parser_parse(Lexer* lexer)
{
    AST_Parser parser;
    parser.index = 0;
    parser.lexer = lexer;
    parser.nodes = dynamic_array_create_empty<AST_Node>(1024);
    parser.token_mapping = dynamic_array_create_empty<Token_Range>(1024);
    parser.intermediate_errors = dynamic_array_create_empty<Parser_Error>(16);
    parser.unresolved_errors = dynamic_array_create_empty<Parser_Error>(16);
    parser.next_free_node = 0;

    ast_parser_parse_root(&parser);
    // Trim arrays
    dynamic_array_remove_range_ordered(&parser.nodes, parser.next_free_node, parser.nodes.size);
    // Do something with the errors

    return parser;
}

void ast_parser_destroy(AST_Parser* parser)
{
    dynamic_array_destroy(&parser->intermediate_errors);
    dynamic_array_destroy(&parser->unresolved_errors);
    dynamic_array_destroy(&parser->token_mapping);
    dynamic_array_destroy(&parser->nodes);
}

String ast_node_type_to_string(AST_Node_Type::ENUM type)
{
    switch (type)
    {
    case AST_Node_Type::ROOT: return string_create_static("ROOT");
    case AST_Node_Type::FUNCTION: return string_create_static("FUNCTION");
    case AST_Node_Type::PARAMETER_BLOCK: return string_create_static("PARAMETER_BLOCK");
    case AST_Node_Type::PARAMETER: return string_create_static("PARAMETER");
    case AST_Node_Type::STATEMENT_BLOCK: return string_create_static("STATEMENT_BLOCK");
    case AST_Node_Type::STATEMENT_IF: return string_create_static("STATEMENT_IF");
    case AST_Node_Type::STATEMENT_IF_ELSE: return string_create_static("STATEMENT_IF_ELSE");
    case AST_Node_Type::STATEMENT_WHILE: return string_create_static("STATEMENT_WHILE");
    case AST_Node_Type::STATEMENT_BREAK: return string_create_static("STATEMENT_BREAK");
    case AST_Node_Type::STATEMENT_CONTINUE: return string_create_static("STATEMENT_CONTINUE");
    case AST_Node_Type::STATEMENT_RETURN: return string_create_static("STATEMENT_RETURN");
    case AST_Node_Type::STATEMENT_EXPRESSION: return string_create_static("STATEMENT_EXPRESSION");
    case AST_Node_Type::STATEMENT_VARIABLE_ASSIGNMENT: return string_create_static("STATEMENT_VARIABLE_ASSIGNMENT");
    case AST_Node_Type::STATEMENT_VARIABLE_DEFINITION: return string_create_static("STATEMENT_VARIABLE_DEFINITION");
    case AST_Node_Type::STATEMENT_VARIABLE_DEFINE_ASSIGN: return string_create_static("STATEMENT_VARIABLE_DEFINE_ASSIGN");
    case AST_Node_Type::STATEMENT_VARIABLE_DEFINE_INFER: return string_create_static("STATEMENT_VARIABLE_DEFINE_INFER");
    case AST_Node_Type::EXPRESSION_LITERAL: return string_create_static("EXPRESSION_LITERAL");
    case AST_Node_Type::EXPRESSION_FUNCTION_CALL: return string_create_static("EXPRESSION_FUNCTION_CALL");
    case AST_Node_Type::EXPRESSION_VARIABLE_READ: return string_create_static("EXPRESSION_VARIABLE_READ");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_ADDITION: return string_create_static("EXPRESSION_BINARY_OPERATION_ADDITION");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_SUBTRACTION: return string_create_static("EXPRESSION_BINARY_OPERATION_SUBTRACTION");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_DIVISION: return string_create_static("EXPRESSION_BINARY_OPERATION_DIVISION");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_MULTIPLICATION: return string_create_static("EXPRESSION_BINARY_OPERATION_MULTIPLICATION");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_MODULO: return string_create_static("EXPRESSION_BINARY_OPERATION_MODULO");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_AND: return string_create_static("EXPRESSION_BINARY_OPERATION_AND");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_OR: return string_create_static("EXPRESSION_BINARY_OPERATION_OR");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_EQUAL: return string_create_static("EXPRESSION_BINARY_OPERATION_EQUAL");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_NOT_EQUAL: return string_create_static("EXPRESSION_BINARY_OPERATION_NOT_EQUAL");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_LESS: return string_create_static("EXPRESSION_BINARY_OPERATION_LESS");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_LESS_OR_EQUAL: return string_create_static("EXPRESSION_BINARY_OPERATION_LESS_OR_EQUAL");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_GREATER: return string_create_static("EXPRESSION_BINARY_OPERATION_GREATER");
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_GREATER_OR_EQUAL: return string_create_static("EXPRESSION_BINARY_OPERATIONGREATER_OR_EQUAL");
    case AST_Node_Type::EXPRESSION_UNARY_OPERATION_NEGATE: return string_create_static("EXPRESSION_UNARY_OPERATION_NEGATE");
    case AST_Node_Type::EXPRESSION_UNARY_OPERATION_NOT: return string_create_static("EXPRESSION_UNARY_OPERATION_NOT");
    case AST_Node_Type::UNDEFINED: return string_create_static("UNDEFINED");
    }
    return string_create_static("SHOULD NOT FUCKING HAPPEN MOTHERFUCKER");
}

void ast_node_append_to_string(AST_Parser* parser, int node_index, String* string, int indentation_lvl)
{
    AST_Node* node = &parser->nodes[node_index];
    for (int j = 0; j < indentation_lvl; j++) {
        string_append_formated(string, "  ");
    }
    String type_str = ast_node_type_to_string(node->type);
    string_append_string(string, &type_str);
    string_append_formated(string, "\n");
    for (int i = 0; i < node->children.size; i++) {
        ast_node_append_to_string(parser, node->children[i], string, indentation_lvl + 1);
    }
}

void ast_parser_append_to_string(AST_Parser* parser, String* string) {
    ast_node_append_to_string(parser, 0, string, 0);
}

