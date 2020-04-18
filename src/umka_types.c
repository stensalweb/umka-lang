#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "umka_types.h"


static char *spelling [] =
{
    "none",
    "void",
    "int8",
    "int16",
    "int32",
    "int",
    "uint8",
    "uint16",
    "uint32",
    "bool",
    "char",
    "real32",
    "real",
    "^",
    "[]",
    "struct",
    "fn"
};


void typeInit(Types *types, ErrorFunc error)
{
    types->first = types->last = NULL;
    types->error = error;
}


void typeFreeFieldsAndParams(Type *type)
{
    // Fields
    if (type->kind == TYPE_STRUCT)
        for (int i = 0; i < type->numItems; i++)
            free(type->field[i]);

    // Parameters
    if (type->kind == TYPE_FN)
        for (int i = 0; i < type->sig.numParams; i++)
            free(type->sig.param[i]);
}


void typeFree(Types *types, int startBlock)
{
    Type *type = types->first;

    // If block is specified, fast forward to the first type in this block (assuming this is the last block in the list)
    if (startBlock >= 0)
    {
        while (type && type->next && type->next->block != startBlock)
            type = type->next;

        Type *next = type->next;
        type->next = NULL;
        type = next;
    }


    while (type)
    {
        Type *next = type->next;
        typeFreeFieldsAndParams(type);
        free(type);
        type = next;
    }
}


Type *typeAdd(Types *types, Blocks *blocks, TypeKind kind)
{
    Type *type = malloc(sizeof(Type));

    type->kind     = kind;
    type->block    = blocks->item[blocks->top].block;
    type->base     = NULL;
    type->numItems = 0;
    type->next     = NULL;

    // Add to list
    if (!types->first)
        types->first = types->last = type;
    else
    {
        types->last->next = type;
        types->last = type;
    }
    return types->last;
}


Type *typeAddPtrTo(Types *types, Blocks *blocks, Type *type)
{
    typeAdd(types, blocks, TYPE_PTR);
    types->last->base = type;
    return types->last;
}


int typeSize(Types *types, Type *type)
{
    switch (type->kind)
    {
        case TYPE_VOID:     return 0;
        case TYPE_INT8:     return sizeof(int8_t);
        case TYPE_INT16:    return sizeof(int16_t);
        case TYPE_INT32:    return sizeof(int32_t);
        case TYPE_INT:      return sizeof(int64_t);
        case TYPE_UINT8:    return sizeof(uint8_t);
        case TYPE_UINT16:   return sizeof(uint16_t);
        case TYPE_UINT32:   return sizeof(uint32_t);
        case TYPE_BOOL:     return sizeof(bool);
        case TYPE_CHAR:     return sizeof(char);
        case TYPE_REAL32:   return sizeof(float);
        case TYPE_REAL:     return sizeof(double);
        case TYPE_PTR:      return sizeof(void *);
        case TYPE_ARRAY:    return typeSize(types, type->base) * type->numItems;
        case TYPE_STRUCT:
        {
            int size = 0;
            for (int i = 0; i < type->numItems; i++)
                size += typeSize(types, type->field[i]->type);
            return size;
        }
        case TYPE_FN:       return sizeof(void *);

        default: types->error("Illegal type: %s", typeSpelling(type)); return 0;
    }
}


bool typeInteger(Type *type)
{
    return type->kind == TYPE_INT8  || type->kind == TYPE_INT16  || type->kind == TYPE_INT32  || type->kind == TYPE_INT ||
           type->kind == TYPE_UINT8 || type->kind == TYPE_UINT16 || type->kind == TYPE_UINT32;
}


bool typeOrdinal(Type *type)
{
    return typeInteger(type) || type->kind == TYPE_CHAR;
}


bool typeReal(Type *type)
{
    return type->kind == TYPE_REAL32 || type->kind == TYPE_REAL;
}


bool typeString(Type *type)
{
    return type->kind == TYPE_PTR && type->base->kind == TYPE_ARRAY && type->base->base->kind == TYPE_CHAR;
}


bool typeDefaultRef(Type *type)
{
    return type->kind == TYPE_PTR || type->kind == TYPE_ARRAY || type->kind == TYPE_STRUCT;
}


bool typeAssignable(Type *type)
{
    return (type->kind == TYPE_PTR && type->base->kind != TYPE_NONE && type->base->kind != TYPE_VOID) ||
            typeDefaultRef(type);
}


bool typeAssertAssignable(Types *types, Type *type)
{
    bool res = typeAssignable(type);
    if (!res)
        types->error("Variable reference expected");
    return res;
}


bool typeEquivalent(Type *left, Type *right)
{
    if (left == right)
        return true;

    if (left->kind == right->kind)
    {
        // References
        if (left->kind == TYPE_PTR)
            return typeEquivalent(left->base, right->base);

        // Arrays
        else if (left->kind == TYPE_ARRAY)
        {
            // Number of elements
            if (left->numItems != right->numItems)
                return false;

            return typeEquivalent(left->base, right->base);
        }

        // Structures
        else if (left->kind == TYPE_STRUCT)
        {
            // Number of fields
            if (left->numItems != right->numItems)
                return false;

            // Fields
            for (int i = 0; i < left->numItems; i++)
            {
                // Name
                if (left->field[i]->hash != right->field[i]->hash || strcmp(left->field[i]->name, right->field[i]->name) != 0)
                    return false;

                // Type
                if (!typeEquivalent(left->field[i]->type, right->field[i]->type))
                    return false;
            }
            return true;
        }

        // Functions
        else if (left->kind == TYPE_FN)
        {
            // Number of parameters
            if (left->sig.numParams != right->sig.numParams)
                return false;

            // Parameters
            for (int i = 0; i < left->sig.numParams; i++)
            {
                // Name
                if (left->sig.param[i]->hash != right->sig.param[i]->hash ||
                    strcmp(left->sig.param[i]->name, right->sig.param[i]->name) != 0)
                    return false;

                // Type
                if (!typeEquivalent(left->sig.param[i]->type, right->sig.param[i]->type))
                    return false;

                // Default value
                if (left->sig.param[i]->defaultVal.intVal != right->sig.param[i]->defaultVal.intVal)
                    return false;
            }

            // Number of results
            if (left->sig.numResults != right->sig.numResults)
                return false;

            // Result types
            for (int i = 0; i < left->sig.numResults; i++)
                if (!typeEquivalent(left->sig.resultType[i], right->sig.resultType[i]))
                    return false;

            return true;
        }

        // Primitive types
        else
            return true;
    }
    return false;
}


bool typeAssertEquivalent(Types *types, Type *left, Type *right)
{
    bool res = typeEquivalent(left, right);
    if (!res)
        types->error("Incompatible types %s and %s", typeSpelling(left), typeSpelling(right));
    return res;
}


bool typeCompatible(Type *left, Type *right)
{
    if (typeEquivalent(left, right))
        return true;

    if (typeInteger(left) && typeInteger(right))
        return true;

    if (typeString(left) && typeString(right))
        return true;

    return false;
}


bool typeAssertCompatible(Types *types, Type *left, Type *right)
{
    bool res = typeCompatible(left, right);
    if (!res)
        types->error("Incompatible types %s and %s", typeSpelling(left), typeSpelling(right));
    return res;
}


bool typeValidOperator(Type *type, TokenKind op)
{
    switch (op)
    {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_MUL:
        case TOK_DIV:       return typeInteger(type) || typeReal(type);
        case TOK_MOD:
        case TOK_AND:
        case TOK_OR:
        case TOK_XOR:
        case TOK_SHL:
        case TOK_SHR:       return typeInteger(type);
        case TOK_PLUSEQ:
        case TOK_MINUSEQ:
        case TOK_MULEQ:
        case TOK_DIVEQ:     return typeInteger(type) || typeReal(type);
        case TOK_MODEQ:
        case TOK_ANDEQ:
        case TOK_OREQ:
        case TOK_XOREQ:
        case TOK_SHLEQ:
        case TOK_SHREQ:     return typeInteger(type);
        case TOK_ANDAND:
        case TOK_OROR:      return type->kind == TYPE_BOOL;
        case TOK_PLUSPLUS:
        case TOK_MINUSMINUS:return typeInteger(type);
        case TOK_EQEQ:
        case TOK_LESS:
        case TOK_GREATER:   return typeOrdinal(type) || typeReal(type);
        case TOK_EQ:        return true;
        case TOK_NOT:       return type->kind == TYPE_BOOL;
        case TOK_NOTEQ:
        case TOK_LESSEQ:
        case TOK_GREATEREQ: return typeOrdinal(type) || typeReal(type);
        default:            return false;
    }
}


bool typeAssertValidOperator(Types *types, Type *type, TokenKind op)
{
    bool res = typeValidOperator(type, op);
    if (!res)
        types->error("Operator %s is not applicable to %s", lexSpelling(op), typeSpelling(type));
    return res;
}


Field *typeFindField(Type *structType, char *name)
{
    if (structType->kind == TYPE_STRUCT)
    {
        int nameHash = hash(name);
        for (int i = 0; i < structType->numItems; i++)
            if (structType->field[i]->hash == nameHash && strcmp(structType->field[i]->name, name) == 0)
                return structType->field[i];
    }
    return NULL;
}


Field *typeAssertFindField(Types *types, Type *structType, char *name)
{
    Field *res = typeFindField(structType, name);
    if (!res)
        types->error("Unknown field %s", name);
    return res;
}


void typeAddField(Types *types, Type *structType, Type *fieldType, char *name)
{
    Field *field = typeFindField(structType, name);
    if (field)
        types->error("Duplicate field %s", name);

    if (structType->numItems > MAX_FIELDS)
        types->error("Too many fields");

    field = malloc(sizeof(Field));

    strcpy(field->name, name);
    field->hash = hash(name);
    field->type = fieldType;
    field->offset = typeSize(types, structType);

    structType->field[structType->numItems++] = field;
}


Param *typeFindParam(Signature *sig, char *name)
{
    int nameHash = hash(name);
    for (int i = 0; i < sig->numParams; i++)
        if (sig->param[i]->hash == nameHash && strcmp(sig->param[i]->name, name) == 0)
            return sig->param[i];

    return NULL;
}


void typeAddParam(Types *types, Signature *sig, Type *type, char *name)
{
    Param *param = typeFindParam(sig, name);
    if (param)
        types->error("Duplicate parameter %s", name);

    if (sig->numParams > MAX_PARAMS)
        types->error("Too many parameters");

    param = malloc(sizeof(Param));

    strcpy(param->name, name);
    param->hash = hash(name);
    param->type = type;

    sig->param[sig->numParams++] = param;
}


static char *typeSpellingRecursive(Type *type, char *buf)
{
    static char fullSpelling[DEFAULT_STRING_LEN];
    if (!buf) buf = fullSpelling;

    if (type->kind == TYPE_PTR || type->kind == TYPE_ARRAY)
    {
        sprintf(buf, "%s%s", spelling[type->kind], typeSpellingRecursive(type->base, buf + strlen(spelling[type->kind])));
        return buf;
    }
   else
        return spelling[type->kind];
}


char *typeSpelling(Type *type)
{
    return typeSpellingRecursive(type, NULL);
}
