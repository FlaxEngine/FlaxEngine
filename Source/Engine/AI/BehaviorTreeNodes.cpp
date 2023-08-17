// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "BehaviorTreeNodes.h"
#include "Engine/Serialization/Serialization.h"

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
        result = child->Update(context);
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

    auto result = Children[state->CurrentChildIndex]->Update(context);

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
