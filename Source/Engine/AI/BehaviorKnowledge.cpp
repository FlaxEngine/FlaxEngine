// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "BehaviorKnowledge.h"
#include "BehaviorTree.h"
#include "BehaviorTreeNodes.h"
#include "BehaviorKnowledgeSelector.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#if USE_CSHARP
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#endif

bool AccessVariant(Variant& instance, const StringAnsiView& member, Variant& value, bool set)
{
    if (member.IsEmpty())
    {
        // Whole blackboard value
        CHECK_RETURN(instance.Type == value.Type, false);
        if (set)
            instance = value;
        else
            value = instance;
        return true;
    }
    // TODO: support further path for nested value types (eg. structure field access)

    const StringAnsiView typeName(instance.Type.TypeName);
    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
    if (typeHandle)
    {
        const ScriptingType& type = typeHandle.GetType();
        switch (type.Type)
        {
        case ScriptingTypes::Structure:
        {
            const String memberStr(member);
            // TODO: let SetField/GetField return boolean status of operation maybe?
            if (set)
                type.Struct.SetField(instance.AsBlob.Data, memberStr, value);
            else
                type.Struct.GetField(instance.AsBlob.Data, memberStr, value);
            return true;
        }
        default:
        {
            if (void* field = typeHandle.Module->FindField(typeHandle, member))
            {
                if (set)
                    return !typeHandle.Module->SetFieldValue(field, instance, value);
                else
                    return !typeHandle.Module->GetFieldValue(field, instance, value);
            }
            break;
        }
        }
    }
#if USE_CSHARP
    if (const auto mClass = Scripting::FindClass(typeName))
    {
        MObject* instanceObject = MUtils::BoxVariant(instance);
        if (const auto mField = mClass->GetField(member.Get()))
        {
            bool failed;
            if (set)
                mField->SetValue(instanceObject, MUtils::VariantToManagedArgPtr(value, mField->GetType(), failed));
            else
                value = MUtils::UnboxVariant(mField->GetValueBoxed(instanceObject));
            return true;
        }
        else if (const auto mProperty = mClass->GetProperty(member.Get()))
        {
            if (set)
                mProperty->SetValue(instanceObject, MUtils::BoxVariant(value), nullptr);
            else
                value = MUtils::UnboxVariant(mProperty->GetValue(instanceObject, nullptr));
            return true;
        }
    }
#endif
    else
    {
        LOG(Warning, "Missing scripting type \'{0}\'", String(typeName));
    }

    return false;
}

bool AccessBehaviorKnowledge(BehaviorKnowledge* knowledge, const StringAnsiView& path, Variant& value, bool set)
{
    const int32 typeEnd = path.Find('/');
    if (typeEnd == -1)
        return false;
    const StringAnsiView type(path.Get(), typeEnd);
    if (type == "Blackboard")
    {
        const StringAnsiView member(path.Get() + typeEnd + 1, path.Length() - typeEnd - 1);
        return AccessVariant(knowledge->Blackboard, member, value, set);
    }
    // TODO: goals and sensors data access from BehaviorKnowledge via Selector
    return false;
}

bool BehaviorKnowledgeSelectorAny::Set(BehaviorKnowledge* knowledge, const Variant& value)
{
    return knowledge && knowledge->Set(Path, value);
}

Variant BehaviorKnowledgeSelectorAny::Get(BehaviorKnowledge* knowledge)
{
    Variant value;
    if (knowledge)
        knowledge->Get(Path, value);
    return value;
}

bool BehaviorKnowledgeSelectorAny::TryGet(BehaviorKnowledge* knowledge, Variant& value)
{
    return knowledge && knowledge->Get(Path, value);
}

BehaviorKnowledge::~BehaviorKnowledge()
{
    FreeMemory();
}

void BehaviorKnowledge::InitMemory(BehaviorTree* tree)
{
    ASSERT_LOW_LAYER(!Tree && tree);
    Tree = tree;
    Blackboard = Variant::NewValue(tree->Graph.Root->BlackboardType);
    RelevantNodes.Resize(tree->Graph.NodesCount, false);
    RelevantNodes.SetAll(false);
    if (!Memory && tree->Graph.NodesStatesSize)
        Memory = Allocator::Allocate(tree->Graph.NodesStatesSize);
}

void BehaviorKnowledge::FreeMemory()
{
    if (Memory)
    {
        // Release any outstanding nodes state and memory
        ASSERT_LOW_LAYER(Tree);
        BehaviorUpdateContext context;
        context.Behavior = Behavior;
        context.Knowledge = this;
        context.Memory = Memory;
        context.RelevantNodes = &RelevantNodes;
        context.DeltaTime = 0.0f;
        context.Time = 0.0f;
        for (const auto& node : Tree->Graph.Nodes)
        {
            if (node.Instance && node.Instance->_executionIndex != -1 && RelevantNodes[node.Instance->_executionIndex])
                node.Instance->ReleaseState(context);
        }
        Allocator::Free(Memory);
        Memory = nullptr;
    }
    RelevantNodes.Clear();
    Blackboard.DeleteValue();
    Tree = nullptr;
}

bool BehaviorKnowledge::Get(const StringAnsiView& path, Variant& value)
{
    return AccessBehaviorKnowledge(this, path, value, false);
}

bool BehaviorKnowledge::Set(const StringAnsiView& path, const Variant& value)
{
    return AccessBehaviorKnowledge(this, path, const_cast<Variant&>(value), true);
}

bool BehaviorKnowledge::CompareValues(float a, float b, BehaviorValueComparison comparison)
{
    switch (comparison)
    {
    case BehaviorValueComparison::Equal:
        return Math::NearEqual(a, b);
    case BehaviorValueComparison::NotEqual:
        return Math::NotNearEqual(a, b);
    case BehaviorValueComparison::Less:
        return a < b;
    case BehaviorValueComparison::LessEqual:
        return a <= b;
    case BehaviorValueComparison::Greater:
        return a > b;
    case BehaviorValueComparison::GreaterEqual:
        return a >= b;
    default:
        return false;
    }
}
