// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "BehaviorTreeNodes.h"
#include "Behavior.h"
#include "BehaviorKnowledge.h"
#include "Engine/Core/Random.h"
#include "Engine/Scripting/Scripting.h"
#if USE_CSHARP
#include "Engine/Scripting/ManagedCLR/MClass.h"
#endif
#include "Engine/Serialization/Serialization.h"

bool IsAssignableFrom(const StringAnsiView& to, const StringAnsiView& from)
{
    // Special case of null
    if (to.IsEmpty())
        return from.IsEmpty();
    if (from.IsEmpty())
        return false;

    // Exact typename math
    if (to == from)
        return true;

    // Scripting Type match
    const ScriptingTypeHandle typeHandleTo = Scripting::FindScriptingType(to);
    const ScriptingTypeHandle typeHandleFrom = Scripting::FindScriptingType(from);
    if (typeHandleTo && typeHandleFrom)
        return typeHandleTo.IsAssignableFrom(typeHandleFrom);

#if USE_CSHARP
    // MClass match
    const auto mclassTo = Scripting::FindClass(to);
    const auto mclassFrom = Scripting::FindClass(from);
    if (mclassTo && mclassFrom)
        return mclassTo == mclassFrom || mclassFrom->IsSubClassOf(mclassTo);
#endif

    return false;
}

BehaviorUpdateResult BehaviorTreeNode::InvokeUpdate(const BehaviorUpdateContext& context)
{
    ASSERT_LOW_LAYER(_executionIndex != -1);
    BitArray<>& relevantNodes = *(BitArray<>*)context.RelevantNodes;
    if (relevantNodes.Get(_executionIndex) == false)
    {
        // Node becomes relevant so initialize it's state
        relevantNodes.Set(_executionIndex, true);
        InitState(context.Behavior, context.Memory);
    }

    // Node-specific update
    const BehaviorUpdateResult result = Update(context);

    // Check if node is not relevant anymore
    if (result != BehaviorUpdateResult::Running)
    {
        InvokeReleaseState(context);
    }

    return result;
}

void BehaviorTreeNode::Serialize(SerializeStream& stream, const void* otherObj)
{
    SerializableScriptingObject::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(BehaviorTreeNode);
    SERIALIZE(Name);
}

void BehaviorTreeNode::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    SerializableScriptingObject::Deserialize(stream, modifier);

    Name.Clear(); // Missing Name is assumes as unnamed node
    DESERIALIZE(Name);
}

void BehaviorTreeNode::InvokeReleaseState(const BehaviorUpdateContext& context)
{
    BitArray<>& relevantNodes = *(BitArray<>*)context.RelevantNodes;
    relevantNodes.Set(_executionIndex, false);
    ReleaseState(context.Behavior, context.Memory);
}

void BehaviorTreeCompoundNode::Init(BehaviorTree* tree)
{
    for (BehaviorTreeNode* child : Children)
        child->Init(tree);
}

BehaviorUpdateResult BehaviorTreeCompoundNode::Update(BehaviorUpdateContext context)
{
    auto result = BehaviorUpdateResult::Success;
    for (int32 i = 0; i < Children.Count() && result == BehaviorUpdateResult::Success; i++)
    {
        BehaviorTreeNode* child = Children[i];
        result = child->InvokeUpdate(context);
    }
    return result;
}

void BehaviorTreeCompoundNode::InvokeReleaseState(const BehaviorUpdateContext& context)
{
    const BitArray<>& relevantNodes = *(const BitArray<>*)context.RelevantNodes;
    for (BehaviorTreeNode* child : Children)
    {
        if (relevantNodes.Get(child->_executionIndex) == true)
        {
            child->InvokeReleaseState(context);
        }
    }

    BehaviorTreeNode::InvokeReleaseState(context);
}

int32 BehaviorTreeSequenceNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeSequenceNode::InitState(Behavior* behavior, void* memory)
{
    auto state = GetState<State>(memory);
    new(state)State();
}

BehaviorUpdateResult BehaviorTreeSequenceNode::Update(BehaviorUpdateContext context)
{
    auto state = GetState<State>(context.Memory);

    if (state->CurrentChildIndex >= Children.Count())
        return BehaviorUpdateResult::Success;
    if (state->CurrentChildIndex == -1)
        return BehaviorUpdateResult::Failed;

    auto result = Children[state->CurrentChildIndex]->InvokeUpdate(context);

    switch (result)
    {
    case BehaviorUpdateResult::Success:
        state->CurrentChildIndex++; // Move to the next node
        if (state->CurrentChildIndex < Children.Count())
            result = BehaviorUpdateResult::Running; // Keep on running to the next child on the next update
        break;
    case BehaviorUpdateResult::Failed:
        state->CurrentChildIndex = -1; // Mark whole sequence as failed
        break;
    }

    return result;
}

int32 BehaviorTreeSelectorNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeSelectorNode::InitState(Behavior* behavior, void* memory)
{
    auto state = GetState<State>(memory);
    new(state)State();
}

BehaviorUpdateResult BehaviorTreeSelectorNode::Update(BehaviorUpdateContext context)
{
    auto state = GetState<State>(context.Memory);

    if (state->CurrentChildIndex >= Children.Count())
        return BehaviorUpdateResult::Failed;

    auto result = Children[state->CurrentChildIndex]->InvokeUpdate(context);

    switch (result)
    {
    case BehaviorUpdateResult::Success:
        return BehaviorUpdateResult::Success;
    case BehaviorUpdateResult::Failed:
        state->CurrentChildIndex++; // Move to the next node
        if (state->CurrentChildIndex < Children.Count())
            result = BehaviorUpdateResult::Running; // Keep on running to the next child on the next update
        break;
    }

    return result;
}

int32 BehaviorTreeDelayNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeDelayNode::InitState(Behavior* behavior, void* memory)
{
    auto state = GetState<State>(memory);
    if (!WaitTimeSelector.TryGet(behavior->GetKnowledge(), state->TimeLeft))
        state->TimeLeft = WaitTime;
    state->TimeLeft = Random::RandRange(Math::Max(WaitTime - RandomDeviation, 0.0f), WaitTime + RandomDeviation);
}

BehaviorUpdateResult BehaviorTreeDelayNode::Update(BehaviorUpdateContext context)
{
    auto state = GetState<State>(context.Memory);
    state->TimeLeft -= context.DeltaTime;
    return state->TimeLeft <= 0.0f ? BehaviorUpdateResult::Success : BehaviorUpdateResult::Running;
}

int32 BehaviorTreeSubTreeNode::GetStateSize() const
{
    return sizeof(State);
}

void BehaviorTreeSubTreeNode::InitState(Behavior* behavior, void* memory)
{
    auto state = GetState<State>(memory);
    new(state)State();
    const BehaviorTree* tree = Tree.Get();
    if (!tree || tree->WaitForLoaded())
        return;
    state->Memory.Resize(tree->Graph.NodesStatesSize);
    state->RelevantNodes.Resize(tree->Graph.NodesCount, false);
    state->RelevantNodes.SetAll(false);
}

void BehaviorTreeSubTreeNode::ReleaseState(Behavior* behavior, void* memory)
{
    auto state = GetState<State>(memory);
    const BehaviorTree* tree = Tree.Get();
    if (tree && tree->IsLoaded())
    {
        for (const auto& node : tree->Graph.Nodes)
        {
            if (node.Instance && node.Instance->_executionIndex != -1 && state->RelevantNodes[node.Instance->_executionIndex])
                node.Instance->ReleaseState(behavior, state->Memory.Get());
        }
    }
    state->~State();
}

BehaviorUpdateResult BehaviorTreeSubTreeNode::Update(BehaviorUpdateContext context)
{
    const BehaviorTree* tree = Tree.Get();
    if (!tree || !tree->Graph.Root)
        return BehaviorUpdateResult::Failed;
    const StringAnsiView treeBlackboardType = tree->Graph.Root->BlackboardType;
    if (treeBlackboardType.HasChars())
    {
        // Validate if nested tree blackboard data matches (the same type or base type)
        const VariantType& blackboardType = context.Knowledge->Blackboard.Type;
        if (IsAssignableFrom(treeBlackboardType, StringAnsiView(blackboardType.GetTypeName())))
        {
            LOG(Error, "Cannot use nested '{}' with Blackboard of type '{}' inside '{}' with Blackboard of type '{}'",
                tree->ToString(), String(treeBlackboardType),
                context.Knowledge->Tree->ToString(), blackboardType.ToString());
            return BehaviorUpdateResult::Failed;
        }
    }

    // Override memory with custom one for the subtree
    auto state = GetState<State>(context.Memory);
    context.Memory = state->Memory.Get();
    context.RelevantNodes = &state->RelevantNodes;

    // Run nested tree
    return tree->Graph.Root->InvokeUpdate(context);
}

BehaviorUpdateResult BehaviorTreeForceFinishNode::Update(BehaviorUpdateContext context)
{
    context.Behavior->StopLogic(Result);
    return Result;
}
