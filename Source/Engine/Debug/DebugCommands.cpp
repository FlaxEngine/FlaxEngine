// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "DebugCommands.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#include "FlaxEngine.Gen.h"

struct CommandData
{
    String Name;
    BinaryModule* Module;
    void* Method = nullptr;
    void* MethodGet = nullptr;
    void* MethodSet = nullptr;
    void* Field = nullptr;

    void Invoke(StringView args) const
    {
        PROFILE_CPU();

        // Get command signature
        Array<ScriptingTypeMethodSignature::Param, InlinedAllocation<16>> sigParams;
        if (Method)
        {
            ScriptingTypeMethodSignature sig;
            Module->GetMethodSignature(Method, sig);
            sigParams = MoveTemp(sig.Params);
        }
        else if (Field)
        {
            ScriptingTypeFieldSignature sig;
            Module->GetFieldSignature(Field, sig);
            auto& p = sigParams.AddOne();
            p.IsOut = false;
            p.Type = sig.ValueType;
        }
        else if (MethodSet && args.HasChars())
        {
            ScriptingTypeMethodSignature sig;
            Module->GetMethodSignature(MethodSet, sig);
            sigParams = MoveTemp(sig.Params);
            sigParams.Resize(1);
        }

        // Parse arguments
        Array<Variant> params;
        params.Resize(sigParams.Count());
        Array<String> argsSeparated;
        String argsStr(args);
        argsStr.Split(' ', argsSeparated);
        for (int32 i = 0; i < argsSeparated.Count() && i < params.Count(); i++)
        {
            params[i] = Variant::Parse(argsSeparated[i], sigParams[i].Type);
        }

        // Call command
        Variant result;
        if (Method)
        {
            Module->InvokeMethod(Method, Variant::Null, ToSpan(params), result);
        }
        else if (Field)
        {
            if (args.IsEmpty())
                Module->GetFieldValue(Field, Variant::Null, result);
            else
                Module->SetFieldValue(Field, Variant::Null, params[0]);
        }
        else if (MethodGet && args.IsEmpty())
        {
            Module->InvokeMethod(MethodGet, Variant::Null, ToSpan(params), result);
        }
        else if (MethodSet && args.HasChars())
        {
            Module->InvokeMethod(MethodSet, Variant::Null, ToSpan(params), result);
        }

        // Print result
        if (result != Variant())
        {
            LOG_STR(Info, result.ToString());
        }
    }
};

namespace
{
    CriticalSection Locker;
    bool Inited = false;
    Array<CommandData> Commands;

    void OnBinaryModuleLoaded(BinaryModule* module)
    {
        if (module == GetBinaryModuleCorlib())
            return;

#if USE_CSHARP
        if (auto* managedModule = dynamic_cast<ManagedBinaryModule*>(module))
        {
            const MClass* attribute = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly->GetClass("FlaxEngine.DebugCommand");
            ASSERT_LOW_LAYER(attribute);
            const auto& classes = managedModule->Assembly->GetClasses();
            for (auto e : classes)
            {
                MClass* mclass = e.Value;
                if (mclass->IsGeneric() ||
                    mclass->IsInterface() ||
                    mclass->IsEnum())
                    continue;
                const bool useClass = mclass->HasAttribute(attribute);
                // TODO: optimize this via stack-based format buffer and then convert Ansi to UTF16
#define BUILD_NAME(commandData, itemName) commandData.Name = String(mclass->GetName()) + TEXT(".") + String(itemName)

                // Process methods
                const auto& methods = mclass->GetMethods();
                for (MMethod* method : methods)
                {
                    if (!method->IsStatic())
                        continue;
                    const StringAnsi& name = method->GetName();
                    if (name.Contains("Internal_") ||
                        mclass->GetFullName().Contains(".Interop."))
                        continue;
                    if (name.StartsWith("get_") ||
                        name.StartsWith("set_") ||
                        name.StartsWith("op_") ||
                        name.StartsWith("add_") ||
                        name.StartsWith("remove_"))
                        continue;
                    if (!useClass && !method->HasAttribute(attribute))
                        continue;

                    auto& commandData = Commands.AddOne();
                    BUILD_NAME(commandData, method->GetName());
                    commandData.Module = module;
                    commandData.Method = method;
                }

                // Process fields
                const auto& fields = mclass->GetFields();
                for (MField* field : fields)
                {
                    if (!field->IsStatic())
                        continue;
                    if (!useClass && !field->HasAttribute(attribute))
                        continue;

                    auto& commandData = Commands.AddOne();
                    BUILD_NAME(commandData, field->GetName());
                    commandData.Module = module;
                    commandData.Field = field;
                }

                // Process properties
                const auto& properties = mclass->GetProperties();
                for (MProperty* property : properties)
                {
                    if (!property->IsStatic())
                        continue;
                    if (!useClass && !property->HasAttribute(attribute))
                        continue;

                    auto& commandData = Commands.AddOne();
                    BUILD_NAME(commandData, property->GetName());
                    commandData.Module = module;
                    commandData.MethodGet = property->GetGetMethod();
                    commandData.MethodSet = property->GetSetMethod();
                }
            }
#undef BUILD_NAME
        }
        else
#endif
        {
            // TODO: implement generic search for other modules (eg. Visual Scripts)
        }
    }

    void OnScriptsReloading()
    {
        // Reset
        Inited = false;
        Commands.Clear();
    }

    void InitCommands()
    {
        PROFILE_CPU();
        Inited = true;
        const auto& modules = BinaryModule::GetModules();
        for (BinaryModule* module : modules)
        {
            OnBinaryModuleLoaded(module);
        }
        Scripting::BinaryModuleLoaded.Bind(&OnBinaryModuleLoaded);
        Scripting::ScriptsReloading.Bind(&OnScriptsReloading);
    }
}

class DebugCommandsService : public EngineService
{
public:
    DebugCommandsService()
        : EngineService(TEXT("DebugCommands"), 0)
    {
    }

    void Dispose() override
    {
        // Cleanup
        ScopeLock lock(Locker);
        Scripting::BinaryModuleLoaded.Unbind(&OnBinaryModuleLoaded);
        Scripting::ScriptsReloading.Unbind(&OnScriptsReloading);
        Commands.Clear();
        Inited = true;
    }
};

DebugCommandsService DebugCommandsServiceInstance;

void DebugCommands::Execute(StringView command)
{
    // Preprocess command text
    while (command.HasChars() && StringUtils::IsWhitespace(command[0]))
        command = StringView(command.Get() + 1, command.Length() - 1);
    while (command.HasChars() && StringUtils::IsWhitespace(command[command.Length() - 1]))
        command = StringView(command.Get(), command.Length() - 1);
    if (command.IsEmpty())
        return;
    StringView name = command;
    StringView args;
    int32 argsStart = name.Find(' ');
    if (argsStart != -1)
    {
        name = command.Left(argsStart);
        args = command.Right(argsStart + 1);
    }

    // Ensure that commands cache has been created
    ScopeLock lock(Locker);
    if (!Inited)
        InitCommands();

    // Find command to run
    for (const CommandData& command : Commands)
    {
        if (name.Length() == command.Name.Length() &&
            StringUtils::CompareIgnoreCase(name.Get(), command.Name.Get(), name.Length()) == 0)
        {
            command.Invoke(args);
            return;
        }
    }

    LOG(Error, "Unknown command '{}'", name);
}

bool DebugCommands::Iterate(const StringView& searchText, int32& index)
{
    ScopeLock lock(Locker);
    if (index >= 0)
    {
        if (!Inited)
            InitCommands();
        while (index < Commands.Count())
        {
            auto& command = Commands.Get()[index];
            if (command.Name.StartsWith(searchText, StringSearchCase::IgnoreCase))
            {
                return true;
            }
            index++;
        }
    }
    return false;
}

String DebugCommands::GetCommandName(int32 index)
{
    ScopeLock lock(Locker);
    CHECK_RETURN(Commands.IsValidIndex(index), String::Empty);
    return Commands.Get()[index].Name;
}
