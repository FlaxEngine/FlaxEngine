// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        if (set)
        {
            CHECK_RETURN(instance.Type == value.Type, false);
            instance = value;
        }
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
        bool failed = false;
        if (const auto mField = mClass->GetField(member.Get()))
        {
            if (set)
                mField->SetValue(instanceObject, MUtils::VariantToManagedArgPtr(value, mField->GetType(), failed));
            else
                value = MUtils::UnboxVariant(mField->GetValueBoxed(instanceObject));
            return !failed;
        }
        else if (const auto mProperty = mClass->GetProperty(member.Get()))
        {
            if (set)
                mProperty->SetValue(instanceObject, MUtils::VariantToManagedArgPtr(value, mProperty->GetType(), failed), nullptr);
            else
                value = MUtils::UnboxVariant(mProperty->GetValue(instanceObject, nullptr));
            return !failed;
        }
    }
#endif
    else if (typeName.HasChars())
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
    if (type == "Goal")
    {
        const StringAnsiView subPath(path.Get() + typeEnd + 1, path.Length() - typeEnd - 1);
        const int32 goalTypeEnd = subPath.Find('/');
        const StringAnsiView goalType(subPath.Get(), goalTypeEnd);
        const StringAnsiView member(subPath.Get() + goalTypeEnd + 1, subPath.Length() - goalTypeEnd - 1);
        for (Variant& goal : knowledge->Goals)
        {
            if (goalType == goal.Type.GetTypeName())
            {
                return AccessVariant(goal, member, value, set);
            }
        }
    }
    return false;
}

bool BehaviorKnowledgeSelectorAny::Set(BehaviorKnowledge* knowledge, const Variant& value) const
{
    return knowledge && knowledge->Set(Path, value);
}

Variant BehaviorKnowledgeSelectorAny::Get(const BehaviorKnowledge* knowledge) const
{
    Variant value;
    if (knowledge)
        knowledge->Get(Path, value);
    return value;
}

bool BehaviorKnowledgeSelectorAny::TryGet(const BehaviorKnowledge* knowledge, Variant& value) const
{
    return knowledge && knowledge->Get(Path, value);
}

BehaviorKnowledge::~BehaviorKnowledge()
{
    FreeMemory();
}

void BehaviorKnowledge::InitMemory(BehaviorTree* tree)
{
    if (Tree)
        FreeMemory();
    if (!tree)
        return;
    Tree = tree;
    Blackboard = Variant::NewValue(tree->Graph.Root->BlackboardType);
    RelevantNodes.Resize(tree->Graph.NodesCount, false);
    RelevantNodes.SetAll(false);
    if (!Memory && tree->Graph.NodesStatesSize)
    {
        Memory = Allocator::Allocate(tree->Graph.NodesStatesSize);
#if !BUILD_RELEASE
        // Clear memory to make it easier to spot missing data issues (eg. zero GCHandle in C# BT node due to missing state init)
        Platform::MemoryClear(Memory, tree->Graph.NodesStatesSize);
#endif
    }
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
    for (Variant& goal : Goals)
        goal.DeleteValue();
    Goals.Resize(0);
    Tree = nullptr;
}

bool BehaviorKnowledge::Get(const StringAnsiView& path, Variant& value) const
{
    return AccessBehaviorKnowledge(const_cast<BehaviorKnowledge*>(this), path, value, false);
}

bool BehaviorKnowledge::Set(const StringAnsiView& path, const Variant& value)
{
    return AccessBehaviorKnowledge(this, path, const_cast<Variant&>(value), true);
}

bool BehaviorKnowledge::HasGoal(ScriptingTypeHandle type) const
{
    for (int32 i = 0; i < Goals.Count(); i++)
    {
        const ScriptingTypeHandle goalType = Scripting::FindScriptingType(Goals[i].Type.GetTypeName());
        if (goalType == type)
            return true;
    }
    return false;
}

const Variant& BehaviorKnowledge::GetGoal(ScriptingTypeHandle type) const
{
    for (const Variant& goal : Goals)
    {
        const ScriptingTypeHandle goalType = Scripting::FindScriptingType(goal.Type.GetTypeName());
        if (goalType == type)
            return goal;
    }
    return Variant::Null;
}

void BehaviorKnowledge::AddGoal(Variant&& goal)
{
    int32 i = 0;
    for (; i < Goals.Count(); i++)
    {
        if (Goals[i].Type == goal.Type)
            break;
    }
    if (i == Goals.Count())
        Goals.AddDefault();
    Goals.Get()[i] = MoveTemp(goal);
}

void BehaviorKnowledge::RemoveGoal(ScriptingTypeHandle type)
{
    for (int32 i = 0; i < Goals.Count(); i++)
    {
        const ScriptingTypeHandle goalType = Scripting::FindScriptingType(Goals[i].Type.GetTypeName());
        if (goalType == type)
        {
            Goals.RemoveAt(i);
            break;
        }
    }
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
