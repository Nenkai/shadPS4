// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/game_hooks/LightHook.h"
#include "core/game_hooks/adhoc_structs.h"
#include "core/game_hooks/gt7_hooks.h"

#include "common/singleton.h"
#include "core/linker.h"
// #include "common/version.h"

#include "magic_enum/magic_enum.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <string_view>

namespace GT7Hooks {

#define HOOK_FUNC SHAD_NO_INLINE PS4_SYSV_ABI

static u64 EBOOT_MODULE_BASE;
static bool initted = false;

constexpr auto GT7_PDI_PDISTD_logger_LoggerOutputNull_print_V100 = 0x5C07258;
constexpr auto GT7_PDI_PrintTarget_vtable_V100 = 0x5C00260;
constexpr auto GT7_PDI_vsprintf_V100 = 0x3A4E740;
constexpr auto GT7_AdhocThrow_V100 = 0x30BCFA0;
constexpr auto GT7_AdhocCompile_V100 = 0x2FECA70;
constexpr auto GT7_SymbolMapAdd_V100 = 0x30FFF10;

static HookInformation SymbolMapAdd_hook = {};
static HookInformation AdhocThrow_hook = {};
static HookInformation AdhocCompile_hook = {};

void PS4_SYSV_ABI ThrowImpl(char* a1, char* a2, int a3, char* a4, char* a5) {

    LOG_INFO(Core_Hooking, "{}{}{}{}{}", a1, a2, a3, a4, a5);
}

/*
std::map<void*, std::string> test{};

void* PS4_SYSV_ABI SymbolMapAddImpl(void* symbolMap, char* name, u32 length) {

    auto orig = (void* PS4_SYSV_ABI (*)(void*, char*, u32))SymbolMapAdd_hook.Trampoline;
    auto res = orig(symbolMap, name, length);

    std::string myStr(name, length);
    test[res] = myStr;
    return res;
}
*/

void PS4_SYSV_ABI GT7Logger(void* logger, void* a2, char* path, int lineNumber, int type, int a6,
                            char* format, char* vaList) {
    u8 buffer[1024];
    PrintTarget target = {.vtable = EBOOT_MODULE_BASE + GT7_PDI_PrintTarget_vtable_V100,
                          .buffer = buffer,
                          .bufferSize = 1024};

    auto func = (int PS4_SYSV_ABI (*)(PrintTarget*, char*, void*))(EBOOT_MODULE_BASE +
                                                                   GT7_PDI_vsprintf_V100);
    int ret = func(&target, format, vaList);

    LOG_INFO(Core_Hooking, "{}", std::string_view(reinterpret_cast<char*>(buffer), ret));
}

void DumpSubroutine(mCodeGT7* mCode, int depth = 0) {

    for (auto& inst : mCode->Instructions) {
        for (int i = 0; i < depth; i++)
            fmt::print("  ");

        switch (inst->type) {
        case AdhocInstructionType::VARIABLE_EVAL: {
            mVariableEval* eval = reinterpret_cast<mVariableEval*>(inst);
            std::string name = eval->SymbolList.GetModulePath();
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), name);
        } break;
        case AdhocInstructionType::VARIABLE_PUSH: {
            mVariablePush* push = reinterpret_cast<mVariablePush*>(inst);
            std::string name = push->SymbolList.GetModulePath();
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), name);
        } break;
        case AdhocInstructionType::ATTRIBUTE_EVAL: {
            mAttributeEval* eval = reinterpret_cast<mAttributeEval*>(inst);
            std::string name = eval->SymbolList.GetModulePath();
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), name);
        } break;
        case AdhocInstructionType::ATTRIBUTE_PUSH: {
            mAttributePush* push = reinterpret_cast<mAttributePush*>(inst);
            std::string name = push->SymbolList.GetModulePath();
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), name);
        } break;
        case AdhocInstructionType::INT_CONST: {
            mIntConst* intConst = reinterpret_cast<mIntConst*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), intConst->Value);
        } break;
        case AdhocInstructionType::U_INT_CONST: {
            mUIntConst* uintConst = reinterpret_cast<mUIntConst*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), uintConst->Value);
        } break;
        case AdhocInstructionType::FLOAT_CONST: {
            mFloatConst* floatConst = reinterpret_cast<mFloatConst*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), floatConst->Value);
        } break;
        case AdhocInstructionType::JUMP_IF_TRUE: {
            mJumpIfTrue* jumpIfTrue = reinterpret_cast<mJumpIfTrue*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), jumpIfTrue->Target);
        } break;
        case AdhocInstructionType::JUMP_IF_FALSE: {
            mJumpIfFalse* jumpIfFalse = reinterpret_cast<mJumpIfFalse*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     jumpIfFalse->Target);
        } break;
        case AdhocInstructionType::JUMP: {
            mJump* jump = reinterpret_cast<mJump*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), jump->Target);
        } break;
        case AdhocInstructionType::JUMP_IF_NIL: {
            mJumpIfNil* jump = reinterpret_cast<mJumpIfNil*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), jump->Target);
        } break;
        case AdhocInstructionType::CALL: {
            mCall* call = reinterpret_cast<mCall*>(inst);
            LOG_INFO(Core_Hooking, "{}: {} args", magic_enum::enum_name(inst->type), call->NumArgs);
        } break;
        case AdhocInstructionType::SET_STATE: {
            mSetState* setState = reinterpret_cast<mSetState*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     magic_enum::enum_name(setState->State));
        } break;
        case AdhocInstructionType::LIST_ASSIGN: {
            mListAssign* listAssign = reinterpret_cast<mListAssign*>(inst);
            LOG_INFO(Core_Hooking, "{}: {} elems, rest: {}", magic_enum::enum_name(inst->type),
                     listAssign->NumArgs, listAssign->HasRestElement);
        } break;
        case AdhocInstructionType::IMPORT: {
            mImport* import = reinterpret_cast<mImport*>(inst);
            LOG_INFO(Core_Hooking, "{}: path:{}, target:{}, alias:{}",
                     magic_enum::enum_name(inst->type), import->Path.GetModulePath(),
                     import->Target->GetName(), import->Alias->GetName());
        } break;
        case AdhocInstructionType::UNDEF: {
            mUndef* undef = reinterpret_cast<mUndef*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     undef->Path.GetModulePath());
        } break;
        case AdhocInstructionType::MODULE_DEFINE: {
            mModuleDefine* def = reinterpret_cast<mModuleDefine*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     def->Path.GetModulePath());
        } break;
        case AdhocInstructionType::STRING_CONST: {
            mStringConst* str = reinterpret_cast<mStringConst*>(inst);
            LOG_INFO(Core_Hooking, "{}: \"{}\"", magic_enum::enum_name(inst->type),
                     str->Str.GetName());
        } break;
        case AdhocInstructionType::SYMBOL_CONST: {
            mSymbolConst* str = reinterpret_cast<mSymbolConst*>(inst);
            LOG_INFO(Core_Hooking, "{}: \'{}\'", magic_enum::enum_name(inst->type),
                     str->Str->GetName());
        } break;
        case AdhocInstructionType::STRING_PUSH: {
            mStringPush* push = reinterpret_cast<mStringPush*>(inst);
            LOG_INFO(Core_Hooking, "{}: {} elems", magic_enum::enum_name(inst->type),
                     push->NumArgs);
        } break;
        case AdhocInstructionType::DELEGATE_DEFINE: {
            mDelegateDefine* def = reinterpret_cast<mDelegateDefine*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     def->Name->GetName());
        } break;
        case AdhocInstructionType::STATIC_DEFINE: {
            mStaticDefine* def = reinterpret_cast<mStaticDefine*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     def->Name->GetName());
        } break;
        case AdhocInstructionType::ATTRIBUTE_DEFINE: {
            mAttributeDefine* def = reinterpret_cast<mAttributeDefine*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type),
                     def->Name->GetName());
        } break;
        case AdhocInstructionType::FUNCTION_CONST: {
            mFunctionConst* func = reinterpret_cast<mFunctionConst*>(inst);
            LOG_INFO(Core_Hooking, "{}", magic_enum::enum_name(inst->type));
            DumpSubroutine(func->Code, depth + 1);
        } break;
        case AdhocInstructionType::CLASS_DEFINE: {
            mClassDefine* def = reinterpret_cast<mClassDefine*>(inst);
            LOG_INFO(Core_Hooking, "{}: {} : {}", magic_enum::enum_name(inst->type),
                         def->Name->GetName(), def->InheritSymbols.GetModulePath());

        } break;
        case AdhocInstructionType::FUNCTION_DEFINE: {
            mFunctionDefine* func = reinterpret_cast<mFunctionDefine*>(inst);
            LOG_INFO(Core_Hooking, "{} - {}", magic_enum::enum_name(inst->type),
                     func->Code->Name.GetName());
            DumpSubroutine(func->Code, depth + 1);
        } break;
        case AdhocInstructionType::LOGICAL_AND: {
            mLogicalAnd* logic = reinterpret_cast<mLogicalAnd*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), logic->Target);
        } break;
        case AdhocInstructionType::LOGICAL_OR: {
            mLogicalOr* logic = reinterpret_cast<mLogicalOr*>(inst);
            LOG_INFO(Core_Hooking, "{}: {}", magic_enum::enum_name(inst->type), logic->Target);
        } break;
        default:
            LOG_INFO(Core_Hooking, "{}", magic_enum::enum_name(inst->type));
        }
    }
}

void HOOK_FUNC CompileImpl(HCodeGT7* a1, StringStruct* a2) {
    auto orig = (void PS4_SYSV_ABI (*)(HCodeGT7*, StringStruct*))AdhocCompile_hook.Trampoline;
    // orig(a1, a2);

    // logFilter = *:Critical Core.Hooking:Info
    // logType = sync

    while (true) {
        const std::string inputFile = "code.ad";
        std::ifstream infile(inputFile, std::ios_base::binary);
        if (infile.good()) {
            infile.seekg(0, std::ios_base::end);
            size_t length = infile.tellg();
            infile.seekg(0, std::ios_base::beg);

            std::vector<char> buffer;
            buffer.reserve(0x1000);
            std::copy(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(),
                      std::back_inserter(buffer));

            LOG_INFO(Core_Hooking, "Compiling:\n{}",
                     std::string_view(buffer.data(), buffer.size()));

            HCodeGT7 ourCompiledCode;
            StringStruct ourString;
            ourString.FormatStr = buffer.data();
            ourString.FormattedCode = buffer.data();
            ourString.StrLen = buffer.size();
            ourString.Capacity = buffer.capacity();
            orig(&ourCompiledCode, &ourString);

            if (ourCompiledCode.Code && ourCompiledCode.Code->UnkCodePart) {
                auto cur = ourCompiledCode.Code->UnkCodePart->next;
                while (cur != ourCompiledCode.Code->UnkCodePart) {
                    auto mCode = cur->data;
                    DumpSubroutine(mCode);
                    cur = cur->next;
                }
            }
        }
        else
        {
            LOG_INFO(Core_Hooking, "code.ad not found");
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void InitHook(HookInformation& hook, u64 offset, void* func) {
    hook = CreateHook(reinterpret_cast<void*>(EBOOT_MODULE_BASE + offset), func);
    EnableHook(&hook);
}

void Initialize(Core::Module* mainModule) {

    if (initted)
        return;

    EBOOT_MODULE_BASE = mainModule->GetBaseAddress();

    u64* printFuncAddr = reinterpret_cast<u64*>(EBOOT_MODULE_BASE +
                                                GT7_PDI_PDISTD_logger_LoggerOutputNull_print_V100);
    *printFuncAddr = reinterpret_cast<u64>(&GT7Logger);

    // InitHook(SymbolMapAdd_hook, GT7_SymbolMapAdd_V100, SymbolMapAddImpl);
    // InitHook(AdhocThrow_hook, GT7_AdhocThrow_V100, ThrowImpl);
    InitHook(AdhocCompile_hook, GT7_AdhocCompile_V100, CompileImpl);

    initted = true;
}
} // namespace GT7Hooks