#pragma once

#include <iostream>
#include <vector>

enum class AdhocInstructionType : char {
    /// <summary>
    /// Also known as ARRAY_PUSH (not the new one)
    /// </summary>
    ARRAY_CONST_OLD,
    ASSIGN_OLD,
    ATTRIBUTE_DEFINE,
    ATTRIBUTE_PUSH,
    BINARY_ASSIGN_OPERATOR,
    BINARY_OPERATOR,
    CALL,
    CLASS_DEFINE,
    EVAL,
    FLOAT_CONST,
    FUNCTION_DEFINE,
    IMPORT,
    INT_CONST,
    JUMP,
    JUMP_IF_TRUE,  // Also known as JUMP_NOT_ZERO
    JUMP_IF_FALSE, // Also known as JUMP_ZERO
    LIST_ASSIGN_OLD,
    LOCAL_DEFINE,
    LOGICAL_AND_OLD,
    LOGICAL_OR_OLD,
    METHOD_DEFINE,
    MODULE_DEFINE,
    NIL_CONST,
    NOP,
    POP_OLD,
    PRINT,
    REQUIRE,
    SET_STATE_OLD,
    STATIC_DEFINE,
    STRING_CONST,
    STRING_PUSH,
    THROW,
    TRY_CATCH,
    UNARY_ASSIGN_OPERATOR,
    UNARY_OPERATOR,
    UNDEF,
    VARIABLE_PUSH,
    ATTRIBUTE_EVAL,
    VARIABLE_EVAL,
    SOURCE_FILE,

    // GTHD Release (V10)
    FUNCTION_CONST,
    METHOD_CONST,
    MAP_CONST_OLD,
    LONG_CONST,
    ASSIGN,
    LIST_ASSIGN,
    CALL_OLD,

    // GT5P JP Demo (V10)
    OBJECT_SELECTOR, // Also known as SELF_SELECTOR earlier than GT5P Demo
    SYMBOL_CONST,
    LEAVE, // Also known as CODE_CONST earlier than GT5P Demo

    // V11
    ARRAY_CONST,
    ARRAY_PUSH,
    MAP_CONST,
    MAP_INSERT,
    POP,
    SET_STATE,
    VOID_CONST,
    ASSIGN_POP,

    // GT5 Spec 3 (V12)
    U_INT_CONST,
    U_LONG_CONST,
    DOUBLE_CONST,

    // GT5 TT Challenge (V12)
    ELEMENT_PUSH,
    ELEMENT_EVAL,
    LOGICAL_AND,
    LOGICAL_OR,
    BOOL_CONST,
    MODULE_CONSTRUCTOR,

    // GT6 (V12)
    VA_CALL,
    CODE_EVAL,

    // GT Sport (V12)
    DELEGATE_DEFINE,
    JUMP_IF_NIL,
    LOGICAL_OPTIONAL,

    // GT7 (V13)
    BYTE_CONST = 72,
    U_BYTE_CONST = 73,
    SHORT_CONST = 74,
    U_SHORT_CONST = 75,
};

typedef struct {
    void* unk;
    char* FormattedCode;
    char* FormatStr;
    size_t StrLen;
    size_t Capacity;
} StringStruct;

// std::string
struct String {
    void* unk;
    char data[8];
    void* unk2;
    size_t strLen;

    char* GetName() {
        if (strLen >= 0x10)
            return *(char**)data;
        else
            return (char*)(&data);
    }
};

struct SymbolList {
    void* unk;
    std::vector<String*> vector;

    std::string GetModulePath() {
        std::string str;
        for (auto i = 0; i < vector.size(); i++)
        {
            str.append(vector[i]->GetName());
            if (i != vector.size() - 1)
                str.append(",");
        }

        return str;
    }
};

typedef struct
{
    String* Name;
    void* Unk2;
} UnkSymbol;

typedef struct {
    AdhocInstructionType type;
    char pad[3];
} hInst;

typedef struct {
    void* vtable;
    void* field_0x08;
    void* field_0x10;
    void* field_0x18;
    void* field_0x20;
    void* field_0x28;
    void* field_0x30;
    std::vector<UnkSymbol> CallbackVariables;
    void* field_0x50;
    void* field_0x58;
    void* field_0x60;
    void* field_0x68;
    void* field_0x70;
    void* field_0x78;
    void* field_0x80;
    String Name;
    void* field_0xA8;
    int field_0xB0;
    int field_0xB4;
    void* field_0xB8;
    std::vector<hInst*> Instructions;
    void* field_0xD8;
    void* field_0xE0;
} mCodeGT7;

struct mCodeListEntry;
struct mCodeListEntry {
    mCodeListEntry* next;
    mCodeListEntry* prev;
    mCodeGT7* data;
};

struct mVariableEval : hInst {
    int field_0x04;
    SymbolList SymbolList;
    int field_0x28;
};

struct mVariablePush : hInst {
    int field_0x04;
    SymbolList SymbolList;
    int field_0x28;
};

struct mAttributeEval : hInst {
    int field_0x04;
    SymbolList SymbolList;
    int field_0x28;
};

struct mAttributePush : hInst {
    int field_0x04;
    SymbolList SymbolList;
};

struct mIntConst : hInst {
    int Value;
};

struct mUIntConst : hInst {
    unsigned int Value;
};

struct mJumpIfTrue : hInst {
    unsigned int Target;
};

struct mJumpIfFalse : hInst {
    unsigned int Target;
};

struct mJump : hInst {
    unsigned int Target;
};

struct mJumpIfNil : hInst {
    unsigned int Target;
};

struct mStringPush : hInst {
    unsigned int NumArgs;
};

struct mCall : hInst {
    unsigned int NumArgs;
};

struct mListAssign : hInst {
    unsigned int NumArgs;
    bool HasRestElement;
};

struct mImport : hInst {
    int unk;
    SymbolList Path;
    String* Target;
    String* Alias;
};

struct mUndef : hInst {
    int unk;
    SymbolList Path;
};

struct mModuleDefine : hInst {
    int unk;
    SymbolList Path;
};

struct mStringConst : hInst {
    int unk;
    String Str;
};

struct mSymbolConst : hInst {
    int unk;
    String* Str;
};

struct mFloatConst : hInst {
    float Value;
};


struct mDelegateDefine : hInst {
    int unk;
    String* Name;
};

struct mStaticDefine : hInst {
    int unk;
    String* Name;
};

struct mAttributeDefine : hInst {
    int unk;
    String* Name;
};

enum class AdhocRunState : char {
    /// <summary>
    /// Script is terminating
    /// </summary>
    EXIT = 0,

    /// <summary>
    /// Script scope is over
    /// </summary>
    RETURN = 1,

    YIELD = 2,

    /// <summary>
    /// Script exception
    /// </summary>
    EXCEPTION = 3,

    CALL = 4,

    RUN = 5,
};

struct mSetState : hInst {
    AdhocRunState State;
};

struct mFunctionConst : hInst {
    int unk;
    mCodeGT7* Code;
};

struct mFunctionDefine : hInst {
    int unk;
    void* Name;
    mCodeGT7* Code;
};

struct mClassDefine : hInst {
    int unk;
    String* Name;
    SymbolList InheritSymbols;
    void* field_0x28;
    void* field_0x30;
};

struct mLogicalOr : hInst {
    unsigned int Target;
};

struct mLogicalAnd : hInst {
    unsigned int Target;
};

struct mLeave : hInst {
    unsigned int Depth;
    unsigned int RewindLocalsStorageTo;
};

typedef struct {
    void* vtable;
    int dword8;
    int dwordC;
    int Version;
    char gap14[12];
    mCodeListEntry* UnkCodePart; // std::list<mCode>
    int field_24;
    __int64 field_28;
} hCode;

typedef struct {
    hCode* Code;
} HCodeGT7;