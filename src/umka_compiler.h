#ifndef UMKA_COMPILER_H_INCLUDED
#define UMKA_COMPILER_H_INCLUDED

#include "umka_common.h"
#include "umka_lexer.h"
#include "umka_types.h"
#include "umka_vm.h"
#include "umka_gen.h"
#include "umka_ident.h"
#include "umka_const.h"


typedef struct
{
    Storage storage;
    Blocks blocks;
    Lexer lex;
    Types types;
    Idents idents;
    Consts consts;
    CodeGen gen;
    VM vm;
    ErrorFunc error;

    // Pointers to built-in types
    Type *voidType,
         *int8Type,  *int16Type,  *int32Type,  *intType,
         *uint8Type, *uint16Type, *uint32Type,
         *boolType,
         *charType,
         *real32Type, *realType,
         *ptrVoidType,
         *stringType;
} Compiler;


void compilerInit(Compiler *comp, char *fileName, int codeCapacity, int storageCapacity, ErrorFunc compileError, ErrorFunc runtimeError);
void compilerFree(Compiler *comp);
void compilerCompile(Compiler *comp);
void compilerRun(Compiler *comp);

#endif // UMKA_COMPILER_H_INCLUDED