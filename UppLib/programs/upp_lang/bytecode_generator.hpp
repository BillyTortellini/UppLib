#pragma once

#include "../../datastructures/dynamic_array.hpp"
#include "../../datastructures/string.hpp"
#include "../../datastructures/hashtable.hpp"
#include "semantic_analyser.hpp"

struct Compiler;

/*
    Runtime system has:
        - Stack (Return addresses, register data, function arguments)
        - Stack_Pointer
        - Instruction_Pointer
        - Return_Register (s, when we have multiple return values)

    A Stack-Frame looks like this:
    [Param0] [Param1] [ParamX...] [Return_Address] [Old_Stack_Pointer] [Reg0] [Reg1] [Reg2] [Regs...]
*/

enum class Instruction_Type
{
    MOVE_STACK_DATA,  // op1 = dest_reg, op2 = src_reg, op3 = size
    WRITE_MEMORY, // op1 = address_reg, op2 = value_reg, op3 = size
    READ_MEMORY, // op1 = dest_reg, op2 = address_reg, op3 = size
    MEMORY_COPY, // op1 = dest_address_reg, op2 = src_address_reg, op3 = size
    READ_GLOBAL, // op1 = dest_address_reg, op2 = global offset, op3 = size
    WRITE_GLOBAL, // op1 = dest_global offset, op2 = src_reg, op3 = size
    READ_CONSTANT, // op1 = dest_reg, op2 = constant offset, op3 = constant size
    U64_ADD_CONSTANT_I32, // op1 = dest_reg, op2 = src, op3 = constant offset
    U64_MULTIPLY_ADD_I32, // op1 = dest_reg, op2 = base_reg, op3 = index_reg, op4 = size

    JUMP, // op1 = instruction_index
    JUMP_ON_TRUE, // op1 = instruction_index, op2 = cnd_reg
    JUMP_ON_FALSE, // op1 = instruction_index, op2 = cnd_reg
    CALL_FUNCTION, // Pushes return address, op1 = instruction_index, op2 = stack_offset for new frame
    CALL_FUNCTION_POINTER, // op1 = src_reg, op2 = stack_offset for new frame
    CALL_HARDCODED_FUNCTION, // op1 = hardcoded_function_type, op2 = stack_offset for new frame
    RETURN, // Pops return address, op1 = return_value reg, op2 = return_size (Capped at 16 bytes)
    EXIT, // op1 = exit_code

    LOAD_RETURN_VALUE, // op1 = dst_reg, op2 = size
    LOAD_REGISTER_ADDRESS, // op1 = dest_reg, op2 = register_to_load
    LOAD_GLOBAL_ADDRESS, // op1 = dest_reg, op2 = global offset
    LOAD_FUNCTION_LOCATION, // op1 = dest_reg, op2 = funciton_index

    CAST_INTEGER_DIFFERENT_SIZE, // op1 = dst_reg, op2 = src_reg, op3 = dst_prim_type, op4 = src_prim_type
    CAST_FLOAT_DIFFERENT_SIZE, // op1 = dst_reg, op2 = src_reg, op3 = dst_prim_type, op4 = src_prim_type
    CAST_FLOAT_INTEGER, // op1 = dst_reg, op2 = src_reg, op3 = dst_prim_type, op4 = src_prim_type
    CAST_INTEGER_FLOAT, // op1 = dst_reg, op2 = src_reg, op3 = dst_prim_type, op4 = src_prim_type

    // Binary operations work the following: op1 = dest_byte_offset, op2 = left_byte_offset, op3 = right_byte_offset, op4 = primitve_type
    BINARY_OP_ADDITION,
    BINARY_OP_SUBTRACTION,
    BINARY_OP_MULTIPLICATION,
    BINARY_OP_DIVISION,
    BINARY_OP_EQUAL,
    BINARY_OP_NOT_EQUAL,
    BINARY_OP_GREATER_THAN,
    BINARY_OP_GREATER_EQUAL,
    BINARY_OP_LESS_THAN,
    BINARY_OP_LESS_EQUAL,
    BINARY_OP_MODULO,
    BINARY_OP_AND,
    BINARY_OP_OR,

    // Unary operations work the following: op1 = dest_byte_offset, op2 = operand_offset, op3 = primitive_type
    UNARY_OP_NEGATE,
    UNARY_OP_NOT
};

struct Bytecode_Instruction
{
    Instruction_Type instruction_type;
    int op1;
    int op2;
    int op3;
    int op4;
};

struct Function_Reference
{
    IR_Function* function;
    int instruction_index;
};

struct Bytecode_Generator
{
    // Result data
    Dynamic_Array<Bytecode_Instruction> instructions;
    Hashtable<IR_Function*, int> function_locations;

    // Program Information
    Dynamic_Array<Dynamic_Array<int>> stack_offsets;
    Hashtable<IR_Code_Block*, int> code_block_register_stack_offset_index;
    Hashtable<IR_Function*, int> function_parameter_stack_offset_index;
    Dynamic_Array<int> global_data_offsets;

    int global_data_size;
    int entry_point_index;
    int maximum_function_stack_depth;

    // Data required for generation
    IR_Program* ir_program;
    Compiler* compiler;
    Dynamic_Array<Function_Reference> fill_out_calls;
    Dynamic_Array<Function_Reference> fill_out_function_ptr_loads;
    Dynamic_Array<int> fill_out_breaks;
    Dynamic_Array<int> fill_out_continues;
    int current_stack_offset;
};

Bytecode_Generator bytecode_generator_create();
void bytecode_generator_destroy(Bytecode_Generator* generator);
void bytecode_generator_generate(Bytecode_Generator* generator, Compiler* compiler);
void bytecode_instruction_append_to_string(String* string, Bytecode_Instruction instruction);
void bytecode_generator_append_bytecode_to_string(Bytecode_Generator* generator, String* string);

int align_offset_next_multiple(int offset, int alignment);