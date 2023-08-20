// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "BehaviorTreeNodes.h"
#include "Behavior.h"
#include "BehaviorKnowledge.h"
#include "Engine/Core/Random.h"
#include "Engine/Serialization/Serialization.h"

BehaviorUpdateResult BehaviorTreeNode::InvokeUpdate(const BehaviorUpdateContext& context)
{
    ASSERT_LOW_LAYER(_executionIndex != -1);
    if (context.Knowledge->RelevantNodes.Get(_executionIndex) == false)
    {
        // Node becomes relevant so initialize it's state
        context.Knowledge->RelevantNodes.Set(_executionIndex, true);
        InitState(context.Behavior, context.Memory);
    }

    // Node-specific update
    const BehaviorUpdateResult result = Update(context);

    // Check if node is not relevant anymore
    if (result != BehaviorUpdateResult::Running)
    {
        context.Knowledge->RelevantNodes.Set(_executionIndex, false);
        ReleaseState(context.Behavior, context.Memory);
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
