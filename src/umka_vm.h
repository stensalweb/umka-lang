#ifndef UMKA_VM_H_INCLUDED
#define UMKA_VM_H_INCLUDED

#include "umka_common.h"
#include "umka_lexer.h"
#include "umka_types.h"


enum
{
    VM_STACK_SIZE       = 65536,  // Slots
    VM_NUM_REGS         = 64,

    // General-purpose registers
    VM_RESULT_REG_0     = 0,
    VM_COMMON_REG_0     = 16,

    // Registers for special use by fprintf() / fscanf()
    VM_IO_FILE_REG      = VM_NUM_REGS - 2,
    VM_IO_FORMAT_REG    = VM_NUM_REGS - 1,
};


typedef enum
{
    OP_NOP,
    OP_PUSH,
    OP_PUSH_LOCAL_PTR,
    OP_PUSH_REG,
    OP_POP,
    OP_POP_REG,
    OP_DUP,
    OP_SWAP,
    OP_DEREF,
    OP_ASSIGN,
    OP_UNARY,
    OP_BINARY,
    OP_GET_ARRAY_PTR,
    OP_GOTO,
    OP_GOTO_IF,
    OP_CALL,
    OP_CALL_BUILTIN,
    OP_RETURN,
    OP_ENTER_FRAME,
    OP_LEAVE_FRAME,
    OP_HALT
} Opcode;


typedef enum
{
    BUILTIN_PRINTF,
    BUILTIN_FPRINTF,
    BUILTIN_SCANF,
    BUILTIN_FSCANF,
    BUILTIN_REAL,           // Integer to real at stack top (right operand)
    BUILTIN_REAL_LHS,       // Integer to real at stack top + 1 (left operand)
    BUILTIN_ROUND,
    BUILTIN_TRUNC,
    BUILTIN_FABS,
    BUILTIN_SQRT,
    BUILTIN_SIN,
    BUILTIN_COS,
    BUILTIN_ATAN,
    BUILTIN_EXP,
    BUILTIN_LOG
} BuiltinFunc;


typedef union
{
    int64_t intVal;
    void *ptrVal;
    double realVal;
    BuiltinFunc builtinVal;
} Slot;


typedef struct
{
    Opcode opcode;
    TokenKind tokKind;  // Unary/binary operation token
    TypeKind typeKind;  // Slot type kind
    Slot operand;
} Instruction;


typedef struct
{
    Slot stack[VM_STACK_SIZE];
    Slot *top, *base;
    Slot reg[VM_NUM_REGS];
    Instruction *instr;
} Fiber;


typedef struct
{
    Instruction *code;
    Fiber fiber;
    ErrorFunc error;
} VM;


void vmInit(VM *vm, Instruction *code, ErrorFunc error);
void vmFree(VM *vm);
void vmRun(VM *vm);

#endif // UMKA_VM_H_INCLUDED