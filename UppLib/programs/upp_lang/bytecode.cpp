#include "bytecode.hpp"

Bytecode_Generator bytecode_generator_create()
{
    Bytecode_Generator result;
    result.instructions = dynamic_array_create_empty<Bytecode_Instruction>(64);
    result.variable_locations = dynamic_array_create_empty<Variable_Location>(64);
    result.break_instructions_to_fill_out = dynamic_array_create_empty<int>(64);
    result.continue_instructions_to_fill_out = dynamic_array_create_empty<int>(64);
    result.function_locations = dynamic_array_create_empty<Function_Location>(64);
    result.function_calls = dynamic_array_create_empty<Function_Call_Location>(64);
    return result;
}

void bytecode_generator_destroy(Bytecode_Generator* generator)
{
    dynamic_array_destroy(&generator->instructions);
    dynamic_array_destroy(&generator->variable_locations);
    dynamic_array_destroy(&generator->break_instructions_to_fill_out);
    dynamic_array_destroy(&generator->continue_instructions_to_fill_out);
    dynamic_array_destroy(&generator->function_calls);
    dynamic_array_destroy(&generator->function_locations);
} 

Bytecode_Instruction instruction_make_0(Instruction_Type::ENUM type) {
    Bytecode_Instruction instr;
    instr.instruction_type = type;
    return instr;
}

Bytecode_Instruction instruction_make_1(Instruction_Type::ENUM type, int src_1) {
    Bytecode_Instruction instr;
    instr.instruction_type = type;
    instr.op1 = src_1;
    return instr;
}

Bytecode_Instruction instruction_make_2(Instruction_Type::ENUM type, int src_1, int src_2) {
    Bytecode_Instruction instr;
    instr.instruction_type = type;
    instr.op1 = src_1;
    instr.op2 = src_2;
    return instr;
}

Bytecode_Instruction instruction_make_3(Instruction_Type::ENUM type, int src_1, int src_2, int src_3) {
    Bytecode_Instruction instr;
    instr.instruction_type = type;
    instr.op1 = src_1;
    instr.op2 = src_2;
    instr.op3 = src_3;
    return instr;
}

int bytecode_generator_generate_register(Bytecode_Generator* generator)
{
    int result = generator->stack_base_offset;
    generator->stack_base_offset++;
    if (generator->stack_base_offset > generator->max_stack_base_offset) {
        generator->max_stack_base_offset = generator->stack_base_offset;
    }
    return result;
}

Variable_Location* bytecode_generator_get_variable_loc(Bytecode_Generator* generator, int name_id)
{
    for (int i = generator->variable_locations.size - 1; i >= 0; i--) {
        Variable_Location* loc = &generator->variable_locations[i];
        if (loc->variable_name == name_id)
            return loc;
    }
    panic("Should not happen after semantic analysis");
    return 0;
}

int bytecode_generator_add_instruction(Bytecode_Generator* generator, Bytecode_Instruction instruction) {
    dynamic_array_push_back(&generator->instructions, instruction);
    return generator->instructions.size - 1;
}

Variable_Type::ENUM bytecode_generator_generate_expression(Bytecode_Generator* generator, AST_Node_Index expression_index, int result_register) 
{
    AST_Node* expression = &generator->analyser->parser->nodes[expression_index];
    Symbol_Table* table = generator->analyser->symbol_tables[generator->analyser->node_to_table_mappings[expression_index]];
    int rewind_reg_count = generator->stack_base_offset;
    SCOPE_EXIT(generator->stack_base_offset = rewind_reg_count;);
    switch (expression->type)
    {
    case AST_Node_Type::EXPRESSION_FUNCTION_CALL: 
    {
        int argument_reg_start = bytecode_generator_generate_register(generator);
        for (int i = 0; i < expression->children.size - 1; i++) {
            bytecode_generator_generate_register(generator);
        }
        for (int i = 0; i < expression->children.size; i++) {
            int argument_index = expression->children[i];
            bytecode_generator_generate_expression(generator, argument_index, argument_reg_start + i);
        }
        Function_Call_Location call_loc;
        call_loc.call_instruction_location = bytecode_generator_add_instruction(generator, 
            instruction_make_2(Instruction_Type::CALL, 0, generator->stack_base_offset));
        call_loc.function_name = expression->name_id;
        dynamic_array_push_back(&generator->function_calls, call_loc);

        //TODO: Find return type, this needs to be done better at some point (Symbol_Table containing the return type, and argument types)
        bool in_current_scope;
        AST_Node* function = &generator->analyser->parser->nodes[
            symbol_table_find_symbol_of_type(table, expression->name_id, Symbol_Type::FUNCTION, &in_current_scope)->function_index];
        return symbol_table_find_symbol_of_type(table, function->type_id, Symbol_Type::TYPE, &in_current_scope)->variable_type;
    }
    case AST_Node_Type::EXPRESSION_LITERAL: 
    {
        Variable_Type::ENUM return_type;
        Token& token = generator->analyser->parser->lexer->tokens[generator->analyser->parser->token_mapping[expression_index].start_index];
        switch (token.type)
        {
        case Token_Type::FLOAT_LITERAL:
            return_type = Variable_Type::FLOAT;
            bytecode_generator_add_instruction(generator, instruction_make_2(
                Instruction_Type::LOAD_INTERMEDIATE_4BYTE, result_register, *((int*)((void*)(&token.attribute.float_value)))
            ));
            break;
        case Token_Type::INTEGER_LITERAL:
            return_type = Variable_Type::INTEGER;
            bytecode_generator_add_instruction(generator, instruction_make_2(
                Instruction_Type::LOAD_INTERMEDIATE_4BYTE, result_register, token.attribute.integer_value)
            );
            break;
        case Token_Type::BOOLEAN_LITERAL:
            return_type = Variable_Type::BOOLEAN;
            bytecode_generator_add_instruction(generator, instruction_make_2(
                Instruction_Type::LOAD_INTERMEDIATE_4BYTE, result_register, token.attribute.bool_value ? 1 : 0)
            );
            break;
        }
        return return_type;
    }
    case AST_Node_Type::EXPRESSION_VARIABLE_READ: {
        Variable_Location* loc = bytecode_generator_get_variable_loc(generator, expression->name_id);
        bytecode_generator_add_instruction(generator, instruction_make_2(Instruction_Type::MOVE, result_register, loc->stack_base_offset));
        return loc->variable_type;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_ADDITION: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::FLOAT_ADDITION, result_register, result_register, right_register));
            return Variable_Type::FLOAT;
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::INT_ADDITION, result_register, result_register, right_register));
            return Variable_Type::INTEGER;
        }
        break;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_SUBTRACTION: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::FLOAT_SUBTRACT, result_register, result_register, right_register));
            return Variable_Type::FLOAT;
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::INT_SUBTRACT, result_register, result_register, right_register));
            return Variable_Type::INTEGER;
        }
        break;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_DIVISION: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::FLOAT_DIVISION, result_register, result_register, right_register));
            return Variable_Type::FLOAT;
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::INT_DIVISION, result_register, result_register, right_register));
            return Variable_Type::INTEGER;
        }
        break;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_MULTIPLICATION: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::FLOAT_MULTIPLY, result_register, result_register, right_register));
            return Variable_Type::FLOAT;
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::INT_MULTIPLY, result_register, result_register, right_register));
            return Variable_Type::INTEGER;
        }
        break; 
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_MODULO: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        instruction_make_3(Instruction_Type::INT_MODULO, result_register, result_register, right_register);
        return Variable_Type::INTEGER;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_AND: 
    {    
        int right_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        instruction_make_3(Instruction_Type::BOOLEAN_AND, result_register, result_register, right_register);
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_OR: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        instruction_make_3(Instruction_Type::BOOLEAN_OR, result_register, result_register, right_register);
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_EQUAL: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        instruction_make_3(Instruction_Type::COMPARE_REGISTERS_4BYTE_EQUAL, result_register, result_register, right_register);
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_NOT_EQUAL: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        instruction_make_3(Instruction_Type::COMPARE_REGISTERS_4BYTE_NOT_EQUAL, result_register, result_register, right_register);
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_LESS: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_FLOAT_LESS_THAN, result_register, result_register, right_register));
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_INT_LESS_THAN, result_register, result_register, right_register));
        }
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_LESS_OR_EQUAL: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_FLOAT_LESS_EQUAL, result_register, result_register, right_register));
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_INT_LESS_EQUAL, result_register, result_register, right_register));
        }
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_GREATER: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_FLOAT_GREATER_THAN, result_register, result_register, right_register));
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_INT_GREATER_THAN, result_register, result_register, right_register));
        }
        return Variable_Type::BOOLEAN;
    }
    case AST_Node_Type::EXPRESSION_BINARY_OPERATION_GREATER_OR_EQUAL: 
    {
        int right_register = bytecode_generator_generate_register(generator);
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_generate_expression(generator, expression->children[1], right_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_FLOAT_GREATER_EQUAL, result_register, result_register, right_register));
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_3(Instruction_Type::COMPARE_INT_GREATER_EQUAL, result_register, result_register, right_register));
        }
        return Variable_Type::BOOLEAN;
        break;
    }
    case AST_Node_Type::EXPRESSION_UNARY_OPERATION_NEGATE: 
    {
        Variable_Type::ENUM operand_types = bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        switch (operand_types) {
        case Variable_Type::FLOAT: bytecode_generator_add_instruction(generator,
            instruction_make_2(Instruction_Type::FLOAT_NEGATE, result_register, result_register));
            return Variable_Type::FLOAT;
        case Variable_Type::INTEGER: bytecode_generator_add_instruction(generator,
            instruction_make_2(Instruction_Type::INT_NEGATE, result_register, result_register));
            return Variable_Type::INTEGER;
        }
        break;
    }
    case AST_Node_Type::EXPRESSION_UNARY_OPERATION_NOT: 
    {
        bytecode_generator_generate_expression(generator, expression->children[0], result_register);
        bytecode_generator_add_instruction(generator, instruction_make_2(Instruction_Type::BOOLEAN_NOT, result_register, result_register));
        return Variable_Type::BOOLEAN;
    }
    }

    panic("Shit this is not something that should happen!\n");
    return Variable_Type::ERROR_TYPE;
}

void bytecode_generator_generate_statement_block(Bytecode_Generator* generator, AST_Node_Index block_index, bool generate_variables);
void bytecode_generator_generate_statement(Bytecode_Generator* generator, AST_Node_Index statement_index)
{
    AST_Node* statement = &generator->analyser->parser->nodes[statement_index];
    int register_rewind = generator->stack_base_offset;
    switch (statement->type)
    {
    case AST_Node_Type::STATEMENT_BLOCK: {
        bytecode_generator_generate_statement_block(generator, statement->children[0], true);
        break;
    }
    case AST_Node_Type::STATEMENT_BREAK:
    {
        dynamic_array_push_back(&generator->break_instructions_to_fill_out, generator->instructions.size);
        dynamic_array_push_back(&generator->instructions, instruction_make_0(Instruction_Type::JUMP));
        break;
    }
    case AST_Node_Type::STATEMENT_CONTINUE:
    {
        dynamic_array_push_back(&generator->continue_instructions_to_fill_out, generator->instructions.size);
        dynamic_array_push_back(&generator->instructions, instruction_make_0(Instruction_Type::JUMP));
        break;
    }
    case AST_Node_Type::STATEMENT_RETURN: {
        int return_value_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, statement->children[0], return_value_register);
        if (generator->in_main_function) {
            dynamic_array_push_back(&generator->instructions, instruction_make_1(Instruction_Type::EXIT, return_value_register));
        }
        else {
            dynamic_array_push_back(&generator->instructions, instruction_make_1(Instruction_Type::RETURN, return_value_register));
        }
        break;
    }
    case AST_Node_Type::STATEMENT_IF:
    {
        int condition_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, statement->children[0], condition_register);
        int fill_out_later = bytecode_generator_add_instruction(generator, instruction_make_2(Instruction_Type::JUMP_ON_FALSE, 0, condition_register));
        bytecode_generator_generate_statement_block(generator, statement->children[1], true);
        generator->instructions[fill_out_later].op1 = generator->instructions.size;
        break;
    }
    case AST_Node_Type::STATEMENT_IF_ELSE:
    {
        int condition_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, statement->children[0], condition_register);
        int jump_else = bytecode_generator_add_instruction(generator, instruction_make_2(Instruction_Type::JUMP_ON_FALSE, 0, condition_register));
        bytecode_generator_generate_statement_block(generator, statement->children[1], true);
        int jump_end = bytecode_generator_add_instruction(generator, instruction_make_0(Instruction_Type::JUMP));
        generator->instructions[jump_else].op1 = generator->instructions.size;
        bytecode_generator_generate_statement_block(generator, statement->children[2], true);
        generator->instructions[jump_end].op1 = generator->instructions.size;
        break;
    }
    case AST_Node_Type::STATEMENT_WHILE:
    {
        int start_index = generator->instructions.size;
        int condition_register = bytecode_generator_generate_register(generator);
        bytecode_generator_generate_expression(generator, statement->children[0], condition_register);
        int jump_end = bytecode_generator_add_instruction(generator, instruction_make_2(Instruction_Type::JUMP_ON_FALSE, 0, condition_register));
        bytecode_generator_generate_statement_block(generator, statement->children[1], true);
        bytecode_generator_add_instruction(generator, instruction_make_1(Instruction_Type::JUMP, start_index));
        int end_index = generator->instructions.size;
        generator->instructions[jump_end].op1 = end_index;

        for (int i = 0; i < generator->break_instructions_to_fill_out.size; i++) {
            int brk_index = generator->break_instructions_to_fill_out[i];
            generator->instructions[brk_index].op1 = end_index;
        }
        dynamic_array_reset(&generator->break_instructions_to_fill_out);
        for (int i = 0; i < generator->continue_instructions_to_fill_out.size; i++) {
            int cnd_index = generator->continue_instructions_to_fill_out[i];
            generator->instructions[cnd_index].op1 = start_index;
        }
        dynamic_array_reset(&generator->continue_instructions_to_fill_out);
        break;
    }
    case AST_Node_Type::STATEMENT_EXPRESSION: {
        bytecode_generator_generate_expression(generator, statement->children[0], bytecode_generator_generate_register(generator));
        break;
    }
    case AST_Node_Type::STATEMENT_VARIABLE_ASSIGNMENT:
    case AST_Node_Type::STATEMENT_VARIABLE_DEFINE_ASSIGN:
    case AST_Node_Type::STATEMENT_VARIABLE_DEFINE_INFER: {
        Variable_Location* loc = bytecode_generator_get_variable_loc(generator, statement->name_id);
        bytecode_generator_generate_expression(generator, statement->children[0], loc->stack_base_offset);
        break;
    }
    case AST_Node_Type::STATEMENT_VARIABLE_DEFINITION: {
        break; // This is all :)
    }
    }

    generator->stack_base_offset = register_rewind;
    return;
}

void bytecode_generator_generate_statement_block(Bytecode_Generator* generator, AST_Node_Index block_index, bool generate_variables)
{
    AST_Node* block = &generator->analyser->parser->nodes[block_index];
    int size_rollback = generator->variable_locations.size;
    int register_rewind = generator->stack_base_offset;
    if (generate_variables)
    {
        Symbol_Table* table = generator->analyser->symbol_tables[generator->analyser->node_to_table_mappings[block_index]];
        for (int i = 0; i < table->symbols.size; i++)
        {
            Symbol s = table->symbols[i];
            if (s.symbol_type != Symbol_Type::VARIABLE) {
                continue;
            }
            Variable_Location loc;
            loc.stack_base_offset = bytecode_generator_generate_register(generator);
            loc.variable_name = s.name;
            loc.variable_type = s.variable_type;
            dynamic_array_push_back(&generator->variable_locations, loc);
        }
    }
    for (int i = 0; i < block->children.size; i++) {
        bytecode_generator_generate_statement(generator, block->children[i]);
    }
    dynamic_array_rollback_to_size(&generator->variable_locations, size_rollback);
    generator->stack_base_offset = register_rewind;
}

void bytecode_generator_generate_function_code(Bytecode_Generator* generator, AST_Node_Index function_index)
{
    AST_Node* function = &generator->analyser->parser->nodes[function_index];
    AST_Node* parameter_block = &generator->analyser->parser->nodes[function->children[0]];
    Symbol_Table* table = generator->analyser->symbol_tables[generator->analyser->node_to_table_mappings[function_index]];

    generator->stack_base_offset = 1; // Since stack points to the retur address
    generator->max_stack_base_offset = 0;
    generator->in_main_function = false;
    if (function->name_id == generator->main_name_id) {
        generator->in_main_function = true;
        generator->entry_point_index = generator->instructions.size;
    }

    // Dont do this when we have globals, since this would reset too much
    dynamic_array_reset(&generator->variable_locations);
    for (int i = 0; i < table->symbols.size; i++)
    {
        // HACK: All parameters are the first symbols in my current symbol_table design
        Symbol s = table->symbols[i];
        if (s.symbol_type != Symbol_Type::VARIABLE) {
            panic("Hack does not work anymore, now do something smarter here!");
        }
        Variable_Location loc;
        if (i < parameter_block->children.size) {
            loc.stack_base_offset = parameter_block->children.size - i - 2; // - 2 since return address is also on the stack
        }
        else {
            loc.stack_base_offset = bytecode_generator_generate_register(generator);
        }
        loc.variable_name = s.name;
        loc.variable_type = s.variable_type;
        dynamic_array_push_back(&generator->variable_locations, loc);
    }

    int fill_out_max_register = bytecode_generator_add_instruction(generator, instruction_make_0(Instruction_Type::ENTER));
    bytecode_generator_generate_statement_block(generator, function->children[1], false);
    generator->instructions[fill_out_max_register].op1 = generator->max_stack_base_offset;

    Function_Location location;
    location.function_entry_instruction = fill_out_max_register;
    location.name_id = function->name_id;
    dynamic_array_push_back(&generator->function_locations, location);
}

void bytecode_generator_generate(Bytecode_Generator* generator, Semantic_Analyser* analyser)
{
    generator->analyser = analyser;
    generator->stack_base_offset = 0;
    dynamic_array_reset(&generator->instructions);
    dynamic_array_reset(&generator->variable_locations);
    dynamic_array_reset(&generator->break_instructions_to_fill_out);
    dynamic_array_reset(&generator->continue_instructions_to_fill_out);
    dynamic_array_reset(&generator->function_calls);
    dynamic_array_reset(&generator->function_locations);
    generator->main_name_id = lexer_add_or_find_identifier_by_string(analyser->parser->lexer, string_create_static("main"));
    generator->entry_point_index = -1;

    // Generate code for all functions
    for (int i = 0; i < analyser->parser->nodes[0].children.size; i++) {
        AST_Node_Index function_index = analyser->parser->nodes[0].children[i];
        bytecode_generator_generate_function_code(generator, function_index);
    }

    // Fill out all function calls
    for (int i = 0; i < generator->function_calls.size; i++) {
        Function_Call_Location& call_loc = generator->function_calls[i];
        for (int j = 0; j < generator->function_locations.size; j++) {
            Function_Location& function_loc = generator->function_locations[j];
            if (function_loc.name_id == call_loc.function_name) {
                generator->instructions[call_loc.call_instruction_location].op1 = function_loc.function_entry_instruction;
                break;
            }
        }
    }

    if (generator->entry_point_index == -1) {
        panic("main not found, fag!\n");
    }
}

void bytecode_generator_append_bytecode_to_string(Bytecode_Generator* generator, String* string)
{
    string_append_formated(string, "Functions:\n");
    for (int i = 0; i < generator->function_locations.size; i++) {
        Function_Location& loc = generator->function_locations[i];
        string_append_formated(string, "\t%s: %d\n", 
            lexer_identifer_to_string(generator->analyser->parser->lexer, loc.name_id).characters, loc.function_entry_instruction);
    }
    string_append_formated(string, "Code: \n");
    for (int i = 0; i < generator->instructions.size; i++) 
    {
        Bytecode_Instruction& instruction = generator->instructions[i];
        string_append_formated(string, "%4d: ", i);
        switch (instruction.instruction_type)
        {
        case Instruction_Type::LOAD_INTERMEDIATE_4BYTE:
            string_append_formated(string, "LOAD_INTERMEDIATE_4BYTE           dest=%d, val=%08x\n", instruction.op1, instruction.op2);
            break;
        case Instruction_Type::MOVE:
            string_append_formated(string, "MOVE                              dest=%d, src=%d\n", instruction.op1, instruction.op2);
            break;
        case Instruction_Type::JUMP:
            string_append_formated(string, "JUMP                              dest=%d\n", instruction.op1);
            break;
        case Instruction_Type::JUMP_ON_TRUE:
            string_append_formated(string, "JUMP_ON_TRUE                      dest=%d, cond=%d\n", instruction.op1, instruction.op2);
            break;
        case Instruction_Type::JUMP_ON_FALSE:
            string_append_formated(string, "JUMP_ON_FALSE                     dest=%d, cond=%d\n", instruction.op1, instruction.op2);
            break;
        case Instruction_Type::CALL:
            string_append_formated(string, "CALL                              dest=%d, stack_offset=%d\n", instruction.op1, instruction.op2);
            break;
        case Instruction_Type::ENTER:
            string_append_formated(string, "ENTER                             stack_size=%d\n", instruction.op1);
            break;
        case Instruction_Type::RETURN:
            string_append_formated(string, "RETURN                            return_reg=%d\n", instruction.op1);
            break;
        case Instruction_Type::LOAD_RETURN_VALUE: 
            string_append_formated(string, "LOAD_RETURN_VALUE                 dst=%d\n", instruction.op1);
            break;
        case Instruction_Type::EXIT: 
            string_append_formated(string, "EXIT                              src=%d\n", instruction.op1);
            break;
        case Instruction_Type::INT_ADDITION:
            string_append_formated(string, "INT_ADDITION                      dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::INT_SUBTRACT:
            string_append_formated(string, "INT_SUBTRACT                      dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::INT_MULTIPLY:
            string_append_formated(string, "INT_MULTIPLY                      dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::INT_DIVISION:
            string_append_formated(string, "INT_DIVISION                      dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::INT_MODULO:
            string_append_formated(string, "INT_MODULO                        dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::INT_NEGATE:
            string_append_formated(string, "INT_NEGATE                        dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::FLOAT_ADDITION:
            string_append_formated(string, "FLOAT_ADDITION                    dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::FLOAT_SUBTRACT:
            string_append_formated(string, "FLOAT_SUBTRACT                    dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::FLOAT_MULTIPLY:
            string_append_formated(string, "FLOAT_MULTIPLY                    dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::FLOAT_DIVISION:
            string_append_formated(string, "FLOAT_DIVISION                    dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::FLOAT_NEGATE:
            string_append_formated(string, "FLOAT_NEGATE                      dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::BOOLEAN_AND:
            string_append_formated(string, "BOOLEAN_AND                       dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::BOOLEAN_OR:
            string_append_formated(string, "BOOLEAN_OR                        dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::BOOLEAN_NOT:
            string_append_formated(string, "BOOLEAN_NOT                       dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_INT_GREATER_THAN:
            string_append_formated(string, "COMPARE_INT_GREATER_THAN          dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_INT_GREATER_EQUAL:
            string_append_formated(string, "COMPARE_INT_GREATER_EQUAL         dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_INT_LESS_THAN:
            string_append_formated(string, "COMPARE_INT_LESS_THAN             dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_INT_LESS_EQUAL:
            string_append_formated(string, "COMPARE_INT_LESS_EQUAL            dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_FLOAT_GREATER_THAN:
            string_append_formated(string, "COMPARE_FLOAT_GREATER_THAN        dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_FLOAT_GREATER_EQUAL:
            string_append_formated(string, "COMPARE_FLOAT_GREATER_EQUAL       dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_FLOAT_LESS_THAN:
            string_append_formated(string, "COMPARE_FLOAT_LESS_THAN           dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_FLOAT_LESS_EQUAL:
            string_append_formated(string, "COMPARE_FLOAT_LESS_EQUAL          dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_REGISTERS_4BYTE_EQUAL:
            string_append_formated(string, "COMPARE_REGISTERS_4BYTE_EQUAL     dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        case Instruction_Type::COMPARE_REGISTERS_4BYTE_NOT_EQUAL:
            string_append_formated(string, "COMPARE_REGISTERS_4BYTE_NOT_EQUAL dst=%d, src1=%d, src2=%d\n", instruction.op1, instruction.op2, instruction.op3);
            break;
        }
    }
}

