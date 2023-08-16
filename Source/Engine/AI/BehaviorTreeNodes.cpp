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
