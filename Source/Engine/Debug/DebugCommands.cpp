// Copyright (c) Wojciech Figat. All rights reserved.

#include "DebugCommands.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/Task.h"
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

    static void PrettyPrint(StringBuilder& sb, const Variant& value)
    {
        if (value.Type.Type == VariantType::Array)
        {
            // Prettify array printing
            auto& resultArray = value.AsArray();
            sb.Append('[');
            for (int32 i = 0; i < resultArray.Count(); i++)
            {
                if (i > 0)
                    sb.Append(',').Append(' ');
                PrettyPrint(sb, resultArray[i]);
                if (i > 30) // Limit on too large values
                {
                    sb.Append(TEXT("..."));
                    break;
                }
            }
            sb.Append(']');
        }
        else if (value.Type.Type == VariantType::Structure)
        {
            // Prettify structure printing
            ScriptingTypeHandle resultType = Scripting::FindScriptingType(value.Type.GetTypeName());
            if (resultType)
            {
                Array<void*> fields;
                resultType.Module->GetFields(resultType, fields);
                sb.Append('{');
                Variant fieldValue;
                ScriptingTypeFieldSignature fieldSig;
                bool first = true;
                for (void* field : fields)
                {
                    resultType.Module->GetFieldSignature(field, fieldSig);
                    if (fieldSig.IsStatic)
                        continue;
                    if (!resultType.Module->GetFieldValue(field, value, fieldValue))
                    {
                        if (!first)
                            sb.Append(',');
                        first = false;
                        sb.Append(' ').Append(String(fieldSig.Name)).Append(':').Append(' ');
                        PrettyPrint(sb, fieldValue);
                    }
                }
                sb.Append(' ').Append('}');
            }
        }
        else
        {
            sb.Append(value.ToString());
        }
    }

    void Invoke(StringView args) const
    {
        PROFILE_CPU();

        // Get command signature
        Array<ScriptingTypeMethodSignature::Param, InlinedAllocation<16>> sigParams;
        VariantType sigValue;
        if (Method)
        {
            ScriptingTypeMethodSignature sig;
            Module->GetMethodSignature(Method, sig);
            sigParams = MoveTemp(sig.Params);
            sigValue = MoveTemp(sig.ReturnType);
        }
        else if (Field)
        {
            ScriptingTypeFieldSignature sig;
            Module->GetFieldSignature(Field, sig);
            auto& p = sigParams.AddOne();
            p.IsOut = false;
            p.Type = sig.ValueType;
            sigValue = MoveTemp(sig.ValueType);
        }
        else if (MethodSet && args.HasChars())
        {
            ScriptingTypeMethodSignature sig;
            Module->GetMethodSignature(MethodSet, sig);
            sigParams = MoveTemp(sig.Params);
            sigParams.Resize(1);
        }
        else if (MethodGet && args.IsEmpty())
        {
            ScriptingTypeMethodSignature sig;
            Module->GetMethodSignature(MethodGet, sig);
            sigValue = MoveTemp(sig.ReturnType);
        }

        // Parse arguments
        if (args == StringView(TEXT("?"), 1))
        {
            LOG(Warning, "TODO: debug commands help/docs printing"); // TODO: debug commands help/docs printing (use CodeDocsModule that parses XML docs)
            return;
        }
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
        LOG(Info, "> {}{}{}", Name, args.Length() != 0 ? TEXT(" ") : TEXT(""), args);
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
        else if (args.HasChars())
        {
            LOG(Warning, "Property {} doesn't have a setter (read-only)", Name);
        }
        else if (args.IsEmpty())
        {
            LOG(Warning, "Property {} doesn't have a getter (write-only)", Name);
        }

        // Print result
        if (result != Variant())
        {
            StringBuilder sb;
            PrettyPrint(sb, result);
            LOG_STR(Info, sb.ToStringView());
        }
        else if (args.IsEmpty() && sigValue.Type != VariantType::Void && sigValue.Type != VariantType::Null)
        {
            LOG_STR(Info, TEXT("null"));
        }
    }
};

namespace
{
    CriticalSection Locker;
    bool Inited = false;
    Task* AsyncTask = nullptr;
    Array<CommandData> Commands;

    void FindDebugCommands(BinaryModule* module)
    {
        if (module == GetBinaryModuleCorlib())
            return;
        PROFILE_CPU();

#if USE_CSHARP
        if (auto* managedModule = dynamic_cast<ManagedBinaryModule*>(module))
        {
            const MClass* attribute = ((NativeBinaryModule*)GetBinaryModuleFlaxEngine())->Assembly->GetClass("FlaxEngine.DebugCommand");
            ASSERT_LOW_LAYER(attribute);
            const auto& classes = managedModule->Assembly->GetClasses();
            for (const auto& e : classes)
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
                    if (useClass && method->GetVisibility() != MVisibility::Public)
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
                    if (useClass && field->GetVisibility() != MVisibility::Public)
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
                    if (useClass && property->GetVisibility() != MVisibility::Public)
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
        ASSERT_LOW_LAYER(!Inited);
        PROFILE_CPU();

        // Cache existing modules
        const auto& modules = BinaryModule::GetModules();
        for (BinaryModule* module : modules)
        {
            FindDebugCommands(module);
        }

        // Link for modules load/reload actions
        Scripting::BinaryModuleLoaded.Bind(&FindDebugCommands);
        Scripting::ScriptsReloading.Bind(&OnScriptsReloading);

        // Mark as done
        Locker.Lock();
        Inited = true;
        AsyncTask = nullptr;
        Locker.Unlock();
    }

    void EnsureInited()
    {
        // Check current state
        Locker.Lock();
        bool inited = Inited;
        Locker.Unlock();
        if (inited)
            return;

        // Wait for any async task
        if (AsyncTask)
            AsyncTask->Wait();

        // Do sync init if still not inited
        Locker.Lock();
        if (!Inited)
            InitCommands();
        Locker.Unlock();
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
        if (AsyncTask)
            AsyncTask->Wait();
        ScopeLock lock(Locker);
        Scripting::BinaryModuleLoaded.Unbind(&FindDebugCommands);
        Scripting::ScriptsReloading.Unbind(&OnScriptsReloading);
        Commands.Clear();
        Inited = true;
    }
};

DebugCommandsService DebugCommandsServiceInstance;

void DebugCommands::Execute(StringView command)
{
    // TODO: fix missing string handle on 1st command execution (command gets invalid after InitCommands due to dotnet GC or dotnet interop handles flush)
    String commandCopy = command;
    command = commandCopy;

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
    EnsureInited();
    ScopeLock lock(Locker);

    // Find command to run
    for (const CommandData& cmd : Commands)
    {
        if (name.Length() == cmd.Name.Length() &&
            StringUtils::CompareIgnoreCase(name.Get(), cmd.Name.Get(), name.Length()) == 0)
        {
            cmd.Invoke(args);
            return;
        }
    }

    LOG(Error, "Unknown command '{}'", name);
}

void DebugCommands::Search(StringView searchText, Array<StringView>& matches, bool startsWith)
{
    if (searchText.IsEmpty())
        return;
    // TODO: fix missing string handle on 1st command execution (command gets invalid after InitCommands due to dotnet GC or dotnet interop handles flush)
    String searchTextCopy = searchText;
    searchText = searchTextCopy;

    EnsureInited();
    ScopeLock lock(Locker);

    if (startsWith)
    {
        for (auto& command : Commands)
        {
            if (command.Name.StartsWith(searchText, StringSearchCase::IgnoreCase))
            {
                matches.Add(command.Name);
            }
        }
    }
    else
    {
        for (auto& command : Commands)
        {
            if (command.Name.Contains(searchText.Get(), StringSearchCase::IgnoreCase))
            {
                matches.Add(command.Name);
            }
        }
    }
}

void DebugCommands::InitAsync()
{
    ScopeLock lock(Locker);
    if (Inited || AsyncTask)
        return;
    AsyncTask = Task::StartNew(InitCommands);
}

DebugCommands::CommandFlags DebugCommands::GetCommandFlags(StringView command)
{
    CommandFlags result = CommandFlags::None;
    // TODO: fix missing string handle on 1st command execution (command gets invalid after InitCommands due to dotnet GC or dotnet interop handles flush)
    String commandCopy = command;
    command = commandCopy;
    EnsureInited();
    for (auto& e : Commands)
    {
        if (e.Name == command)
        {
            if (e.Method)
                result |= CommandFlags::Exec;
            else if (e.Field)
                result |= CommandFlags::ReadWrite;
            if (e.MethodGet)
                result |= CommandFlags::Read;
            if (e.MethodSet)
                result |= CommandFlags::Write;
            break;
        }
    }
    return result;
}

bool DebugCommands::Iterate(const StringView& searchText, int32& index)
{
    EnsureInited();
    if (index >= 0)
    {
        ScopeLock lock(Locker);
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

StringView DebugCommands::GetCommandName(int32 index)
{
    ScopeLock lock(Locker);
    CHECK_RETURN(Commands.IsValidIndex(index), String::Empty);
    return Commands.Get()[index].Name;
}
