// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "VisualScript.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/Events.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MType.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Threading/MainThreadTask.h"
#include "FlaxEngine.Gen.h"

namespace
{
    struct VisualScriptThread
    {
        uint32 StackFramesCount;
        VisualScripting::StackFrame* Stack;
    };

    ThreadLocal<VisualScriptThread> ThreadStacks;
    VisualScriptingBinaryModule VisualScriptingModule;
    VisualScriptExecutor VisualScriptingExecutor;

    void PrintStack(LogType type)
    {
        const String stack = VisualScripting::GetStackTrace();
        Log::Logger::Write(type, TEXT("Visual Script stack trace:"));
        Log::Logger::Write(type, stack);
        Log::Logger::Write(type, TEXT(""));
    }
}

#if VISUAL_SCRIPT_DEBUGGING
Action VisualScripting::DebugFlow;
#endif

static_assert(TIsPODType<VisualScripting::StackFrame>::Value, "VisualScripting::StackFrame must be POD type.");
static_assert(TIsPODType<VisualScriptThread>::Value, "VisualScriptThread must be POD type.");

bool VisualScriptGraph::onNodeLoaded(Node* n)
{
    switch (n->GroupID)
    {
        // Function
    case 16:
        switch (n->TypeID)
        {
            // Invoke Method
        case 4:
            n->Data.InvokeMethod.Method = nullptr;
            break;
            // Get/Set Field
        case 7:
        case 8:
            n->Data.GetSetField.Field = nullptr;
            break;
        }
    }

    // Base
    return VisjectGraph<VisualScriptGraphNode, VisjectGraphBox, VisjectGraphParameter>::onNodeLoaded(n);
}

VisualScriptExecutor::VisualScriptExecutor()
{
    _perGroupProcessCall[2] = (ProcessBoxHandler)&VisualScriptExecutor::ProcessGroupConstants;
    _perGroupProcessCall[4] = (ProcessBoxHandler)&VisualScriptExecutor::ProcessGroupPacking;
    _perGroupProcessCall[6] = (ProcessBoxHandler)&VisualScriptExecutor::ProcessGroupParameters;
    _perGroupProcessCall[7] = (ProcessBoxHandler)&VisualScriptExecutor::ProcessGroupTools;
    _perGroupProcessCall[16] = (ProcessBoxHandler)&VisualScriptExecutor::ProcessGroupFunction;
    _perGroupProcessCall[17] = (ProcessBoxHandler)&VisualScriptExecutor::ProcessGroupFlow;
}

void VisualScriptExecutor::Invoke(const Guid& scriptId, int32 nodeId, int32 boxId, const Guid& instanceId, Variant& result) const
{
    auto script = Content::Load<VisualScript>(scriptId);
    if (!script)
        return;
    const auto node = script->Graph.GetNode(nodeId);
    if (!node)
        return;
    const auto box = node->GetBox(boxId);
    if (!box)
        return;
    auto instance = Scripting::FindObject<ScriptingObject>(instanceId);

    // Add to the calling stack
    VisualScripting::ScopeContext scope;
    auto& stack = ThreadStacks.Get();
    VisualScripting::StackFrame frame;
    frame.Script = script;
    frame.Node = node;
    frame.Box = box;
    frame.Instance = instance;
    frame.PreviousFrame = stack.Stack;
    frame.Scope = &scope;
    stack.Stack = &frame;
    stack.StackFramesCount++;

    // Call per group custom processing event
    const auto func = VisualScriptingExecutor._perGroupProcessCall[node->GroupID];
    (VisualScriptingExecutor.*func)(box, node, result);

    // Remove from the calling stack
    stack.StackFramesCount--;
    stack.Stack = frame.PreviousFrame;
}

VisjectExecutor::Value VisualScriptExecutor::eatBox(Node* caller, Box* box)
{
    // Check if graph is looped or is too deep
    auto& stack = ThreadStacks.Get();
    if (stack.StackFramesCount >= VISUAL_SCRIPT_GRAPH_MAX_CALL_STACK)
    {
        OnError(caller, box, TEXT("Graph is looped or too deep!"));
        return Value::Zero;
    }
#if !BUILD_RELEASE
    if (box == nullptr)
    {
        OnError(caller, box, TEXT("Null graph box!"));
        return Value::Zero;
    }
#endif
    const auto parentNode = box->GetParent<Node>();

    // Add to the calling stack
    VisualScripting::StackFrame frame = *stack.Stack;
    frame.Node = parentNode;
    frame.Box = box;
    frame.PreviousFrame = stack.Stack;
    stack.Stack = &frame;
    stack.StackFramesCount++;

#if VISUAL_SCRIPT_DEBUGGING
    // Debugger event
    VisualScripting::DebugFlow();
#endif

    // Call per group custom processing event
    Value value;
    const ProcessBoxHandler func = _perGroupProcessCall[parentNode->GroupID];
    (this->*func)(box, parentNode, value);

    // Remove from the calling stack
    stack.StackFramesCount--;
    stack.Stack = frame.PreviousFrame;

    return value;
}

VisjectExecutor::Graph* VisualScriptExecutor::GetCurrentGraph() const
{
    auto& stack = ThreadStacks.Get();
    return stack.Stack && stack.Stack->Script ? &stack.Stack->Script->Graph : nullptr;
}

void VisualScriptExecutor::ProcessGroupConstants(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Enum
    case 11:
    {
        value = node->Values[0];
        break;
    }
    default:
        VisjectExecutor::ProcessGroupConstants(box, node, value);
        break;
    }
}

void VisualScriptExecutor::ProcessGroupPacking(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Pack Structure
    case 26:
    {
        // Find type
        const StringView typeName(node->Values[0]);
        const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
        const StringAnsiView typeNameAnsiView(typeNameAnsi.Get(), typeName.Length());
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeNameAnsiView);
        if (!typeHandle)
        {
            const auto mclass = Scripting::FindClass(typeNameAnsiView);
            if (mclass)
            {
                // Fallback to C#-only types
                bool failed = false;
                auto instance = mclass->CreateInstance();
                value = instance;
                auto& layoutCache = node->Values[1];
                CHECK(layoutCache.Type.Type == VariantType::Blob);
                MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
                const byte version = stream.ReadByte();
                if (version == 1)
                {
                    int32 fieldsCount;
                    stream.ReadInt32(&fieldsCount);
                    for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
                    {
                        box = &node->Boxes[boxId];
                        String fieldName;
                        stream.ReadString(&fieldName, 11);
                        VariantType fieldType;
                        stream.ReadVariantType(&fieldType);
                        if (box && box->HasConnection())
                        {
                            StringAsANSI<40> fieldNameAnsi(*fieldName, fieldName.Length());
                            auto field = mclass->GetField(fieldNameAnsi.Get());
                            if (field)
                            {
                                Variant fieldValue = eatBox(node, box->FirstConnection());
                                field->SetValue(instance, MUtils::VariantToManagedArgPtr(fieldValue, field->GetType(), failed));
                            }
                        }
                    }
                }
            }
            else if (typeName.HasChars())
            {
                LOG(Error, "Missing type '{0}'", typeName);
                PrintStack(LogType::Error);
            }
            return;
        }
        const ScriptingType& type = typeHandle.GetType();

        // Allocate structure data and initialize it with native constructor
        value.SetType(VariantType(VariantType::Structure, typeNameAnsiView));

        // Setup structure fields
        auto& layoutCache = node->Values[1];
        CHECK(layoutCache.Type.Type == VariantType::Blob);
        MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
        const byte version = stream.ReadByte();
        if (version == 1)
        {
            int32 fieldsCount;
            stream.ReadInt32(&fieldsCount);
            for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
            {
                box = &node->Boxes[boxId];
                String fieldName;
                stream.ReadString(&fieldName, 11);
                VariantType fieldType;
                stream.ReadVariantType(&fieldType);
                if (box && box->HasConnection())
                {
                    const Variant fieldValue = eatBox(node, box->FirstConnection());
                    type.Struct.SetField(value.AsBlob.Data, fieldName, fieldValue);
                }
            }
        }
        break;
    }
        // Unpack Structure
    case 36:
    {
        // Get value with structure data
        const Variant structureValue = eatBox(node, node->GetBox(0)->FirstConnection());
        if (!node->GetBox(0)->HasConnection())
            return;

        // Find type
        const StringView typeName(node->Values[0]);
        const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
        const StringAnsiView typeNameAnsiView(typeNameAnsi.Get(), typeName.Length());
        const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeNameAnsiView);
        if (!typeHandle)
        {
            const auto mclass = Scripting::FindClass(typeNameAnsiView);
            if (mclass)
            {
                // Fallback to C#-only types
                auto instance = mono_gchandle_get_target(structureValue.AsUint);
                CHECK(instance);
                if (structureValue.Type.Type != VariantType::ManagedObject || mono_object_get_class(instance) != mclass->GetNative())
                {
                    LOG(Error, "Cannot unpack value of type {0} to structure of type {1}", String(MUtils::GetClassFullname(instance)), typeName);
                    PrintStack(LogType::Error);
                    return;
                }
                auto& layoutCache = node->Values[1];
                CHECK(layoutCache.Type.Type == VariantType::Blob);
                MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
                const byte version = stream.ReadByte();
                if (version == 1)
                {
                    int32 fieldsCount;
                    stream.ReadInt32(&fieldsCount);
                    for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
                    {
                        String fieldName;
                        stream.ReadString(&fieldName, 11);
                        VariantType fieldType;
                        stream.ReadVariantType(&fieldType);
                        if (box->ID == boxId)
                        {
                            StringAsANSI<40> fieldNameAnsi(*fieldName, fieldName.Length());
                            auto field = mclass->GetField(fieldNameAnsi.Get());
                            if (field)
                                value = MUtils::UnboxVariant(field->GetValueBoxed(instance));
                            break;
                        }
                    }
                }
            }
            else if (typeName.HasChars())
            {
                LOG(Error, "Missing type '{0}'", typeName);
                PrintStack(LogType::Error);
            }
            return;
        }
        const ScriptingType& type = typeHandle.GetType();
        if (structureValue.Type.Type != VariantType::Structure || StringUtils::Compare(typeNameAnsi.Get(), structureValue.Type.TypeName) != 0)
        {
            LOG(Error, "Cannot unpack value of type {0} to structure of type {1}", structureValue.Type, typeName);
            PrintStack(LogType::Error);
            return;
        }

        // Read structure field
        auto& layoutCache = node->Values[1];
        CHECK(layoutCache.Type.Type == VariantType::Blob);
        MemoryReadStream stream((byte*)layoutCache.AsBlob.Data, layoutCache.AsBlob.Length);
        const byte version = stream.ReadByte();
        if (version == 1)
        {
            int32 fieldsCount;
            stream.ReadInt32(&fieldsCount);
            for (int32 boxId = 1; boxId < node->Boxes.Count(); boxId++)
            {
                String fieldName;
                stream.ReadString(&fieldName, 11);
                VariantType fieldType;
                stream.ReadVariantType(&fieldType);
                if (box->ID == boxId)
                {
                    type.Struct.GetField(structureValue.AsBlob.Data, fieldName, value);
                    break;
                }
            }
        }
        break;
    }
    default:
        VisjectExecutor::ProcessGroupPacking(box, node, value);
        break;
    }
}

void VisualScriptExecutor::ProcessGroupParameters(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Get
    case 3:
    {
        int32 paramIndex;
        auto& stack = ThreadStacks.Get();
        if (!stack.Stack->Instance)
        {
            LOG(Error, "Cannot access Visual Script parameter without instance.");
            PrintStack(LogType::Error);
            break;
        }
        const auto param = stack.Stack->Script->Graph.GetParameter((Guid)node->Values[0], paramIndex);
        const auto instanceParams = stack.Stack->Script->_instances.Find(stack.Stack->Instance->GetID());
        if (param && instanceParams)
        {
            value = instanceParams->Value.Params[paramIndex];
        }
        else
        {
            LOG(Error, "Failed to access Visual Script parameter for {0}.", stack.Stack->Instance->ToString());
            PrintStack(LogType::Error);
        }
        break;
    }
        // Set
    case 4:
    {
        int32 paramIndex;
        auto& stack = ThreadStacks.Get();
        if (!stack.Stack->Instance)
        {
            LOG(Error, "Cannot access Visual Script parameter without instance.");
            PrintStack(LogType::Error);
            break;
        }
        const auto param = stack.Stack->Script->Graph.GetParameter((Guid)node->Values[0], paramIndex);
        const auto instanceParams = stack.Stack->Script->_instances.Find(stack.Stack->Instance->GetID());
        if (param && instanceParams)
        {
            instanceParams->Value.Params[paramIndex] = tryGetValue(node->GetBox(1), 1, Value::Zero);
        }
        else
        {
            LOG(Error, "Failed to access Visual Script parameter for {0}.", stack.Stack->Instance->ToString());
            PrintStack(LogType::Error);
        }
        if (node->Boxes[2].HasConnection())
            eatBox(node, node->Boxes[2].FirstConnection());
        break;
    }
    default:
        break;
    }
}

void VisualScriptExecutor::ProcessGroupTools(Box* box, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // This Instance
    case 19:
    {
        value = ThreadStacks.Get().Stack->Instance;
        break;
    }
        // As
    case 22:
    {
        value = Value::Null;
        const auto obj = (ScriptingObject*)tryGetValue(node->GetBox(1), Value::Null);
        if (obj)
        {
            const StringView typeName(node->Values[0]);
            const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
            const auto objClass = obj->GetClass();
            if (typeHandle && objClass && objClass->IsSubClassOf(typeHandle.GetType().ManagedClass))
                value = obj;
        }
        break;
    }
        // Type Reference node
    case 23:
    {
        const StringView typeName(node->Values[0]);
        if (box->ID == 0)
            value.SetTypename(typeName);
        else
            value = typeName;
        break;
    }
        // Is
    case 24:
    {
        value = Value::False;
        const auto obj = (ScriptingObject*)tryGetValue(node->GetBox(1), Value::Null);
        if (obj)
        {
            const StringView typeName(node->Values[0]);
            const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
            const auto objClass = obj->GetClass();
            value.AsBool = typeHandle && objClass && objClass->IsSubClassOf(typeHandle.GetType().ManagedClass);
        }
        break;
    }
        // Cast
    case 25:
    {
        if (box->ID == 0)
        {
            // Get object and try to cast it
            auto obj = (ScriptingObject*)tryGetValue(node->GetBox(1), Value::Null);
            if (obj)
            {
                const StringView typeName(node->Values[0]);
                const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
                const auto objClass = obj->GetClass();
                if (!typeHandle || !objClass || !objClass->IsSubClassOf(typeHandle.GetType().ManagedClass))
                    obj = nullptr;
            }

            // Cache cast object value (only if it's valid)
            const bool isValid = obj != nullptr;
            if (isValid)
            {
                const auto scope = ThreadStacks.Get().Stack->Scope;
                int32 returnedIndex = 0;
                for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
                {
                    const auto& e = scope->ReturnedValues[returnedIndex];
                    if (e.NodeId == node->ID && e.BoxId == 4)
                        break;
                }
                if (returnedIndex == scope->ReturnedValues.Count())
                    scope->ReturnedValues.AddOne();
                auto& returnedValue = scope->ReturnedValues[returnedIndex];
                returnedValue.NodeId = node->ID;
                returnedValue.BoxId = 4;
                returnedValue.Value = obj;
            }

            // Call graph further
            const auto impulseBox = &node->Boxes[isValid ? 2 : 3];
            if (impulseBox && impulseBox->HasConnection())
                eatBox(node, impulseBox->FirstConnection());
        }
        else if (box->ID == 4)
        {
            // Find returned value inside the current scope
            const auto scope = ThreadStacks.Get().Stack->Scope;
            int32 returnedIndex = 0;
            for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
            {
                const auto& e = scope->ReturnedValues[returnedIndex];
                if (e.NodeId == node->ID && e.BoxId == 4)
                    break;
            }
            if (returnedIndex != scope->ReturnedValues.Count())
                value = scope->ReturnedValues[returnedIndex].Value;
        }
        break;
    }
        // Cast Value
    case 26:
    {
        if (box->ID == 0)
        {
            // Get object and try to cast it
            auto obj = tryGetValue(node->GetBox(1), Value::Null);
            if (obj)
            {
                const StringView typeName(node->Values[0]);
                const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
                if (StringUtils::Compare(typeNameAnsi.Get(), obj.Type.GetTypeName()) != 0)
                {
                    MonoClass* klass = Scripting::FindClassNative(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
                    MonoClass* objKlass = MUtils::GetClass(obj);
                    if (!klass || !objKlass || mono_class_is_subclass_of(objKlass, klass, false) == 0)
                        obj = Value::Null;
                }
            }

            // Cache cast object value (only if it's valid)
            const bool isValid = obj != Value::Null;
            if (isValid)
            {
                const auto scope = ThreadStacks.Get().Stack->Scope;
                int32 returnedIndex = 0;
                for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
                {
                    const auto& e = scope->ReturnedValues[returnedIndex];
                    if (e.NodeId == node->ID && e.BoxId == 4)
                        break;
                }
                if (returnedIndex == scope->ReturnedValues.Count())
                    scope->ReturnedValues.AddOne();
                auto& returnedValue = scope->ReturnedValues[returnedIndex];
                returnedValue.NodeId = node->ID;
                returnedValue.BoxId = 4;
                returnedValue.Value = MoveTemp(obj);
            }

            // Call graph further
            const auto impulseBox = &node->Boxes[isValid ? 2 : 3];
            if (impulseBox && impulseBox->HasConnection())
                eatBox(node, impulseBox->FirstConnection());
        }
        else if (box->ID == 4)
        {
            // Find returned value inside the current scope
            const auto scope = ThreadStacks.Get().Stack->Scope;
            int32 returnedIndex = 0;
            for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
            {
                const auto& e = scope->ReturnedValues[returnedIndex];
                if (e.NodeId == node->ID && e.BoxId == 4)
                    break;
            }
            if (returnedIndex != scope->ReturnedValues.Count())
                value = scope->ReturnedValues[returnedIndex].Value;
        }
        break;
    }
        // Is Null
    case 27:
        value = tryGetValue(node->GetBox(1), Value::Null) == Value::Null;
        break;
        // Is Valid
    case 28:
        value = tryGetValue(node->GetBox(1), Value::Null) != Value::Null;
        break;
    default:
        VisjectExecutor::ProcessGroupTools(box, node, value);
        break;
    }
}

void VisualScriptExecutor::ProcessGroupFunction(Box* boxBase, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // Method Override
    case 3:
    {
        if (boxBase->ID == 0)
        {
            // Call graph further
            if (boxBase->HasConnection())
                eatBox(node, boxBase->FirstConnection());
        }
        else
        {
            // Evaluate overriden method parameter value from the current scope
            auto& scope = ThreadStacks.Get().Stack->Scope;
            value = scope->Parameters[boxBase->ID - 1];
        }
        break;
    }
        // Invoke Method
    case 4:
    {
        // Call Impulse or Pure Method
        if (boxBase->ID == 0 || (bool)node->Values[3])
        {
            auto& cache = node->Data.InvokeMethod;
            if (!cache.Method)
            {
                // Load method signature
                const auto typeName = (StringView)node->Values[0];
                const auto methodName = (StringView)node->Values[1];
                const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
                const StringAsANSI<60> methodNameAnsi(methodName.Get(), methodName.Length());
                ScriptingTypeMethodSignature signature;
                signature.Name = StringAnsiView(methodNameAnsi.Get(), methodName.Length());
                auto& signatureCache = node->Values[4];
                if (signatureCache.Type.Type != VariantType::Blob)
                {
                    LOG(Error, "Missing method '{0}::{1}' signature data", typeName, methodName);
                    PrintStack(LogType::Error);
                    return;
                }
                MemoryReadStream stream((byte*)signatureCache.AsBlob.Data, signatureCache.AsBlob.Length);
                const byte version = stream.ReadByte();
                if (version == 4)
                {
                    signature.IsStatic = stream.ReadBool();
                    stream.ReadVariantType(&signature.ReturnType);
                    int32 signatureParamsCount;
                    stream.ReadInt32(&signatureParamsCount);
                    signature.Params.Resize(signatureParamsCount);
                    for (int32 i = 0; i < signatureParamsCount; i++)
                    {
                        auto& param = signature.Params[i];
                        int32 parameterNameLength;
                        stream.ReadInt32(&parameterNameLength);
                        stream.SetPosition(stream.GetPosition() + parameterNameLength * sizeof(Char));
                        stream.ReadVariantType(&param.Type);
                        param.IsOut = stream.ReadBool();
                    }
                }
                else
                {
                    LOG(Error, "Unsupported method '{0}::{1}' signature data", typeName, methodName);
                    PrintStack(LogType::Error);
                    return;
                }
                void* method;
                ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
                if (typeHandle)
                {
                    // Find method in the scripting type
                    method = typeHandle.Module->FindMethod(typeHandle, signature);
                    if (!method)
                    {
                        LOG(Error, "Missing method '{0}::{1}'", typeName, methodName);
                        PrintStack(LogType::Error);
                        return;
                    }
                }
                else
                {
                    // Fallback to C#-only types
                    const auto mclass = Scripting::FindClass(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
                    if (mclass)
                    {
                        method = ManagedBinaryModule::FindMethod(mclass, signature);
                        if (!method)
                        {
                            LOG(Error, "Missing method '{0}::{1}'", typeName, methodName);
                            PrintStack(LogType::Error);
                            return;
                        }
                    }
                    else
                    {
                        if (typeName.HasChars())
                        {
                            LOG(Error, "Missing type '{0}'", typeName);
                            PrintStack(LogType::Error);
                        }
                        return;
                    }

                    // Mock the scripting type for C# method that doesn't exist in engine script types database
                    typeHandle = ScriptingTypeHandle(GetBinaryModuleFlaxEngine(), 0);
                }

                // Cache method data
                cache.Method = method;
                cache.Module = typeHandle.Module;
                cache.ParamsCount = signature.Params.Count();
                cache.IsStatic = signature.IsStatic;
                cache.OutParamsMask = 0;
                for (int32 paramIdx = 0; paramIdx < signature.Params.Count() && paramIdx < 32; paramIdx++)
                    cache.OutParamsMask |= signature.Params[paramIdx].IsOut ? (1 << static_cast<uint32>(paramIdx)) : 0;
            }

            // Evaluate object instance for non-static methods
            Variant instance;
            if (!cache.IsStatic)
            {
                // Evaluate object instance
                const auto box = node->GetBox(1);
                if (box->HasConnection())
                {
                    instance = eatBox(node, box->FirstConnection());
                }
                else
                {
                    // Unconnected instance box is treated as this script instance if type matches the member method class
                    auto& stack = ThreadStacks.Get();
                    instance.SetObject(stack.Stack->Instance);
                }
            }

            // Evaluate parameter values
            Variant* paramValues = (Variant*)alloca(cache.ParamsCount * sizeof(Variant));
            bool hasOutParams = false;
            for (int32 paramIdx = 0; paramIdx < cache.ParamsCount; paramIdx++)
            {
                auto& paramValue = paramValues[paramIdx];
                Memory::ConstructItem(&paramValue);
                const bool isOut = paramIdx < 32 && (cache.OutParamsMask & (1 << static_cast<uint32>(paramIdx))) != 0;
                hasOutParams |= isOut;
                const auto box = node->GetBox(paramIdx + 4);
                if (box->HasConnection() && !isOut)
                    paramValue = eatBox(node, box->FirstConnection());
                else if (node->Values.Count() > 5 + paramIdx)
                    paramValue = node->Values[5 + paramIdx];
            }

            // Invoke the method
            Variant result;
            if (cache.Module->InvokeMethod(cache.Method, instance, Span<Variant>(paramValues, cache.ParamsCount), result))
            {
                PrintStack(LogType::Error);
            }
            else
            {
                // Cache returned value inside the current scope
                const auto scope = ThreadStacks.Get().Stack->Scope;
                {
                    int32 returnedIndex = 0;
                    for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
                    {
                        const auto& e = scope->ReturnedValues[returnedIndex];
                        if (e.NodeId == node->ID && e.BoxId == 3)
                            break;
                    }
                    if (returnedIndex == scope->ReturnedValues.Count())
                        scope->ReturnedValues.AddOne();
                    auto& returnedValue = scope->ReturnedValues[returnedIndex];
                    returnedValue.NodeId = node->ID;
                    returnedValue.BoxId = 3;
                    returnedValue.Value = MoveTemp(result);
                }

                // Cache output parameters values inside the current scope
                if (hasOutParams)
                {
                    for (int32 paramIdx = 0; paramIdx < cache.ParamsCount; paramIdx++)
                    {
                        const bool isOut = paramIdx < 32 && (cache.OutParamsMask & (1 << static_cast<uint32>(paramIdx))) != 0;
                        if (isOut && node->GetBox(paramIdx + 4)->HasConnection())
                        {
                            int32 returnedIndex = 0;
                            for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
                            {
                                const auto& e = scope->ReturnedValues[returnedIndex];
                                if (e.NodeId == node->ID && e.BoxId == paramIdx + 4)
                                    break;
                            }
                            if (returnedIndex == scope->ReturnedValues.Count())
                                scope->ReturnedValues.AddOne();
                            auto& returnedValue = scope->ReturnedValues[returnedIndex];
                            returnedValue.NodeId = node->ID;
                            returnedValue.BoxId = paramIdx + 4;
                            returnedValue.Value = MoveTemp(paramValues[paramIdx]);
                        }
                    }
                }

                // Call graph further
                const auto returnedImpulse = &node->Boxes[2];
                if (returnedImpulse && returnedImpulse->HasConnection())
                    eatBox(node, returnedImpulse->FirstConnection());
            }

            // Free parameters data
            Memory::DestructItems(paramValues, cache.ParamsCount);
        }
        // Returned value or Output Parameter
        if (boxBase->ID == 3 || boxBase->ID >= 4)
        {
            // Find returned value inside the current scope (from the previous method call)
            const auto scope = ThreadStacks.Get().Stack->Scope;
            int32 returnedIndex = 0;
            for (; returnedIndex < scope->ReturnedValues.Count(); returnedIndex++)
            {
                const auto& e = scope->ReturnedValues[returnedIndex];
                if (e.NodeId == node->ID && e.BoxId == boxBase->ID)
                    break;
            }
            if (returnedIndex != scope->ReturnedValues.Count())
                value = scope->ReturnedValues[returnedIndex].Value;
        }
        break;
    }
        // Return
    case 5:
    {
        auto& scope = ThreadStacks.Get().Stack->Scope;
        scope->FunctionReturn = tryGetValue(node->GetBox(1), Value::Zero);
        break;
    }
        // Function
    case 6:
    {
        if (boxBase->ID == 0)
        {
            // Call function
            if (boxBase->HasConnection())
                eatBox(node, boxBase->FirstConnection());
        }
        else
        {
            // Evaluate method parameter value from the current scope
            auto& scope = ThreadStacks.Get().Stack->Scope;
            value = scope->Parameters[boxBase->ID - 1];
        }
        break;
    }
        // Get Field
    case 7:
    {
        auto& cache = node->Data.GetSetField;
        if (!cache.Field)
        {
            const auto typeName = (StringView)node->Values[0];
            const auto fieldName = (StringView)node->Values[1];
            const auto fieldTypeName = (StringView)node->Values[2];
            const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
            const StringAsANSI<60> fieldNameAnsi(fieldName.Get(), fieldName.Length());
            const StringAsANSI<100> fieldTypeNameAnsi(fieldTypeName.Get(), fieldTypeName.Length());
            void* field;
            ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
            if (typeHandle)
            {
                // Find field in the scripting type
                field = typeHandle.Module->FindField(typeHandle, fieldNameAnsi.Get());
                if (!field)
                {
                    LOG(Error, "Missing field '{1}' in type '{0}'", typeName, fieldName);
                    PrintStack(LogType::Error);
                    return;
                }
            }
            else
            {
                // Fallback to C#-only types
                const auto mclass = Scripting::FindClass(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
                if (mclass)
                {
                    field = mclass->GetField(fieldNameAnsi.Get());
                    if (!field)
                    {
                        LOG(Error, "Missing field '{1}' in type '{0}'", typeName, fieldName);
                        PrintStack(LogType::Error);
                        return;
                    }
                }
                else
                {
                    if (typeName.HasChars())
                    {
                        LOG(Error, "Missing type '{0}'", typeName);
                        PrintStack(LogType::Error);
                    }
                    return;
                }

                // Mock the scripting type for C# field that doesn't exist in engine script types database
                typeHandle = ScriptingTypeHandle(GetBinaryModuleFlaxEngine(), 0);
            }

            // Cache field data
            cache.Field = field;
            cache.Module = typeHandle.Module;
            ScriptingTypeFieldSignature signature;
            cache.Module->GetFieldSignature(field, signature);
            cache.IsStatic = signature.IsStatic;
        }

        // Evaluate object instance for non-static fields
        Variant instance;
        if (!cache.IsStatic)
        {
            // Evaluate object instance
            const auto box = node->GetBox(1);
            if (box->HasConnection())
            {
                instance = eatBox(node, box->FirstConnection());
            }
            else
            {
                // Unconnected instance box is treated as this script instance if type matches the member field class
                auto& stack = ThreadStacks.Get();
                instance.SetObject(stack.Stack->Instance);
            }
        }

        // Get field value
        if (cache.Module->GetFieldValue(cache.Field, instance, value))
        {
            PrintStack(LogType::Error);
        }
        break;
    }
        // Get Field
    case 8:
    {
        auto& cache = node->Data.GetSetField;
        if (!cache.Field)
        {
            const auto typeName = (StringView)node->Values[0];
            const auto fieldName = (StringView)node->Values[1];
            const auto fieldTypeName = (StringView)node->Values[2];
            const StringAsANSI<100> typeNameAnsi(typeName.Get(), typeName.Length());
            const StringAsANSI<60> fieldNameAnsi(fieldName.Get(), fieldName.Length());
            const StringAsANSI<100> fieldTypeNameAnsi(fieldTypeName.Get(), fieldTypeName.Length());
            void* field;
            ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
            if (typeHandle)
            {
                // Find field in the scripting type
                field = typeHandle.Module->FindField(typeHandle, fieldNameAnsi.Get());
                if (!field)
                {
                    LOG(Error, "Missing field '{1}' in type '{0}'", typeName, fieldName);
                    PrintStack(LogType::Error);
                    return;
                }
            }
            else
            {
                // Fallback to C#-only types
                const auto mclass = Scripting::FindClass(StringAnsiView(typeNameAnsi.Get(), typeName.Length()));
                if (mclass)
                {
                    field = mclass->GetField(fieldNameAnsi.Get());
                    if (!field)
                    {
                        LOG(Error, "Missing field '{1}' in type '{0}'", typeName, fieldName);
                        PrintStack(LogType::Error);
                        return;
                    }
                }
                else
                {
                    if (typeName.HasChars())
                    {
                        LOG(Error, "Missing type '{0}'", typeName);
                        PrintStack(LogType::Error);
                    }
                    return;
                }

                // Mock the scripting type for C# field that doesn't exist in engine script types database
                typeHandle = ScriptingTypeHandle(GetBinaryModuleFlaxEngine(), 0);
            }

            // Cache field data
            cache.Field = field;
            cache.Module = typeHandle.Module;
            ScriptingTypeFieldSignature signature;
            cache.Module->GetFieldSignature(field, signature);
            cache.IsStatic = signature.IsStatic;
        }

        // Evaluate object instance for non-static fields
        Variant instance;
        if (!cache.IsStatic)
        {
            // Evaluate object instance
            const auto box = node->GetBox(1);
            if (box->HasConnection())
            {
                instance = eatBox(node, box->FirstConnection());
            }
            else
            {
                // Unconnected instance box is treated as this script instance if type matches the member field class
                auto& stack = ThreadStacks.Get();
                instance.SetObject(stack.Stack->Instance);
            }
        }

        // Set field value
        value = tryGetValue(node->GetBox(0), 4, Value::Zero);
        if (cache.Module->SetFieldValue(cache.Field, instance, value))
        {
            PrintStack(LogType::Error);
            break;
        }

        // Call graph further
        const auto returnedImpulse = &node->Boxes[3];
        if (returnedImpulse && returnedImpulse->HasConnection())
            eatBox(node, returnedImpulse->FirstConnection());
        break;
    }
        // Bind/Unbind
    case 9:
    case 10:
    {
        const bool bind = node->TypeID == 9;
        auto& stack = ThreadStacks.Get();
        if (!stack.Stack->Instance)
        {
            // TODO: add support for binding to events in static Visual Script
            LOG(Error, "Cannot bind to event in static Visual Script.");
            PrintStack(LogType::Error);
            break;
        }
        const auto object = stack.Stack->Instance;

        // Find method to bind
        VisualScriptGraphNode* methodNode = nullptr;
        const auto graph = stack.Stack && stack.Stack->Script ? &stack.Stack->Script->Graph : nullptr;
        if (graph)
            methodNode = graph->GetNode((uint32)node->Values[2]);
        if (!methodNode)
        {
            LOG(Error, "Missing function handler to bind to the event.");
            PrintStack(LogType::Error);
            break;
        }
        VisualScript::Method* method = nullptr;
        for (auto& m : stack.Stack->Script->_methods)
        {
            if (m.Node == methodNode)
            {
                method = &m;
                break;
            }
        }
        if (!method)
        {
            LOG(Error, "Missing method to bind to the event.");
            PrintStack(LogType::Error);
            break;
        }

        // Find event
        const StringView eventTypeName(node->Values[0]);
        const StringView eventName(node->Values[1]);
        const StringAsANSI<100> eventTypeNameAnsi(eventTypeName.Get(), eventTypeName.Length());
        const ScriptingTypeHandle eventType = Scripting::FindScriptingType(StringAnsiView(eventTypeNameAnsi.Get(), eventTypeName.Length()));

        // Find event binding callback
        auto eventBinder = ScriptingEvents::EventsTable.TryGet(Pair<ScriptingTypeHandle, StringView>(eventType, eventName));
        if (!eventBinder)
        {
            LOG(Error, "Cannot bind to missing event {0} from type {1}.", eventName, eventTypeName);
            PrintStack(LogType::Error);
            break;
        }

        // Evaluate object instance
        const auto box = node->GetBox(1);
        Variant instance;
        if (box->HasConnection())
            instance = eatBox(node, box->FirstConnection());
        else
            instance.SetObject(object);
        if (!instance.AsObject)
        {
            LOG(Error, "Cannot bind event to null object.");
            PrintStack(LogType::Error);
            break;
        }
        // TODO: check if instance is of event type (including inheritance)

        // Add Visual Script method to the event bindings table
        const auto& type = object->GetType();
        Guid id;
        if (Guid::Parse(type.Fullname, id))
            break;
        if (const auto visualScript = (VisualScript*)Content::GetAsset(id))
        {
            if (auto i = visualScript->GetScriptInstance(object))
            {
                VisualScript::EventBinding* eventBinding = nullptr;
                for (auto& b : i->EventBindings)
                {
                    if (b.Type == eventType && b.Name == eventName)
                    {
                        eventBinding = &b;
                        break;
                    }
                }
                if (bind)
                {
                    // Bind to the event
                    if (!eventBinding)
                    {
                        eventBinding = &i->EventBindings.AddOne();
                        eventBinding->Type = eventType;
                        eventBinding->Name = eventName;
                    }
                    eventBinding->BindedMethods.Add(method);
                    if (eventBinding->BindedMethods.Count() == 1)
                        (*eventBinder)(instance.AsObject, object, true);
                }
                else if (eventBinding)
                {
                    // Unbind from the event
                    if (eventBinding->BindedMethods.Count() == 1)
                        (*eventBinder)(instance.AsObject, object, false);
                    eventBinding->BindedMethods.Remove(method);
                }
            }
        }

        // Call graph further
        const auto returnedImpulse = &node->Boxes[2];
        if (returnedImpulse && returnedImpulse->HasConnection())
            eatBox(node, returnedImpulse->FirstConnection());
        break;
    }
    default:
        break;
    }
}

void VisualScriptExecutor::ProcessGroupFlow(Box* boxBase, Node* node, Value& value)
{
    switch (node->TypeID)
    {
        // If
    case 1:
    {
        const bool condition = (bool)tryGetValue(node->GetBox(1), Value::Zero);
        boxBase = node->GetBox(condition ? 2 : 3);
        if (boxBase->HasConnection())
            eatBox(node, boxBase->FirstConnection());
        break;
    }
        // For Loop
    case 2:
    {
        const auto scope = ThreadStacks.Get().Stack->Scope;
        int32 iteratorIndex = 0;
        for (; iteratorIndex < scope->ReturnedValues.Count(); iteratorIndex++)
        {
            const auto& e = scope->ReturnedValues[iteratorIndex];
            if (e.NodeId == node->ID)
                break;
        }
        switch (boxBase->ID)
        {
            // Loop
        case 0:
        {
            if (iteratorIndex == scope->ReturnedValues.Count())
                scope->ReturnedValues.AddOne();
            auto& iteratorValue = scope->ReturnedValues[iteratorIndex];
            iteratorValue.NodeId = node->ID;
            iteratorValue.BoxId = 0;
            iteratorValue.Value = (int32)tryGetValue(node->GetBox(1), 0, Value::Zero);
            const int32 count = (int32)tryGetValue(node->GetBox(2), 1, Value::Zero);
            for (; iteratorValue.Value.AsInt < count; iteratorValue.Value.AsInt++)
            {
                boxBase = node->GetBox(4);
                if (boxBase->HasConnection())
                    eatBox(node, boxBase->FirstConnection());
            }
            boxBase = node->GetBox(6);
            if (boxBase->HasConnection())
                eatBox(node, boxBase->FirstConnection());
            break;
        }
            // Break
        case 3:
            // Reset loop iterator
            if (iteratorIndex != scope->ReturnedValues.Count())
                scope->ReturnedValues[iteratorIndex].Value.AsInt = MAX_int32 - 1;
            break;
            // Index
        case 5:
            if (iteratorIndex != scope->ReturnedValues.Count())
                value = scope->ReturnedValues[iteratorIndex].Value;
            break;
        }
        break;
    }
        // While Loop
    case 3:
    {
        const auto scope = ThreadStacks.Get().Stack->Scope;
        int32 iteratorIndex = 0;
        for (; iteratorIndex < scope->ReturnedValues.Count(); iteratorIndex++)
        {
            const auto& e = scope->ReturnedValues[iteratorIndex];
            if (e.NodeId == node->ID)
                break;
        }
        switch (boxBase->ID)
        {
            // Loop
        case 0:
        {
            if (iteratorIndex == scope->ReturnedValues.Count())
                scope->ReturnedValues.AddOne();
            auto& iteratorValue = scope->ReturnedValues[iteratorIndex];
            iteratorValue.NodeId = node->ID;
            iteratorValue.BoxId = 0;
            iteratorValue.Value = 0;
            for (; (bool)tryGetValue(node->GetBox(1), 1, Value::Zero) && iteratorValue.Value.AsInt != -1; iteratorValue.Value.AsInt++)
            {
                boxBase = node->GetBox(3);
                if (boxBase->HasConnection())
                    eatBox(node, boxBase->FirstConnection());
            }
            boxBase = node->GetBox(5);
            if (boxBase->HasConnection())
                eatBox(node, boxBase->FirstConnection());
            break;
        }
            // Break
        case 2:
            // Reset loop iterator
            if (iteratorIndex != scope->ReturnedValues.Count())
                scope->ReturnedValues[iteratorIndex].Value.AsInt = -1;
            break;
            // Index
        case 4:
            if (iteratorIndex != scope->ReturnedValues.Count())
                value = scope->ReturnedValues[iteratorIndex].Value;
            break;
        }
        break;
    }
        // Sequence
    case 4:
    {
        const int32 count = (int32)node->Values[0];
        for (int32 i = 0; i < count; i++)
        {
            boxBase = node->GetBox(i + 1);
            if (boxBase->HasConnection())
                eatBox(node, boxBase->FirstConnection());
        }
        break;
    }
        // Branch On Enum
    case 5:
    {
        const Value v = tryGetValue(node->GetBox(1), Value::Null);
        if (v.Type.Type == VariantType::Enum && node->Values.Count() == 1 && node->Values[0].Type.Type == VariantType::Blob)
        {
            int32* dataValues = (int32*)node->Values[0].AsBlob.Data;
            int32 dataValuesCount = node->Values[0].AsBlob.Length / 4;
            int32 vAsInt = (int32)v;
            for (int32 i = 0; i < dataValuesCount; i++)
            {
                if (dataValues[i] == vAsInt)
                {
                    boxBase = node->GetBox(i + 2);
                    if (boxBase->HasConnection())
                        eatBox(node, boxBase->FirstConnection());
                    break;
                }
            }
        }
        break;
    }
        // Delay
    case 6:
    {
        boxBase = node->GetBox(2);
        if (!boxBase->HasConnection())
            break;
        const float duration = (float)tryGetValue(node->GetBox(1), node->Values[0]);
        if (duration > ZeroTolerance)
        {
            class DelayTask : public MainThreadTask
            {
            public:
                Guid Script;
                Guid Instance;
                int32 Node;
                int32 Box;

            protected:
                bool Run() override
                {
                    Variant result;
                    VisualScriptingExecutor.Invoke(Script, Node, Box, Instance, result);
                    return false;
                }
            };
            const auto& stack = ThreadStacks.Get().Stack;
            auto task = New<DelayTask>();
            task->Script = stack->Script->GetID();;
            task->Instance = stack->Instance->GetID();;
            task->Node = ((Node*)boxBase->FirstConnection()->Parent)->ID;
            task->Box = boxBase->FirstConnection()->ID;
            task->InitialDelay = duration;
            task->Start();
        }
        else
        {
            eatBox(node, boxBase->FirstConnection());
        }
        break;
    }
    }
}

REGISTER_BINARY_ASSET(VisualScript, "FlaxEngine.VisualScript", false);

VisualScript::VisualScript(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Asset::LoadResult VisualScript::load()
{
    // Build Visual Script typename that is based on asset id
    String typeName = _id.ToString();
    StringUtils::ConvertUTF162ANSI(typeName.Get(), _typenameChars, 32);
    _typenameChars[32] = 0;
    _typename = StringAnsiView(_typenameChars, 32);

    // Load metadata
    const auto metadataChunk = GetChunk(1);
    if (metadataChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream metadataStream(metadataChunk->Get(), metadataChunk->Size());
    int32 version;
    metadataStream.ReadInt32(&version);
    switch (version)
    {
    case 1:
    {
        metadataStream.ReadString(&Meta.BaseTypename, 31);
        metadataStream.ReadInt32((int32*)&Meta.Flags);
        break;
    }
    default:
        LOG(Error, "Unknown Visual Script \'{1}\' metadata version {0}.", version, ToString());
        return LoadResult::InvalidData;
    }

    // Load graph
    const auto surfaceChunk = GetChunk(0);
    if (surfaceChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream surfaceStream(surfaceChunk->Get(), surfaceChunk->Size());
    if (Graph.Load(&surfaceStream, true))
    {
        LOG(Warning, "Failed to load graph \'{0}\'", ToString());
        return LoadResult::Failed;
    }

    // Find method nodes
    for (auto& node : Graph.Nodes)
    {
        switch (node.Type)
        {
        case GRAPH_NODE_MAKE_TYPE(16, 3):
        {
            // Override method
            auto& method = _methods.AddOne();
            method.Script = this;
            method.Node = &node;
            method.Name = StringAnsi((StringView)node.Values[0]);
            method.MethodFlags = (MethodFlags)((byte)MethodFlags::Virtual | (byte)MethodFlags::Override);
            method.Signature.Name = method.Name;
            method.Signature.IsStatic = false;
            method.Signature.Params.Resize(node.Values[1].AsInt);
            method.ParamNames.Resize(method.Signature.Params.Count());
            break;
        }
        case GRAPH_NODE_MAKE_TYPE(16, 6):
        {
            // Function
            auto& method = _methods.AddOne();
            method.Script = this;
            method.Node = &node;
            method.Signature.IsStatic = false;
            auto& signatureData = node.Values[0];
            if (signatureData.Type.Type != VariantType::Blob || signatureData.AsBlob.Length == 0)
                break;
            MemoryReadStream signatureStream((byte*)signatureData.AsBlob.Data, signatureData.AsBlob.Length);
            switch (signatureStream.ReadByte())
            {
            case 1:
            {
                signatureStream.ReadStringAnsi(&method.Name, 71);
                method.MethodFlags = (MethodFlags)signatureStream.ReadByte();
                method.Signature.IsStatic = ((byte)method.MethodFlags & (byte)MethodFlags::Static) != 0;
                signatureStream.ReadVariantType(&method.Signature.ReturnType);
                int32 parametersCount;
                signatureStream.ReadInt32(&parametersCount);
                method.Signature.Params.Resize(parametersCount);
                method.ParamNames.Resize(parametersCount);
                for (int32 i = 0; i < parametersCount; i++)
                {
                    auto& param = method.Signature.Params[i];
                    signatureStream.ReadStringAnsi(&method.ParamNames[i], 13);
                    signatureStream.ReadVariantType(&param.Type);
                    param.IsOut = signatureStream.ReadByte() != 0;
                    bool hasDefaultValue = signatureStream.ReadByte() != 0;
                }
                break;
            }
            default:
                break;
            }
            method.Signature.Name = method.Name;
            break;
        }
        }
    }
#if COMPILE_WITH_PROFILER
    for (auto& method : _methods)
    {
        const StringView assetName(StringUtils::GetFileNameWithoutExtension(GetPath()));
        method.ProfilerName.Resize(assetName.Length() + 2 + method.Name.Length());
        StringUtils::ConvertUTF162ANSI(assetName.Get(), method.ProfilerName.Get(), assetName.Length());
        method.ProfilerName.Get()[assetName.Length()] = ':';
        method.ProfilerName.Get()[assetName.Length() + 1] = ':';
        Platform::MemoryCopy(method.ProfilerName.Get() + assetName.Length() + 2, method.Name.Get(), method.Name.Length());
    }
#endif

    // Setup fields list
    _fields.Resize(Graph.Parameters.Count());
    for (int32 i = 0; i < Graph.Parameters.Count(); i++)
    {
        auto& parameter = Graph.Parameters[i];
        auto& field = _fields[i];
        field.Script = this;
        field.Parameter = &parameter;
        field.Index = i;
        field.Name.Set(parameter.Name.Get(), parameter.Name.Length());
    }

#if USE_EDITOR
    if (_instances.HasItems())
    {
        // Setup scripting type
        CacheScriptingType();

        // Create default instance object to with valid new vtable
        ScriptingObject* defaultInstance = _scriptingTypeHandle.GetType().GetDefaultInstance();

        // Reinitialize existing Visual Script instances in the Editor
        for (auto& e : _instances)
        {
            ScriptingObject* object = Scripting::TryFindObject<ScriptingObject>(e.Key);
            if (!object)
                continue;

            // Hack vtable similarly to VisualScriptObjectSpawn
            ScriptingType& visualScriptType = (ScriptingType&)object->GetType();
            if (visualScriptType.Script.ScriptVTable)
            {
                // Override object vtable with hacked one that has Visual Script functions calls
                ASSERT(visualScriptType.Script.VTable);
                *(void**)object = visualScriptType.Script.VTable;
            }
        }
        const int32 oldCount = _oldParamsLayout.Count();
        const int32 count = Graph.Parameters.Count();
        if (oldCount != 0 && count != 0)
        {
            // Update instanced data from previous format to the current graph parameters scheme
            for (auto& e : _instances)
            {
                auto& instanceParams = e.Value.Params;
                Array<Variant> valuesCache(MoveTemp(instanceParams));
                instanceParams.Resize(count);
                for (int32 i = 0; i < count; i++)
                {
                    const int32 oldIndex = _oldParamsLayout.Find(Graph.Parameters[i].Identifier);
                    instanceParams[i] = oldIndex != -1 ? valuesCache[oldIndex] : Graph.Parameters[i].Value;
                }
            }
        }
        else
        {
            // Reset instances values to defaults
            for (auto& e : _instances)
            {
                auto& instanceParams = e.Value.Params;
                instanceParams.Resize(count);
                for (int32 i = 0; i < count; i++)
                    instanceParams[i] = Graph.Parameters[i].Value;
            }
        }
    }
#endif

    return LoadResult::Ok;
}

void VisualScript::unload(bool isReloading)
{
#if USE_EDITOR
    if (isReloading)
    {
        // Cache existing instanced parameters IDs to restore values after asset reload (params order might be changed but the IDs are stable)
        _oldParamsLayout.Resize(Graph.Parameters.Count());
        for (int32 i = 0; i < Graph.Parameters.Count(); i++)
        {
            auto& param = Graph.Parameters[i];
            _oldParamsLayout[i] = param.Identifier;
        }
    }
    else
    {
        _oldParamsLayout.Clear();
    }
#else
    _instances.Clear();
#endif

    // Clear resources
    _methods.Clear();
    _fields.Clear();
    Graph.Clear();

    // Note: preserve the registered scripting type but invalidate the locally cached handle
    if (_scriptingTypeHandle)
    {
        auto& type = VisualScriptingModule.Types[_scriptingTypeHandle.TypeIndex];
        if (type.Script.DefaultInstance)
        {
            Delete(type.Script.DefaultInstance);
            type.Script.DefaultInstance = nullptr;
        }
        VisualScriptingModule.TypeNameToTypeIndex.RemoveValue(_scriptingTypeHandle.TypeIndex);
        VisualScriptingModule.Scripts[_scriptingTypeHandle.TypeIndex] = nullptr;
        _scriptingTypeHandleCached = _scriptingTypeHandle;
        _scriptingTypeHandle = ScriptingTypeHandle();
    }
}

AssetChunksFlag VisualScript::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0) | GET_CHUNK_FLAG(1);
}

void VisualScript::CacheScriptingType()
{
    auto& binaryModule = VisualScriptingModule;

    // Find base type
    const StringAnsi baseTypename(Meta.BaseTypename);
    const ScriptingTypeHandle baseType = Scripting::FindScriptingType(baseTypename);
    if (baseType)
    {
        // Find first native base C++ class of this Visual Script class
        ScriptingTypeHandle nativeType = baseType;
        while (nativeType && nativeType.GetType().Script.ScriptVTable)
        {
            nativeType = nativeType.GetType().GetBaseType();
        }
        if (!nativeType)
        {
            LOG(Error, "Missing native base class for {0}", ToString());
            return;
        }

        // Create scripting type
        if (_scriptingTypeHandleCached)
        {
            // Reuse cached slot (already created objects with that type handle can use the new type info eg. after asset reload when editing script in editor)
            ASSERT(_scriptingTypeHandleCached.GetType().Fullname == _typename);
            _scriptingTypeHandle = _scriptingTypeHandleCached;
            _scriptingTypeHandleCached = ScriptingTypeHandle();
            auto& type = VisualScriptingModule.Types[_scriptingTypeHandle.TypeIndex];
            type.~ScriptingType();
            new(&type)ScriptingType(_typename, &binaryModule, baseType.GetType().Size, ScriptingType::DefaultInitRuntime, VisualScriptingBinaryModule::VisualScriptObjectSpawn, baseType);
            binaryModule.Scripts[_scriptingTypeHandle.TypeIndex] = this;
        }
        else
        {
            // Allocate new slot
            const int32 typeIndex = binaryModule.Types.Count();
            binaryModule.Types.AddUninitialized();
            new(binaryModule.Types.Get() + binaryModule.Types.Count() - 1)ScriptingType(_typename, &binaryModule, baseType.GetType().Size, ScriptingType::DefaultInitRuntime, VisualScriptingBinaryModule::VisualScriptObjectSpawn, baseType);
            binaryModule.TypeNameToTypeIndex[_typename] = typeIndex;
            _scriptingTypeHandle = ScriptingTypeHandle(&binaryModule, typeIndex);
            binaryModule.Scripts.Add(this);

            // Special initialization when the first Visual Script gets loaded
            if (typeIndex == 0)
            {
#if USE_EDITOR
                // Register for other modules unload to clear runtime execution cache
                Scripting::ScriptsReloading.Bind<VisualScriptingBinaryModule, &VisualScriptingBinaryModule::OnScriptsReloading>(&binaryModule);
#endif

                // Register for scripting events
                ScriptingEvents::Event.Bind(VisualScriptingBinaryModule::OnEvent);
            }
        }
        auto& type = _scriptingTypeHandle.Module->Types[_scriptingTypeHandle.TypeIndex];
        type.ManagedClass = baseType.GetType().ManagedClass;

        // Create custom vtable for this class (build out of the wrapper C++ methods that call Visual Script graph)
        // Call setup for all class starting from the first native type (first that uses virtual calls will allocate table of a proper size, further base types will just add own methods)
        for (ScriptingTypeHandle e = nativeType; e;)
        {
            const ScriptingType& eType = e.GetType();
            if (eType.Script.SetupScriptVTable)
            {
                ASSERT(eType.ManagedClass);
                eType.Script.SetupScriptVTable(eType.ManagedClass, type.Script.ScriptVTable, type.Script.ScriptVTableBase);
            }
            e = eType.GetBaseType();
        }
        MMethod** scriptVTable = (MMethod**)type.Script.ScriptVTable;
        while (scriptVTable && *scriptVTable)
        {
            const MMethod* referenceMethod = *scriptVTable;

            // Find that method overriden in Visual Script (the current or one of the base classes in Visual Script)
            auto node = FindMethod(referenceMethod->GetName(), referenceMethod->GetParametersCount());
            if (node == nullptr)
            {
                // Check base classes that are Visual Script
                auto e = baseType;
                while (e.Module == &binaryModule && node == nullptr)
                {
                    auto& eType = e.GetType();
                    Guid id;
                    if (!Guid::Parse(eType.Fullname, id))
                    {
                        if (const auto visualScript = Content::LoadAsync<VisualScript>(id))
                        {
                            node = visualScript->FindMethod(referenceMethod->GetName(), referenceMethod->GetParametersCount());
                        }
                    }
                    e = e.GetType().GetBaseType();
                }
            }

            // Set the method to call (null entry marks unused entries that won't use Visual Script wrapper calls)
            *scriptVTable = (MMethod*)node;

            // Move to the next entry (table is null terminated)
            scriptVTable++;
        }
    }
    else if (Meta.BaseTypename.HasChars())
    {
        LOG(Error, "Failed to find a scripting type \'{0}\' that is a base type for {1}", Meta.BaseTypename, ToString());
    }
    else
    {
        LOG(Error, "Cannot use {0} as script because base typename is missing.", ToString());
    }
}

VisualScriptingBinaryModule::VisualScriptingBinaryModule()
    : _name("Visual Scripting")
{
}

ScriptingObject* VisualScriptingBinaryModule::VisualScriptObjectSpawn(const ScriptingObjectSpawnParams& params)
{
    // Create native object (base type can be C++ or C#)
    ScriptingType& visualScriptType = (ScriptingType&)params.Type.GetType();
    ScriptingTypeHandle baseTypeHandle = visualScriptType.GetBaseType();
    const ScriptingType* baseTypePtr = &baseTypeHandle.GetType();
    while (baseTypePtr->Script.Spawn == &VisualScriptObjectSpawn)
    {
        baseTypeHandle = baseTypePtr->GetBaseType();
        baseTypePtr = &baseTypeHandle.GetType();
    }
    ScriptingObject* object = baseTypePtr->Script.Spawn(params);
    if (!object)
    {
        return nullptr;
    }

    // Beware! Hacking vtables incoming! Undefined behaviors exploits! Low-level programming!
    // What's happening here?
    // We create a custom vtable for the Visual Script objects that use a native class object with virtual functions overrides.
    // To make it easy to use in C++ we inject custom wrapper methods into C++ object vtable to execute Visual Script graph from them.
    // Because virtual member functions calls are C++ ABI and impl-defined this is quite hard. But works.
    if (visualScriptType.Script.ScriptVTable)
    {
        if (!visualScriptType.Script.VTable)
        {
            // Duplicate vtable
            void** vtable = *(void***)object;
            const int32 prefixSize = GetVTablePrefix();
            int32 entriesCount = 0;
            while (vtable[entriesCount] && entriesCount < 200)
                entriesCount++;
            const int32 size = entriesCount * sizeof(void*);
            visualScriptType.Script.VTable = (void**)((byte*)Platform::Allocate(prefixSize + size, 16) + prefixSize);
            Platform::MemoryCopy((byte*)visualScriptType.Script.VTable - prefixSize, (byte*)vtable - prefixSize, prefixSize + size);

            // Override vtable entries by the class
            for (ScriptingTypeHandle e = baseTypeHandle; e;)
            {
                const ScriptingType& eType = e.GetType();
                if (eType.Script.SetupScriptObjectVTable)
                    eType.Script.SetupScriptObjectVTable(visualScriptType.Script.ScriptVTable, visualScriptType.Script.ScriptVTableBase, visualScriptType.Script.VTable, entriesCount, 1);
                e = eType.GetBaseType();
            }
        }

        // Override object vtable with hacked one that has Visual Script functions calls
        *(void**)object = visualScriptType.Script.VTable;
    }

    // Mark as custom scripting type
    object->Flags |= ObjectFlags::IsCustomScriptingType;

    // Get Visual Script asset
    ASSERT(&VisualScriptingModule == params.Type.Module);
    VisualScript* visualScript = VisualScriptingModule.Scripts[params.Type.TypeIndex];

    // Initialize instance data
    auto& instanceParams = visualScript->_instances[object->GetID()].Params;
    instanceParams.Resize(visualScript->Graph.Parameters.Count());
    for (int32 i = 0; i < instanceParams.Count(); i++)
        instanceParams[i] = visualScript->Graph.Parameters[i].Value;

    return object;
}

#if USE_EDITOR

void VisualScriptingBinaryModule::OnScriptsReloading()
{
    // Clear any cached types from that module across all loaded Visual Scripts
    for (auto& script : Scripts)
    {
        if (!script || !script->IsLoaded())
            continue;
        ScopeLock lock(script->Locker);

        // Clear cached types (underlying base class could be in reloaded C# scripts)
        if (script->_scriptingTypeHandle)
        {
            auto& type = VisualScriptingModule.Types[script->_scriptingTypeHandle.TypeIndex];
            if (type.Script.DefaultInstance)
            {
                Delete(type.Script.DefaultInstance);
                type.Script.DefaultInstance = nullptr;
            }
            VisualScriptingModule.TypeNameToTypeIndex.RemoveValue(script->_scriptingTypeHandle.TypeIndex);
            script->_scriptingTypeHandleCached = script->_scriptingTypeHandle;
            script->_scriptingTypeHandle = ScriptingTypeHandle();
        }

        // Clear methods cache
        for (auto& node : script->Graph.Nodes)
        {
            switch (node.Type)
            {
                // Invoke Method
            case GRAPH_NODE_MAKE_TYPE(16, 4):
            {
                node.Data.InvokeMethod.Method = nullptr;
                break;
            }
                // Get/Set Field
            case GRAPH_NODE_MAKE_TYPE(16, 7):
            case GRAPH_NODE_MAKE_TYPE(16, 8):
            {
                node.Data.GetSetField.Field = nullptr;
                break;
            }
            }
        }
    }
}

#endif

void VisualScriptingBinaryModule::OnEvent(ScriptingObject* object, Span<Variant> parameters, ScriptingTypeHandle eventType, StringView eventName)
{
    if (object)
    {
        // Object event
        const auto& type = object->GetType();
        Guid id;
        if (Guid::Parse(type.Fullname, id))
            return;
        if (const auto visualScript = (VisualScript*)Content::GetAsset(id))
        {
            if (auto instance = visualScript->GetScriptInstance(object))
            {
                for (auto& b : instance->EventBindings)
                {
                    if (b.Type != eventType || b.Name != eventName)
                        continue;
                    for (auto& m : b.BindedMethods)
                    {
                        VisualScripting::Invoke(m, object, parameters);
                    }
                }
            }
        }
    }
    else
    {
        // Static event
        for (auto& asset : Content::GetAssetsRaw())
        {
            if (const auto visualScript = ScriptingObject::Cast<VisualScript>(asset.Value))
            {
                for (auto& e : visualScript->_instances)
                {
                    auto instance = &e.Value;
                    for (auto& b : instance->EventBindings)
                    {
                        if (b.Type != eventType || b.Name != eventName)
                            continue;
                        for (auto& m : b.BindedMethods)
                        {
                            VisualScripting::Invoke(m, object, parameters);
                        }
                    }
                }
            }
        }
    }
}

const StringAnsi& VisualScriptingBinaryModule::GetName() const
{
    return _name;
}

bool VisualScriptingBinaryModule::IsLoaded() const
{
    return true;
}

bool VisualScriptingBinaryModule::FindScriptingType(const StringAnsiView& typeName, int32& typeIndex)
{
    // Type Name for Visual Scripts is 32 chars Guid representation of asset ID
    if (typeName.Length() == 32)
    {
        if (TypeNameToTypeIndex.TryGet(typeName, typeIndex))
            return true;
        Guid id;
        if (!Guid::Parse(typeName, id))
        {
            const auto visualScript = Content::LoadAsync<VisualScript>(id);
            if (visualScript)
            {
                const auto handle = visualScript->GetScriptingType();
                if (handle)
                {
                    typeIndex = handle.TypeIndex;
                    return true;
                }
            }
        }
    }
    return false;
}

void* VisualScriptingBinaryModule::FindMethod(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, int32 numParams)
{
    return (void*)Scripts[typeHandle.TypeIndex]->FindMethod(name, numParams);
}

bool VisualScriptingBinaryModule::InvokeMethod(void* method, const Variant& instance, Span<Variant> paramValues, Variant& result)
{
    auto vsMethod = (VisualScript::Method*)method;
    ScriptingObject* instanceObject = nullptr;
    if (!vsMethod->Signature.IsStatic)
    {
        instanceObject = (ScriptingObject*)instance;
        if (!instanceObject || instanceObject->GetTypeHandle() != vsMethod->Script->GetScriptingType())
        {
            if (!instanceObject)
                LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) without object instance", String(vsMethod->Script->GetScriptTypeName()), String(vsMethod->Name), vsMethod->ParamNames.Count());
            else
                LOG(Error, "Failed to call method '{0}.{1}' (args count: {2}) with invalid object instance of type '{3}'", String(vsMethod->Script->GetScriptTypeName()), String(vsMethod->Name), vsMethod->ParamNames.Count(), String(instanceObject->GetType().Fullname));
            return true;
        }
    }
    result = VisualScripting::Invoke(vsMethod, instanceObject, paramValues);
    return false;
}

void VisualScriptingBinaryModule::GetMethodSignature(void* method, ScriptingTypeMethodSignature& methodSignature)
{
    const auto vsMethod = (const VisualScript::Method*)method;
    methodSignature = vsMethod->Signature;
}

void* VisualScriptingBinaryModule::FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name)
{
    return (void*)Scripts[typeHandle.TypeIndex]->FindField(name);
}

void VisualScriptingBinaryModule::GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature)
{
    const auto vsFiled = (VisualScript::Field*)field;
    fieldSignature.Name = vsFiled->Name;
    fieldSignature.ValueType = vsFiled->Parameter->Type;
    fieldSignature.IsStatic = false;
}

bool VisualScriptingBinaryModule::GetFieldValue(void* field, const Variant& instance, Variant& result)
{
    const auto vsFiled = (VisualScript::Field*)field;
    const auto instanceObject = (ScriptingObject*)instance;
    if (!instanceObject)
    {
        LOG(Error, "Failed to get field '{0}' without object instance", vsFiled->Parameter->Name);
        return true;
    }
    const auto instanceParams = vsFiled->Script->_instances.Find(instanceObject->GetID());
    if (!instanceParams)
    {
        LOG(Error, "Missing parameters for the object instance.");
        return true;
    }
    result = instanceParams->Value.Params[vsFiled->Index];
    return false;
}

bool VisualScriptingBinaryModule::SetFieldValue(void* field, const Variant& instance, Variant& value)
{
    const auto vsFiled = (VisualScript::Field*)field;
    const auto instanceObject = (ScriptingObject*)instance;
    if (!instanceObject)
    {
        LOG(Error, "Failed to set field '{0}' without object instance", vsFiled->Parameter->Name);
        return true;
    }
    const auto instanceParams = vsFiled->Script->_instances.Find(instanceObject->GetID());
    if (!instanceParams)
    {
        LOG(Error, "Missing parameters for the object instance.");
        return true;
    }
    instanceParams->Value.Params[vsFiled->Index] = value;
    return false;
}

void VisualScriptingBinaryModule::SerializeObject(JsonWriter& stream, ScriptingObject* object, const ScriptingObject* otherObj)
{
    char idName[33];
    stream.StartObject();
    const auto asset = Scripts[object->GetTypeHandle().TypeIndex].Get();
    if (asset)
    {
        const auto instanceParams = asset->_instances.Find(object->GetID());
        if (instanceParams)
        {
            auto& params = instanceParams->Value.Params;
            if (otherObj)
            {
                // Serialize parameters diff
                const auto otherParams = asset->_instances.Find(otherObj->GetID());
                if (otherParams)
                {
                    for (int32 paramIndex = 0; paramIndex < params.Count(); paramIndex++)
                    {
                        auto& param = asset->Graph.Parameters[paramIndex];
                        auto& value = params[paramIndex];
                        auto& otherValue = otherParams->Value.Params[paramIndex];
                        if (value != otherValue)
                        {
                            param.Identifier.ToString(idName, Guid::FormatType::N);
                            stream.Key(idName, 32);
                            Serialization::Serialize(stream, params[paramIndex], &otherValue);
                        }
                    }
                }
                else
                {
                    for (int32 paramIndex = 0; paramIndex < params.Count(); paramIndex++)
                    {
                        auto& param = asset->Graph.Parameters[paramIndex];
                        auto& value = params[paramIndex];
                        auto& otherValue = param.Value;
                        if (value != otherValue)
                        {
                            param.Identifier.ToString(idName, Guid::FormatType::N);
                            stream.Key(idName, 32);
                            Serialization::Serialize(stream, params[paramIndex], &otherValue);
                        }
                    }
                }
            }
            else
            {
                // Serialize all parameters
                for (int32 paramIndex = 0; paramIndex < params.Count(); paramIndex++)
                {
                    auto& param = asset->Graph.Parameters[paramIndex];
                    auto& value = params[paramIndex];
                    param.Identifier.ToString(idName, Guid::FormatType::N);
                    stream.Key(idName, 32);
                    Serialization::Serialize(stream, value, nullptr);
                }
            }
        }
    }
    stream.EndObject();
}

void VisualScriptingBinaryModule::DeserializeObject(ISerializable::DeserializeStream& stream, ScriptingObject* object, ISerializeModifier* modifier)
{
    ASSERT(stream.IsObject());
    const auto asset = Scripts[object->GetTypeHandle().TypeIndex].Get();
    if (asset)
    {
        const auto instanceParams = asset->_instances.Find(object->GetID());
        if (instanceParams)
        {
            // Deserialize all parameters
            auto& params = instanceParams->Value.Params;
            for (auto i = stream.MemberBegin(); i != stream.MemberEnd(); ++i)
            {
                StringAnsiView idNameAnsi(i->name.GetStringAnsiView());
                Guid paramId;
                if (!Guid::Parse(idNameAnsi, paramId))
                {
                    int32 paramIndex;
                    if (asset->Graph.GetParameter(paramId, paramIndex))
                    {
                        Serialization::Deserialize(i->value, params[paramIndex], modifier);
                    }
                }
            }
        }
    }
}

void VisualScriptingBinaryModule::OnObjectIdChanged(ScriptingObject* object, const Guid& oldId)
{
    const auto asset = Scripts[object->GetTypeHandle().TypeIndex].Get();
    if (asset)
    {
        auto& instanceParams = asset->_instances[object->GetID()];
        auto oldParams = asset->_instances.Find(oldId);
        if (oldParams)
        {
            instanceParams = MoveTemp(oldParams->Value);
            asset->_instances.Remove(oldParams);
        }
    }
}

void VisualScriptingBinaryModule::OnObjectDeleted(ScriptingObject* object)
{
    const auto asset = Scripts[object->GetTypeHandle().TypeIndex].Get();
    if (asset)
    {
        // Cleanup object data
        asset->_instances.Remove(object->GetID());
    }
}

void VisualScriptingBinaryModule::Destroy(bool isReloading)
{
    // Skip module unregister during reloads (Visual Scripts are persistent)
    if (isReloading)
        return;

    BinaryModule::Destroy(isReloading);
}

ScriptingTypeHandle VisualScript::GetScriptingType()
{
    if (!_scriptingTypeHandle && !WaitForLoaded())
    {
        CacheScriptingType();
    }
    return _scriptingTypeHandle;
}

ScriptingObject* VisualScript::CreateInstance()
{
    const auto scriptingTypeHandle = GetScriptingType();
    return scriptingTypeHandle ? scriptingTypeHandle.GetType().Script.Spawn(ScriptingObjectSpawnParams(Guid::New(), scriptingTypeHandle)) : nullptr;
}

VisualScript::Instance* VisualScript::GetScriptInstance(ScriptingObject* instance) const
{
    return instance ? _instances.TryGet(instance->GetID()) : nullptr;
}

Variant VisualScript::GetScriptInstanceParameterValue(const StringView& name, ScriptingObject* instance) const
{
    CHECK_RETURN(instance, Variant());
    for (int32 paramIndex = 0; paramIndex < Graph.Parameters.Count(); paramIndex++)
    {
        if (Graph.Parameters[paramIndex].Name == name)
        {
            const auto instanceParams = _instances.Find(instance->GetID());
            if (instanceParams)
                return instanceParams->Value.Params[paramIndex];
            LOG(Error, "Failed to access Visual Script parameter {1} for {0}.", instance->ToString(), name);
            return Graph.Parameters[paramIndex].Value;
        }
    }
    LOG(Warning, "Failed to get {0} parameter '{1}'", ToString(), name);
    return Variant();
}

void VisualScript::SetScriptInstanceParameterValue(const StringView& name, ScriptingObject* instance, const Variant& value) const
{
    CHECK(instance);
    for (int32 paramIndex = 0; paramIndex < Graph.Parameters.Count(); paramIndex++)
    {
        if (Graph.Parameters[paramIndex].Name == name)
        {
            const auto instanceParams = _instances.Find(instance->GetID());
            if (instanceParams)
            {
                instanceParams->Value.Params[paramIndex] = value;
                return;
            }
            LOG(Error, "Failed to access Visual Script parameter {1} for {0}.", instance->ToString(), name);
            return;
        }
    }
    LOG(Warning, "Failed to set {0} parameter '{1}'", ToString(), name);
}

void VisualScript::SetScriptInstanceParameterValue(const StringView& name, ScriptingObject* instance, Variant&& value) const
{
    CHECK(instance);
    for (int32 paramIndex = 0; paramIndex < Graph.Parameters.Count(); paramIndex++)
    {
        if (Graph.Parameters[paramIndex].Name == name)
        {
            const auto instanceParams = _instances.Find(instance->GetID());
            if (instanceParams)
            {
                instanceParams->Value.Params[paramIndex] = MoveTemp(value);
                return;
            }
        }
    }
    LOG(Warning, "Failed to set {0} parameter '{1}'", ToString(), name);
}

const VisualScript::Method* VisualScript::FindMethod(const StringAnsiView& name, int32 numParams) const
{
    for (const auto& e : _methods)
    {
        if (e.Signature.Params.Count() == numParams && e.Name == name)
            return &e;
    }
    return nullptr;
}

const VisualScript::Field* VisualScript::FindField(const StringAnsiView& name) const
{
    for (const auto& e : _fields)
    {
        if (e.Name == name)
            return &e;
    }
    return nullptr;
}

BytesContainer VisualScript::LoadSurface()
{
    ScopeLock lock(Locker);
    if (!LoadChunks(GET_CHUNK_FLAG(0)))
    {
        const auto data = GetChunk(0);
        BytesContainer result;
        result.Copy(data->Data);
        return result;
    }

    LOG(Warning, "\'{0}\' surface data is missing.", ToString());
    return BytesContainer();
}

#if USE_EDITOR

bool VisualScript::SaveSurface(BytesContainer& data, const Metadata& meta)
{
    // Wait for asset to be loaded or don't if last load failed
    if (LastLoadFailed())
    {
        LOG(Warning, "Saving asset that failed to load.");
    }
    else if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }

    ScopeLock lock(Locker);

    // Release all chunks
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        ReleaseChunk(i);

    // Set Visject Surface data
    auto visjectSurfaceChunk = GetOrCreateChunk(0);
    visjectSurfaceChunk->Data.Copy(data);

    // Set metadata
    auto metadataChunk = GetOrCreateChunk(1);
    MemoryWriteStream metaStream(512);
    {
        metaStream.WriteInt32(1);
        metaStream.WriteString(meta.BaseTypename, 31);
        metaStream.WriteInt32((int32)meta.Flags);
    }
    metadataChunk->Data.Copy(metaStream.GetHandle(), metaStream.GetPosition());

    // Save
    AssetInitData assetData;
    assetData.SerializedVersion = 1;
    if (SaveAsset(assetData))
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

void VisualScript::GetMethodSignature(int32 index, String& name, byte& flags, String& returnTypeName, Array<String>& paramNames, Array<String>& paramTypeNames, Array<bool>& paramOuts)
{
    auto& method = _methods[index];
    name = String(method.Name);
    flags = (byte)method.MethodFlags;
    returnTypeName = method.Signature.ReturnType.GetTypeName();
    paramNames.Resize(method.Signature.Params.Count());
    paramTypeNames.Resize(method.Signature.Params.Count());
    paramOuts.Resize(method.Signature.Params.Count());
    for (int32 i = 0; i < method.Signature.Params.Count(); i++)
    {
        auto& param = method.Signature.Params[i];
        paramNames[i] = String(method.ParamNames[i]);
        paramTypeNames[i] = param.Type.GetTypeName();
        paramOuts[i] = param.IsOut;
    }
}

Span<byte> VisualScript::GetMetaData(int32 typeID)
{
    auto meta = Graph.Meta.GetEntry(typeID);
    return meta ? ToSpan(meta->Data.Get(), meta->Data.Count()) : Span<byte>(nullptr, 0);
}

Span<byte> VisualScript::GetMethodMetaData(int32 index, int32 typeID)
{
    auto& method = _methods[index];
    auto meta = method.Node->Meta.GetEntry(typeID);
    return meta ? ToSpan(meta->Data.Get(), meta->Data.Count()) : Span<byte>(nullptr, 0);
}

#endif

VisualScripting::StackFrame* VisualScripting::GetThreadStackTop()
{
    return ThreadStacks.Get().Stack;
}

String VisualScripting::GetStackTrace()
{
    String result;
    auto frame = ThreadStacks.Get().Stack;
    while (frame)
    {
        String node;
        switch (frame->Node->Type)
        {
            // Get/Set Parameter
        case GRAPH_NODE_MAKE_TYPE(6, 3):
        case GRAPH_NODE_MAKE_TYPE(6, 4):
        {
            const auto param = frame->Script->Graph.GetParameter((Guid)frame->Node->Values[0]);
            node = frame->Node->TypeID == 3 ? TEXT("Get ") : TEXT("Set ");
            node += param ? param->Name : ((Guid)frame->Node->Values[0]).ToString();
            break;
        }
            // Method Override
        case GRAPH_NODE_MAKE_TYPE(16, 3):
            node = (StringView)frame->Node->Values[0];
            node += TEXT("()");
            break;
            // Invoke Method
        case GRAPH_NODE_MAKE_TYPE(16, 4):
            node = (StringView)frame->Node->Values[0];
            node += TEXT(".");
            node += (StringView)frame->Node->Values[1];
            node += TEXT("()");
            break;
            // Function
        case GRAPH_NODE_MAKE_TYPE(16, 6):
            node = String(frame->Script->GetScriptTypeName());
            for (int32 i = 0; i < frame->Script->_methods.Count(); i++)
            {
                auto& method = frame->Script->_methods[i];
                if (method.Node == frame->Node)
                {
                    node += TEXT(".");
                    node += String(method.Name);
                    node += TEXT("()");
                    break;
                }
            }
            break;
        default:
            node = StringUtils::ToString(frame->Node->Type);
            break;
        }
        result += String::Format(TEXT("    at {0}:{1} in node {2}\n"), StringUtils::GetFileNameWithoutExtension(frame->Script->GetPath()), frame->Script->GetID(), node);
        frame = frame->PreviousFrame;
    }
    return result;
}

VisualScriptingBinaryModule* VisualScripting::GetBinaryModule()
{
    return &VisualScriptingModule;
}

Variant VisualScripting::Invoke(VisualScript::Method* method, ScriptingObject* instance, Span<Variant> parameters)
{
    CHECK_RETURN(method && method->Script->IsLoaded(), Variant::Zero);
    PROFILE_CPU_NAMED(*method->ProfilerName);

    // Add to the calling stack
    ScopeContext scope;
    scope.Parameters = parameters;
    auto& stack = ThreadStacks.Get();
    StackFrame frame;
    frame.Script = method->Script;
    frame.Node = method->Node;
    frame.Box = method->Node->GetBox(0);
    frame.Instance = instance;
    frame.PreviousFrame = stack.Stack;
    frame.Scope = &scope;
    stack.Stack = &frame;
    stack.StackFramesCount++;

    // Call per group custom processing event
    const auto func = VisualScriptingExecutor._perGroupProcessCall[method->Node->GroupID];
    (VisualScriptingExecutor.*func)(frame.Box, method->Node, scope.FunctionReturn);

    // Remove from the calling stack
    stack.StackFramesCount--;
    stack.Stack = frame.PreviousFrame;

    return scope.FunctionReturn;
}

#if VISUAL_SCRIPT_DEBUGGING

bool VisualScripting::Evaluate(VisualScript* script, ScriptingObject* instance, uint32 nodeId, uint32 boxId, Variant& result)
{
    if (!script)
        return false;
    const auto node = script->Graph.GetNode(nodeId);
    if (!node)
        return false;
    const auto box = node->GetBox(boxId);
    if (!box)
        return false;

    // Add to the calling stack
    ScopeContext scope;
    auto& stack = ThreadStacks.Get();
    StackFrame frame;
    frame.Script = script;
    frame.Node = node;
    frame.Box = box;
    frame.Instance = instance;
    frame.PreviousFrame = stack.Stack;
    frame.Scope = &scope;
    stack.Stack = &frame;
    stack.StackFramesCount++;

    // Call per group custom processing event
    const auto func = VisualScriptingExecutor._perGroupProcessCall[node->GroupID];
    (VisualScriptingExecutor.*func)(box, node, result);

    // Remove from the calling stack
    stack.StackFramesCount--;
    stack.Stack = frame.PreviousFrame;

    return true;
}

#endif
