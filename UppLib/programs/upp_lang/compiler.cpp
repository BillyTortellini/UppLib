#include "compiler.hpp"

#include <cstring>

bool token_type_is_keyword(Token_Type::ENUM type)
{
    switch (type)
    {
    case Token_Type::IF: return true;
    case Token_Type::ELSE: return true;
    case Token_Type::FOR: return true;
    case Token_Type::WHILE: return true;
    case Token_Type::CONTINUE: return true;
    case Token_Type::BREAK: return true;
    case Token_Type::RETURN: return true;
    }
    return false;
}

TokenAttribute token_attribute_make_empty() {
    TokenAttribute result;
    result.integer_value = 67676767;
    return result;
}

Token token_make(Token_Type::ENUM type, TokenAttribute attribute, int line_num, int char_pos, int char_len, int code_index)
{
    Token result;
    result.type = type;
    result.attribute = attribute;
    result.line_number = line_num;
    result.character_position = char_pos;
    result.lexem_length = char_len;
    result.source_code_index = code_index;
    return result;
}

bool skip_comments(String* code, int* index, int* character_pos, int* line_number)
{
    if (*index + 1 >= code->size) return false;
    if (code->characters[*index] == '/' && code->characters[*index + 1] == '/') {
        while (*index < code->size && code->characters[*index] != '\n') {
            *index = *index + 1;
            *character_pos = *character_pos + 1;
        }
        *index = *index + 1;
        *character_pos = 0;
        *line_number = *line_number + 1;
        return true;
    }

    if (code->characters[*index] == '/' && code->characters[*index + 1] == '*') 
    {
        *character_pos = *character_pos + 2;
        *index = *index + 2;
        int comment_depth = 1;
        while (*index + 1 < code->size) 
        {
            char current = code->characters[*index];
            char next = code->characters[*index+1];
            if (current == '/' && next == '*') {
                comment_depth++;
                *index = *index + 2;
                *character_pos = *character_pos + 2;
                continue;
            }
            if (current == '*' && next == '/') {
                comment_depth--;
                *index = *index + 2;
                *character_pos = *character_pos + 2;
                if (comment_depth == 0) break;
                continue;
            }

            *index = *index + 1;
            if (current == '\n') {
                *character_pos = 0;
                *line_number = *line_number + 1;
            }
            else {
                *character_pos = *character_pos + 1;
            }
        }
        return true;
    }
    return false;
}

void skip_whitespace_and_comments(String* code, int* index, int* character_pos, int* line_number)
{
    while (*index < code->size && string_contains_character(string_create_static("\t \r\n/"), code->characters[*index]))
    {
        if (skip_comments(code, index, character_pos, line_number)) continue;
        else if (code->characters[*index] == '/') break;
        (*character_pos)++;
        if (code->characters[*index] == '\n') {
            *character_pos = 0;
            *line_number = *line_number + 1;
        }
        *index = *index + 1;
    }
}

bool character_is_digit(int c) {
    return (c >= '0' && c <= '9');
}

bool character_is_letter(int c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z');
}

bool string_equals_cstring(String* string, const char* compare) {
    if (strcmp(string->characters, compare) == 0) {
        return true;
    }
    return false;
}

LexerResult lexer_parse_string(String* code)
{
    String identifier_string = string_create_empty(256);
    SCOPE_EXIT(string_destroy(&identifier_string));

    Hashtable<String, int> identifier_index_lookup_table = hashtable_create_empty<String, int>(2048, &string_calculate_hash, &string_equals);
    DynamicArray<String> identifiers = dynamic_array_create_empty<String>(code->size);
    DynamicArray<Token> tokens = dynamic_array_create_empty<Token>(code->size);
    int index = 0;
    int character_pos = 0;
    int line_number = 0;
    bool has_errors = false;
    while (index < code->size)
    {
        // Advance index
        skip_whitespace_and_comments(code, &index, &character_pos, &line_number);
        if (index >= code->size) {
            break;
        }

        // Get current character
        int current_character = code->characters[index];
        int next_character = -1;
        if (index + 1 < code->size) {
            next_character = code->characters[index + 1];
        }

        switch (current_character)
        {
            // Check for single symbols
        case '.':
            dynamic_array_push_back(&tokens, token_make(Token_Type::DOT, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case ';':
            dynamic_array_push_back(&tokens, token_make(Token_Type::SEMICOLON, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case ',':
            dynamic_array_push_back(&tokens, token_make(Token_Type::COMMA, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '(':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OPEN_PARENTHESIS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case ')':
            dynamic_array_push_back(&tokens, token_make(Token_Type::CLOSED_PARENTHESIS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '{':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OPEN_BRACES, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '}':
            dynamic_array_push_back(&tokens, token_make(Token_Type::CLOSED_BRACES, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '[':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OPEN_BRACKETS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case ']':
            dynamic_array_push_back(&tokens, token_make(Token_Type::CLOSED_BRACKETS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '+':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OP_PLUS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '*':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OP_STAR, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '/':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OP_SLASH, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '%':
            dynamic_array_push_back(&tokens, token_make(Token_Type::OP_PERCENT, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
            // Check for ambiguities between one and two characters (< and <=, = and ==, ! and !=, ...)
        case '=':
            if (next_character == '=') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::COMPARISON_EQUAL, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::OP_ASSIGNMENT, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '-':
            if (next_character == '>') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::ARROW, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::OP_MINUS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '<':
            if (next_character == '=') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::COMPARISON_LESS_EQUAL, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::COMPARISON_LESS, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '>':
            if (next_character == '=') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::COMPARISON_GREATER_EQUAL, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::COMPARISON_GREATER, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '!':
            if (next_character == '=') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::COMPARISON_NOT_EQUAL, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::LOGICAL_NOT, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '&':
            if (next_character == '&') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::LOGICAL_AND, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::LOGICAL_BITWISE_AND, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case '|':
            if (next_character == '|') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::LOGICAL_OR, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::LOGICAL_BITWISE_OR, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        case ':':
            if (next_character == ':') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::DOUBLE_COLON, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            else if (next_character == '=') {
                dynamic_array_push_back(&tokens, token_make(Token_Type::INFER_ASSIGN, token_attribute_make_empty(), line_number, character_pos, 2, index));
                index += 2;
                character_pos += 2;
                continue;
            }
            dynamic_array_push_back(&tokens, token_make(Token_Type::COLON, token_attribute_make_empty(), line_number, character_pos, 1, index));
            character_pos++;
            index++;
            continue;
        }

        // Constants, Identifier and Keywords
        // Parse Numbers
        if (current_character >= '0' && current_character <= '9')
        {
            int pre_comma_start_index = index;
            int pre_comma_end_index = index;
            // Parse number characters
            while (pre_comma_end_index < code->size && code->characters[pre_comma_end_index] >= '0' && code->characters[pre_comma_end_index] <= '9') {
                pre_comma_end_index++;
            }
            // TODO: Parse float numbers
            int value = 0;
            for (int i = pre_comma_start_index; i < pre_comma_end_index; i++) {
                int num_value = code->characters[i] - '0'; // 0 to 9
                value = (value * 10) + num_value;
            }
            // Add number
            TokenAttribute attribute;
            attribute.integer_value = value;
            int character_length = pre_comma_end_index - pre_comma_start_index;
            dynamic_array_push_back(&tokens, token_make(Token_Type::CONSTANT_INT, attribute, line_number, character_pos, character_length, index));
            index += character_length;
            character_pos += character_length;
            continue;
        }

        // Identifiers, keywords or error
        if (!character_is_letter(code->characters[index]))
        {
            // Error, parse till next Delimiter
            int error_end_index = index;
            while (error_end_index < code->size && !(
                code->characters[error_end_index] == ';' ||
                code->characters[error_end_index] == ',' ||
                code->characters[error_end_index] == '.' ||
                code->characters[error_end_index] == '(' ||
                code->characters[error_end_index] == ')' ||
                code->characters[error_end_index] == '{' ||
                code->characters[error_end_index] == '}' ||
                code->characters[error_end_index] == '[' ||
                code->characters[error_end_index] == ']' ||
                code->characters[error_end_index] == '=' ||
                code->characters[error_end_index] == '+' ||
                code->characters[error_end_index] == '*' ||
                code->characters[error_end_index] == '%' ||
                code->characters[error_end_index] == '-' ||
                code->characters[error_end_index] == '/' ||
                code->characters[error_end_index] == '\n' ||
                code->characters[error_end_index] == ' ' ||
                code->characters[error_end_index] == '\r' ||
                code->characters[error_end_index] == '\t' ||
                code->characters[error_end_index] == '!'
                )) {
                error_end_index++;
            }
            has_errors = true;
            error_end_index--;
            int error_length = error_end_index - index + 1;
            dynamic_array_push_back(&tokens, token_make(Token_Type::ERROR_TOKEN, token_attribute_make_empty(), line_number, character_pos, error_length, index));
            index += error_length;
            character_pos += error_length;
            continue;
        }

        // Parse identifier/keyword
        {
            int identifier_end_index = index;
            while (identifier_end_index < code->size && (
                character_is_letter(code->characters[identifier_end_index]) ||
                character_is_digit(code->characters[identifier_end_index]) ||
                code->characters[identifier_end_index] == '_'
                )) {
                identifier_end_index++;
            }
            identifier_end_index--;

            int identifier_string_length = identifier_end_index - index + 1;
            string_reserve(&identifier_string, identifier_string_length);
            memory_copy(identifier_string.characters, code->characters + index, identifier_string_length + 1);
            identifier_string.size = identifier_string_length;
            identifier_string.characters[identifier_string.size] = 0;

            // Check if identifier is a keyword
            if (string_equals_cstring(&identifier_string, "if")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::IF, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else if (string_equals_cstring(&identifier_string, "else")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::ELSE, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else if (string_equals_cstring(&identifier_string, "for")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::FOR, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else if (string_equals_cstring(&identifier_string, "while")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::WHILE, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else if (string_equals_cstring(&identifier_string, "continue")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::CONTINUE, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else if (string_equals_cstring(&identifier_string, "break")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::BREAK, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else if (string_equals_cstring(&identifier_string, "return")) {
                dynamic_array_push_back(&tokens, token_make(Token_Type::RETURN, token_attribute_make_empty(),
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
            else {
                // Identifier is a keyword
                TokenAttribute attribute;
                int* identifier_id = hashtable_find_element(&identifier_index_lookup_table, identifier_string);
                if (identifier_id != 0) {
                    attribute.identifier_number = *identifier_id;
                }
                else {
                    String identifier_string_copy = string_create(identifier_string.characters);
                    dynamic_array_push_back(&identifiers, identifier_string_copy);
                    attribute.identifier_number = identifiers.size - 1;
                    // Identifer needs to be added to table
                    hashtable_insert_element(&identifier_index_lookup_table, identifier_string_copy, attribute.identifier_number);
                }

                // Add token
                dynamic_array_push_back(&tokens, token_make(Token_Type::IDENTIFIER, attribute,
                    line_number, character_pos, identifier_string_length, index));
                index += identifier_string_length;
                character_pos += identifier_string_length;
            }
        }
    }

    LexerResult result;
    result.has_errors = has_errors;
    result.identifiers = identifiers;
    result.tokens = tokens;
    result.identifier_index_lookup_table = identifier_index_lookup_table;
    return result;
}

void lexer_result_destroy(LexerResult* result)
{
    for (int i = 0; i < result->identifiers.size; i++) {
        string_destroy(&result->identifiers[i]);
    }
    dynamic_array_destroy(&result->identifiers);
    dynamic_array_destroy(&result->tokens);
    hashtable_destroy(&result->identifier_index_lookup_table);
}

String lexer_result_identifer_to_string(LexerResult* result, int index) {
    return result->identifiers[index];
}

const char* tokentype_to_string(Token_Type::ENUM type)
{
    switch (type)
    {
    case Token_Type::IF: return "IF";
    case Token_Type::ELSE: return "ELSE";
    case Token_Type::FOR: return "FOR";
    case Token_Type::WHILE: return "WHILE";
    case Token_Type::CONTINUE: return "CONTINUE";
    case Token_Type::BREAK: return "BREAK";
    case Token_Type::DOT: return "DOT";
    case Token_Type::COLON: return "COLON";
    case Token_Type::COMMA: return "COMMA";
    case Token_Type::DOUBLE_COLON: return "DOUBLE_COLON";
    case Token_Type::INFER_ASSIGN: return "INFER_ASSIGN";
    case Token_Type::ARROW: return "ARROW";
    case Token_Type::SEMICOLON: return "SEMICOLON";
    case Token_Type::OPEN_PARENTHESIS: return "OPEN_BRACKET";
    case Token_Type::CLOSED_PARENTHESIS: return "CLOSED_BRACKET";
    case Token_Type::OPEN_BRACES: return "OPEN_CURLY_BRACKET";
    case Token_Type::CLOSED_BRACES: return "CLOSED_CURLY_BRACKET";
    case Token_Type::OPEN_BRACKETS: return "OPEN_SQUARE_BRACKET";
    case Token_Type::CLOSED_BRACKETS: return "CLOSED_SQUARE_BRACKET";
    case Token_Type::OP_ASSIGNMENT: return "OP_ASSIGNMENT";
    case Token_Type::OP_PLUS: return "OP_PLUS";
    case Token_Type::OP_MINUS: return "OP_MINUS";
    case Token_Type::OP_SLASH: return "OP_SLASH";
    case Token_Type::OP_STAR: return "OP_STAR";
    case Token_Type::OP_PERCENT: return "OP_PERCENT";
    case Token_Type::COMPARISON_LESS: return "COMPARISON_LESS";
    case Token_Type::COMPARISON_LESS_EQUAL: return "COMPARISON_LESS_EQUAL";
    case Token_Type::COMPARISON_GREATER: return "COMPARISON_GREATER";
    case Token_Type::COMPARISON_GREATER_EQUAL: return "COMPARISON_GREATER_EQUAL";
    case Token_Type::COMPARISON_EQUAL: return "COMPARISON_EQUAL";
    case Token_Type::COMPARISON_NOT_EQUAL: return "COMPARISON_NOT_EQUAL";
    case Token_Type::LOGICAL_AND: return "LOGICAL_AND";
    case Token_Type::LOGICAL_OR: return "LOGICAL_OR";
    case Token_Type::LOGICAL_BITWISE_AND: return "LOGICAL_BITWISE_AND";
    case Token_Type::LOGICAL_BITWISE_OR: return "LOGICAL_BITWISE_OR";
    case Token_Type::LOGICAL_NOT: return "LOGICAL_NOT";
    case Token_Type::CONSTANT_INT: return "CONSTANT_INT";
    case Token_Type::CONSTANT_FLOAT: return "CONSTANT_FLOAT";
    case Token_Type::CONSTANT_DOUBLE: return "CONSTANT_DOUBLE";
    case Token_Type::IDENTIFIER: return "IDENTIFIER";
    case Token_Type::ERROR_TOKEN: return "ERROR_TOKE";
    }
    return "TOKEN_NOT_KNOWN";
}

void lexer_result_print(LexerResult* result)
{
    String msg = string_create_empty(256);
    SCOPE_EXIT(string_destroy(&msg));
    string_append_formated(&msg, "Tokens: \n");
    for (int i = 0; i < result->tokens.size; i++) {
        Token token = result->tokens[i];
        string_append_formated(&msg, "\t %s (Line %d, Pos %d, Length %d)",
            tokentype_to_string(token.type), token.line_number, token.character_position, token.lexem_length);
        if (token.type == Token_Type::IDENTIFIER) {
            string_append_formated(&msg, " = %s", result->identifiers.data[token.attribute.identifier_number].characters);
        }
        else if (token.type == Token_Type::CONSTANT_INT) {
            string_append_formated(&msg, " = %d", token.attribute.integer_value);
        }
        string_append_formated(&msg, "\n");
    }
    logg("\n%s\n", msg.characters);
}

/*
    Wie schaut die syntax aus?

    Variablen, Expressions und Statements
    x : int = expression;
    x : int;
    x := 5;
    x :: (int)5; // Constant int, i dont think i want this (Because attribute system)

    Arrays und dynamic arrays und initialisierung;
    x : int[] = [int: 15, 32, 17, 18];
    x : int[..] = dynamic_array::create();

    Funktionen, Macros und

    Structs, Enums und Unions

    Modularisierung

    Mit was starte ich?
     - Funktionen, Variablen und Statements
     - Scopes
     - If-else

     Program  -> Function*
     Function -> ID ::

     function_name :: (parameters, ...) -> (return_values) {}
     return_value function_name (parameters, ...) {}

     lambdas:
     take_lambda_function :: ( x: (int)->void ) { x(5); } // Definition
     take_lambda_function((num : int) -> void { print(num); });

     TYPE ID(
     int main(int argc, char* argv[]) {}
*/

/*
    Variable Declaration:
        a : int;
    Variable Declaration + Initialization
        a : int = a;
    Variable Declaration with Type Deduction
        a := 5;
    Variable Assignment
        a = (5 + 5);
    If/For/While statement
*/

int parser_find_next_token_type(Parser* parser, Token_Type::ENUM type)
{
    for (int i = parser->index; i < parser->tokens.size; i++) {
        if (parser->tokens[i].type == type) {
            return i;
        }
    }
    return parser->tokens.size;
}

void parser_log_intermediate_error(Parser* parser, const char* msg, int length)
{
    ParserError error;
    error.error_message = msg;
    error.token_start_index = parser->index;
    error.token_end_index = parser->index;
    dynamic_array_push_back(&parser->intermediate_errors, error);
}

void parser_reset_intermediate_errors(Parser* parser) {
    dynamic_array_reset(&parser->intermediate_errors);
}

void parser_print_intermediate_errors(Parser* parser) {
    for (int i = 0; i < parser->intermediate_errors.size; i++) {
        ParserError* error = &parser->intermediate_errors.data[i];
        logg("Intermediate error #%d: %s\n", i, error->error_message);
    }
}

void parser_log_unresolvable_error(Parser* parser, const char* msg, int start_token, int end_token)
{
    ParserError error;
    error.error_message = msg;
    error.token_start_index = start_token;
    error.token_end_index = end_token;
    dynamic_array_push_back(&parser->unresolved_errors, error);
}

bool parser_test_next_token(Parser* parser, Token_Type::ENUM type)
{
    if (parser->index >= parser->tokens.size) {
        return false;
    }
    if (parser->tokens[parser->index].type == type) {
        return true;
    }
    return false;
}

bool parser_test_next_2_tokens(Parser* parser, Token_Type::ENUM type1, Token_Type::ENUM type2)
{
    if (parser->index + 1 >= parser->tokens.size) {
        return false;
    }
    if (parser->tokens[parser->index].type == type1 && parser->tokens[parser->index + 1].type == type2) {
        return true;
    }
    return false;
}

bool parser_test_next_3_tokens(Parser* parser, Token_Type::ENUM type1, Token_Type::ENUM type2, Token_Type::ENUM type3)
{
    if (parser->index + 2 >= parser->tokens.size) {
        return false;
    }
    if (parser->tokens[parser->index].type == type1 &&
        parser->tokens[parser->index + 1].type == type2 &&
        parser->tokens[parser->index + 2].type == type3) {
        return true;
    }
    return false;
}





/*
    AST
*/
void ast_node_expression_destroy(Ast_Node_Expression* expression)
{
    if (expression->type == ExpressionType::OP_ADD) {
        ast_node_expression_destroy(expression->left);
        ast_node_expression_destroy(expression->right);
        if (expression->left != 0) {
            delete expression->left;
        }
        if (expression->right != 0) {
            delete expression->right;
        }
    }
}

void ast_node_statement_destroy(Ast_Node_Statement* statement) {
    if (statement->type == StatementType::VARIABLE_ASSIGNMENT) {
        ast_node_expression_destroy(&statement->expression);
    }
}

void ast_node_statement_block_destroy(Ast_Node_Statement_Block* block) {
    for (int i = 0; i < block->statements.size; i++) {
        Ast_Node_Statement* statement = &block->statements.data[i];
        ast_node_statement_destroy(statement);
    }
    dynamic_array_destroy(&block->statements);
}

void ast_node_function_destroy(Ast_Node_Function* function)
{
    dynamic_array_destroy(&function->parameters);
    ast_node_statement_block_destroy(&function->body);
}

void ast_node_root_destroy(Ast_Node_Root* root)
{
    for (int i = 0; i < root->functions.size; i++)
    {
        Ast_Node_Function* function = &root->functions.data[i];
        ast_node_function_destroy(function);
    }
    dynamic_array_destroy(&root->functions);
}

void ast_node_expression_append_to_string(String* string, Ast_Node_Expression* expression, LexerResult* result)
{
    if (expression->type == ExpressionType::OP_ADD) {
        string_append(string, "(");
        ast_node_expression_append_to_string(string, expression->left, result);
        string_append(string, " + ");
        ast_node_expression_append_to_string(string, expression->right, result);
        string_append(string, ")");
    }
    if (expression->type == ExpressionType::OP_MULTIPLY) {
        string_append(string, "(");
        ast_node_expression_append_to_string(string, expression->left, result);
        string_append(string, " * ");
        ast_node_expression_append_to_string(string, expression->right, result);
        string_append(string, ")");
    }
    if (expression->type == ExpressionType::OP_SUBTRACT) {
        string_append(string, "(");
        ast_node_expression_append_to_string(string, expression->left, result);
        string_append(string, " - ");
        ast_node_expression_append_to_string(string, expression->right, result);
        string_append(string, ")");
    }
    if (expression->type == ExpressionType::OP_DIVIDE) {
        string_append(string, "(");
        ast_node_expression_append_to_string(string, expression->left, result);
        string_append(string, " / ");
        ast_node_expression_append_to_string(string, expression->right, result);
        string_append(string, ")");
    }
    if (expression->type == ExpressionType::OP_MODULO) {
        string_append(string, "(");
        ast_node_expression_append_to_string(string, expression->left, result);
        string_append(string, " % ");
        ast_node_expression_append_to_string(string, expression->right, result);
        string_append(string, ")");
    }
    else if (expression->type == ExpressionType::INTEGER_CONSTANT) {
        string_append_formated(string, "%d", expression->integer_constant_value);
    }
    else if (expression->type == ExpressionType::VARIABLE_READ) {
        string_append_formated(string, "%s", result->identifiers[expression->variable_name_id].characters);
    }
}

void ast_node_statement_append_to_string(String* string, Ast_Node_Statement* statement, LexerResult* result)
{
    if (statement->type == StatementType::VARIABLE_DEFINITION) {
        string_append_formated(string, "%s : %s;",
            result->identifiers[statement->variable_name_id].characters,
            result->identifiers[statement->variable_type_id].characters);
    }
    else if (statement->type == StatementType::VARIABLE_ASSIGNMENT) {
        string_append_formated(string, "%s = ", result->identifiers[statement->variable_name_id].characters);
        ast_node_expression_append_to_string(string, &statement->expression, result);
        string_append(string, ";");
    }
    else if (statement->type == StatementType::RETURN_STATEMENT) {
        string_append_formated(string, "return ");
        ast_node_expression_append_to_string(string, &statement->expression, result);
        string_append_formated(string, ";");
    }
}

void ast_node_function_append_to_string(String* string, Ast_Node_Function* function, LexerResult* result)
{
    string_append_formated(string, "%s :: (", result->identifiers[function->function_name_id].characters);
    for (int i = 0; i < function->parameters.size; i++) {
        Parameter* param = &(function->parameters.data[i]);
        string_append_formated(string, "%s : %s, ",
            result->identifiers[param->name_id].characters,
            result->identifiers[param->type_id].characters);
    }
    string_append_formated(string, ") -> %s {\n", result->identifiers[function->return_type_id].characters);

    for (int i = 0; i < function->body.statements.size; i++) {
        Ast_Node_Statement* statement = &function->body.statements.data[i];
        string_append_formated(string, "\t");
        ast_node_statement_append_to_string(string, statement, result);
        string_append_formated(string, "\n");
    }
    string_append_formated(string, "}");
}

void ast_node_root_append_to_string(String* string, Ast_Node_Root* root, LexerResult* lexer_result)
{
    string_append_formated(string, "\nRoot: (Function count #%d)\n", root->functions.size);
    for (int i = 0; i < root->functions.size; i++)
    {
        Ast_Node_Function* function = &(root->functions.data[i]);
        ast_node_function_append_to_string(string, function, lexer_result);
    }
}



/*
    PARSER
*/
bool parser_parse_expression(Parser* parser, Ast_Node_Expression* expression);

bool parser_parse_expression_single_value(Parser* parser, Ast_Node_Expression* expression)
{
    expression->left = 0;
    expression->right = 0;
    if (parser_test_next_token(parser, Token_Type::IDENTIFIER))
    {
        expression->type = ExpressionType::VARIABLE_READ;
        expression->variable_name_id = parser->tokens[parser->index].attribute.identifier_number;
        parser->index++;
        return true;
    }
    else if (parser_test_next_token(parser, Token_Type::CONSTANT_INT))
    {
        expression->type = ExpressionType::INTEGER_CONSTANT;
        expression->integer_constant_value = parser->tokens[parser->index].attribute.integer_value;
        parser->index++;
        return true;
    }
    else if (parser_test_next_token(parser, Token_Type::OPEN_PARENTHESIS))
    {
        int rewind_point = parser->index;
        parser->index++;
        if (parser_parse_expression(parser, expression)) {
            if (parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS)) {
                parser->index++;
                return true;
            }
            else {
                ast_node_expression_destroy(expression);
                parser->index = rewind_point;
                return false;
            }
        }
        parser->index = rewind_point;
        return false;
    }
    else {
        parser_log_intermediate_error(parser,
            "Error, could not parse single expression, does not start with constant or identifier\n", 1);
        return false;
    }
}

bool parser_parse_binary_operation(Parser* parser, ExpressionType::ENUM* op_type, int* op_priority) 
{
    if (parser_test_next_token(parser, Token_Type::OP_PLUS)) {
        *op_type = ExpressionType::OP_ADD;
        *op_priority = 0;
    }
    else if (parser_test_next_token(parser, Token_Type::OP_MINUS)) {
        *op_type = ExpressionType::OP_SUBTRACT;
        *op_priority = 0;
    }
    else if (parser_test_next_token(parser, Token_Type::OP_SLASH)) {
        *op_type = ExpressionType::OP_DIVIDE;
        *op_priority = 1;
    }
    else if (parser_test_next_token(parser, Token_Type::OP_STAR)) {
        *op_type = ExpressionType::OP_MULTIPLY;
        *op_priority = 1;
    }
    else if (parser_test_next_token(parser, Token_Type::OP_PERCENT)) {
        *op_type = ExpressionType::OP_MODULO;
        *op_priority = 2;
    }
    else {
        return false;
    }
    parser->index++;
    return true;
}

// Input expression is not empty
bool parser_parse_expression_new_priority(Parser* parser, Ast_Node_Expression* expression, int min_priority) 
{
    int start_point = parser->index;
    int rewind_point = parser->index;

    bool first_run = true;
    // Parse expression start operand
    while (true)
    {
        int first_op_priority;
        ExpressionType::ENUM first_op_type;
        if (!parser_parse_binary_operation(parser, &first_op_type, &first_op_priority)) {
            break;
        }
        if (first_op_priority < min_priority) {
            parser->index = rewind_point;
            break;
        }
        if (first_run) {
            min_priority = first_op_priority;
            first_run = false;
        }

        Ast_Node_Expression right_operand;
        if (!parser_parse_expression_single_value(parser, &right_operand)) {
            parser->index = rewind_point;
            break;
        }
        rewind_point = parser->index;

        int second_op_priority;
        ExpressionType::ENUM second_op_type;
        bool second_op_exists = parser_parse_binary_operation(parser, &second_op_type, &second_op_priority);
        if (second_op_exists) {
            parser->index--;
            if (second_op_priority > first_op_priority) {
                parser_parse_expression_new_priority(parser, &right_operand, second_op_priority);
            }
        }
        Ast_Node_Expression* new_left = new Ast_Node_Expression();
        Ast_Node_Expression* new_right = new Ast_Node_Expression();
        *new_right = right_operand;
        *new_left = *expression;
        expression->type = first_op_type;
        expression->left = new_left;
        expression->right = new_right;
        if (!second_op_exists) break;
    }

    return parser->index != start_point;
}

bool parser_parse_expression_priority(Parser* parser, Ast_Node_Expression* expression, int priority_level)
{
    int rewind_point;
    while (true)
    {
        rewind_point = parser->index;
        ExpressionType::ENUM op_type;
        int operation_priority = 0;
        if (parser_test_next_token(parser, Token_Type::OP_PLUS)) {
            op_type = ExpressionType::OP_ADD;
            operation_priority = 0;
        }
        else if (parser_test_next_token(parser, Token_Type::OP_MINUS)) {
            op_type = ExpressionType::OP_SUBTRACT;
            operation_priority = 0;
        }
        else if (parser_test_next_token(parser, Token_Type::OP_SLASH)) {
            op_type = ExpressionType::OP_DIVIDE;
            operation_priority = 1;
        }
        else if (parser_test_next_token(parser, Token_Type::OP_STAR)) {
            op_type = ExpressionType::OP_MULTIPLY;
            operation_priority = 1;
        }
        else if (parser_test_next_token(parser, Token_Type::OP_PERCENT)) {
            op_type = ExpressionType::OP_MODULO;
            operation_priority = 2;
        }
        else {
            return true;
        }

        if (operation_priority < priority_level) {
            return true;
        }
        if (operation_priority == priority_level ||
            (expression->type == ExpressionType::INTEGER_CONSTANT || expression->type == ExpressionType::VARIABLE_READ))
        {
            parser->index++;
            Ast_Node_Expression right;
            if (!parser_parse_expression_single_value(parser, &right)) {
                parser->index = rewind_point;
                return false;
            }
            Ast_Node_Expression* new_left = new Ast_Node_Expression();
            *new_left = *expression;
            Ast_Node_Expression* new_right = new Ast_Node_Expression();
            *new_right = right;
            expression->type = op_type;
            expression->left = new_left;
            expression->right = new_right;
        }
        else if (operation_priority > priority_level)
        {
            if (!parser_parse_expression_priority(parser, expression->right, operation_priority)) {
                parser->index = rewind_point;
                return false;
            }
        }
    }

    parser->index = rewind_point;
    return true;
}

bool parser_parse_expression(Parser* parser, Ast_Node_Expression* expression)
{
    if (!parser_parse_expression_single_value(parser, expression)) {
        return false;
    }
    parser_parse_expression_new_priority(parser, expression, 0);
    return true;
}

bool parser_parse_statement(Parser* parser, Ast_Node_Statement* statement)
{
    int rewind_point = parser->index;

    if (parser_test_next_token(parser, Token_Type::RETURN)) {
        parser->index++;
        Ast_Node_Expression expression;
        if (!parser_parse_expression(parser, &expression)) { // Return may also be fine if the function does not return anything
            parser->index = rewind_point;
            return false;
        }
        statement->type = StatementType::RETURN_STATEMENT;
        statement->expression = expression;
        return true;
    }

    if (parser_test_next_3_tokens(parser, Token_Type::IDENTIFIER, Token_Type::COLON, Token_Type::IDENTIFIER)) // Variable definition 'x : int'
    {
        statement->type = StatementType::VARIABLE_DEFINITION;
        statement->variable_name_id = parser->tokens[parser->index].attribute.identifier_number;
        statement->variable_type_id = parser->tokens[parser->index + 2].attribute.identifier_number;
        parser->index += 3;
        return true;
    }

    if (parser_test_next_2_tokens(parser, Token_Type::IDENTIFIER, Token_Type::OP_ASSIGNMENT))
    {
        // Assignment
        statement->type = StatementType::VARIABLE_ASSIGNMENT;
        statement->variable_name_id = parser->tokens[parser->index].attribute.identifier_number;
        parser->index += 2;

        // Parse expression
        if (parser_parse_expression(parser, &statement->expression)) {
            return true;
        }

        parser->index = rewind_point;
    }

    return false;
}

bool parser_parse_statement_block(Parser* parser, Ast_Node_Statement_Block* block)
{
    block->statements = dynamic_array_create_empty<Ast_Node_Statement>(16);
    Ast_Node_Statement statement;
    int rewind_index;
    while (parser->index < parser->tokens.size)
    {
        rewind_index = parser->index;
        bool do_error_recovery = true;;
        if (parser_parse_statement(parser, &statement)) {
            if (parser_test_next_token(parser, Token_Type::SEMICOLON)) {
                dynamic_array_push_back(&block->statements, statement);
                parser->index++;
                do_error_recovery = false;
            }
            else {
                ast_node_statement_destroy(&statement);
                parser_log_unresolvable_error(parser, "Statement did not end with SEMICOLON(';')!\n", rewind_index, parser->index - 1);
                do_error_recovery = false;
            }
        }

        if (do_error_recovery)
        {
            // Error recovery, skip one token and go after next ;
            int next_semicolon = parser_find_next_token_type(parser, Token_Type::SEMICOLON);
            int next_unexpected_token = parser_find_next_token_type(parser, Token_Type::CLOSED_BRACES);
            // TODO: maybe do something smarter, like going to next IF or something
            if (next_unexpected_token < next_semicolon || next_semicolon >= parser->tokens.size) { // Stop parsing statements
                break;
            }
            else {
                // Just go to next semicolon, and try parsing
                parser_log_unresolvable_error(parser, "Could not parse statement, skipped it\n", parser->index, next_semicolon);
                parser->index = next_semicolon + 1;
            }
        }
    }
    return true;
}

bool parser_parse_function(Parser* parser, Ast_Node_Function* function)
{
    int rewind_point = parser->index;
    function->parameters = dynamic_array_create_empty<Parameter>(8);
    bool exit_failure = false;
    SCOPE_EXIT(
        if (exit_failure) {
            dynamic_array_destroy(&function->parameters);
            parser->index = rewind_point;
        }
    );

    // Parse Function start
    if (!parser_test_next_3_tokens(parser, Token_Type::IDENTIFIER, Token_Type::DOUBLE_COLON, Token_Type::OPEN_PARENTHESIS)) {
        parser_log_intermediate_error(parser, "Could not parse function, it did not start with 'ID :: ('", 3);
        exit_failure = true;
        return false;
    }
    function->function_name_id = parser->tokens[parser->index].attribute.identifier_number;
    parser->index += 3;

    while (!parser_test_next_token(parser, Token_Type::CLOSED_PARENTHESIS))
    {
        // Parameters need to be named, meaning x : int     
        if (!parser_test_next_3_tokens(parser, Token_Type::IDENTIFIER, Token_Type::COLON, Token_Type::IDENTIFIER)) {
            parser_log_intermediate_error(parser, "Could not parse function, parameter was not in the form ID : TYPE", 3);
            exit_failure = true;
            return false;
        }

        Parameter param;
        param.name_id = parser->tokens[parser->index].attribute.identifier_number;
        param.type_id = parser->tokens[parser->index + 2].attribute.identifier_number;
        dynamic_array_push_back(&function->parameters, param);
        parser->index += 3;

        // Check for ) or ,
        if (parser_test_next_token(parser, Token_Type::COMMA)) {
            parser->index++;
        }
    }
    parser->index++; // Skip )

    if (!parser_test_next_3_tokens(parser, Token_Type::ARROW, Token_Type::IDENTIFIER, Token_Type::OPEN_BRACES)) {
        parser_log_intermediate_error(parser, "Could not parse function, did not find return type after Parameters '-> TYPE {'", 3);
        exit_failure = true;
        return false;
    }
    function->return_type_id = parser->tokens[parser->index + 1].attribute.identifier_number;
    parser->index += 3;

    // Parse statements
    if (!parser_parse_statement_block(parser, &function->body)) {
        panic("I dont think this can happen, since we do parser recovery on statement level\n");
        return false;
    }
    SCOPE_EXIT(
        if (exit_failure) {
            ast_node_statement_block_destroy(&function->body);
        }
    );

    // Check that the function ends properly with a closing curly bracket
    if (!parser_test_next_token(parser, Token_Type::CLOSED_BRACES)) {
        // If it is not, eat all tokens until next curly bracket
        int next_closed_curly = parser_find_next_token_type(parser, Token_Type::CLOSED_BRACES);
        parser_log_unresolvable_error(parser, "Function did not end properly with }",
            math_clamp(parser->index, 0, parser->tokens.size - 1),
            math_clamp(next_closed_curly, 0, parser->tokens.size - 1)
        );
        //exit_failure = true;
        parser->index = next_closed_curly + 1;
        //return false;
    }
    else {
        parser->index++; // Eat {
    }

    return true;
}


bool parser_parse_root(Parser* parser, Ast_Node_Root* root)
{
    root->functions = dynamic_array_create_empty<Ast_Node_Function>(32);

    Ast_Node_Function function;
    while (true)
    {
        if (parser_parse_function(parser, &function)) {
            dynamic_array_push_back(&root->functions, function);
        }
        else if (parser->index >= parser->tokens.size) {
            break;
        }
        else {
            // Skip to next token in next line, then try parsing again
            int next_line_token = parser->index;
            while (next_line_token < parser->tokens.size &&
                parser->tokens[next_line_token].line_number == parser->tokens[parser->index].line_number)
            {
                next_line_token++;
            }
            if (next_line_token >= parser->tokens.size) {
                parser_log_unresolvable_error(parser, "Could not parse last function in file!\n", parser->index, parser->tokens.size - 1);
                break;
            }
            else {
                // Skip to next line token, try parsing funciton again
                parser_log_unresolvable_error(parser, "Could not parse function header!\n", parser->index, next_line_token - 1);
                parser->index = next_line_token;
            }
        }
    }

    return true;
}

void parser_semantic_analysis(Parser* parser, LexerResult* lexer_result, Ast_Node_Root* root)
{
}

Parser parser_parse(LexerResult* result)
{
    Parser parser;
    parser.index = 0;
    parser.tokens = dynamic_array_to_array(&result->tokens);
    parser.intermediate_errors = dynamic_array_create_empty<ParserError>(16);
    parser.unresolved_errors = dynamic_array_create_empty<ParserError>(16);

    // Parse root
    if (!parser_parse_root(&parser, &parser.root)) {
        logg("Dont quite know what to do herer lol\n");
    }

    // Do semantic checking
    //parser_semantic_analysis(&parser, result, &root);

    return parser;
}

void parser_destroy(Parser* parser)
{
    dynamic_array_destroy(&parser->intermediate_errors);
    dynamic_array_destroy(&parser->unresolved_errors);
    ast_node_root_destroy(&parser->root);
}




/*
    AST INTERPRETER
*/
struct Ast_Interpreter_Variable
{
    int variable_name;
    int value;
};

struct Ast_Interpreter
{
    Ast_Node_Root* root;
    DynamicArray<Ast_Interpreter_Variable> variables;
    LexerResult* lexer;
};

Ast_Interpreter ast_interpreter_create(Ast_Node_Root* root, LexerResult* lexer)
{
    Ast_Interpreter result;
    result.root = root;
    result.lexer = lexer;
    result.variables = dynamic_array_create_empty<Ast_Interpreter_Variable>(16);
    return result;
}

void ast_interpreter_destroy(Ast_Interpreter* interpreter) {
    dynamic_array_destroy(&interpreter->variables);
}

int ast_interpreter_find_variable_index(Ast_Interpreter* interpreter, int identifier) {
    for (int i = 0; i < interpreter->variables.size; i++) {
        if (interpreter->variables[i].variable_name == identifier) {
            return i;
        }
    }
    return -1;
}

void ast_interpreter_define_variable(Ast_Interpreter* interpreter, int identifier) {
    int index = ast_interpreter_find_variable_index(interpreter, identifier);
    if (index != -1) {
        logg("Variable %s already defined", lexer_result_identifer_to_string(interpreter->lexer, identifier).characters);
        return;
    }
    Ast_Interpreter_Variable var;
    var.value = -1;
    var.variable_name = identifier;
    dynamic_array_push_back(&interpreter->variables, var);
}

int ast_interpreter_evaluate_expression(Ast_Interpreter* interpreter, Ast_Node_Expression* expression)
{
    if (expression->type == ExpressionType::INTEGER_CONSTANT) {
        return expression->integer_constant_value;
    }
    else if (expression->type == ExpressionType::VARIABLE_READ)
    {
        int index = ast_interpreter_find_variable_index(interpreter, expression->variable_name_id);
        if (index == -1) {
            logg("Expression variable %s not defined!\n", lexer_result_identifer_to_string(interpreter->lexer, expression->variable_name_id).characters);
            return -1;
        }
        Ast_Interpreter_Variable* var = &interpreter->variables[index];
        return var->value;
    }
    else if (expression->type == ExpressionType::OP_ADD) {
        return ast_interpreter_evaluate_expression(interpreter, expression->left) +
            ast_interpreter_evaluate_expression(interpreter, expression->right);
    }
    else if (expression->type == ExpressionType::OP_SUBTRACT) {
        return ast_interpreter_evaluate_expression(interpreter, expression->left) -
            ast_interpreter_evaluate_expression(interpreter, expression->right);
    }
    else if (expression->type == ExpressionType::OP_DIVIDE) {
        int divisor = ast_interpreter_evaluate_expression(interpreter, expression->right);
        if (divisor == 0) {
            logg("Error division by 0");
            return -1;
        }
        return ast_interpreter_evaluate_expression(interpreter, expression->left) / divisor;
    }
    else if (expression->type == ExpressionType::OP_MULTIPLY) {
        return ast_interpreter_evaluate_expression(interpreter, expression->left) *
            ast_interpreter_evaluate_expression(interpreter, expression->right);
    }
    else if (expression->type == ExpressionType::OP_MODULO) {
        return ast_interpreter_evaluate_expression(interpreter, expression->left) %
            ast_interpreter_evaluate_expression(interpreter, expression->right);
    }
    logg("Expression type invalid!\n");
    return -1;
}

int ast_interpreter_execute_main(Ast_Node_Root* root, LexerResult* lexer)
{
    Ast_Interpreter interpreter = ast_interpreter_create(root, lexer);
    SCOPE_EXIT(ast_interpreter_destroy(&interpreter));

    // Find main
    Ast_Node_Function* main = 0;
    {
        int* main_identifer = hashtable_find_element(&lexer->identifier_index_lookup_table, string_create_static("main"));
        if (main_identifer == 0) {
            logg("Main not defined\n");
            return -1;
        }
        for (int i = 0; i < root->functions.size; i++) {
            if (root->functions[i].function_name_id == *main_identifer) {
                main = &root->functions[i];
            }
        }
        if (main == 0) {
            logg("Main function not found\n");
            return -1;
        }
    }

    for (int i = 0; i < main->body.statements.size; i++) {
        Ast_Node_Statement* statement = &main->body.statements[i];
        if (statement->type == StatementType::RETURN_STATEMENT) {
            return ast_interpreter_evaluate_expression(&interpreter, &statement->expression);
        }
        else if (statement->type == StatementType::VARIABLE_ASSIGNMENT) {
            int index = ast_interpreter_find_variable_index(&interpreter, statement->variable_name_id);
            if (index == -1) {
                logg("Variable assignment statement variable %s not defined!\n", lexer_result_identifer_to_string(lexer, statement->variable_name_id).characters);
                return -1;
            }
            Ast_Interpreter_Variable* var = &interpreter.variables[index];
            var->value = ast_interpreter_evaluate_expression(&interpreter, &statement->expression);
        }
        else if (statement->type == StatementType::VARIABLE_DEFINITION) {
            ast_interpreter_define_variable(&interpreter, statement->variable_name_id);
        }
    }

    logg("No return statement found!\n");
    return -1;
}
