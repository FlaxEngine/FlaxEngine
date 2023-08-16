// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/SerializableScriptingObject.h"
#include "BehaviorTypes.h"

/// <summary>
/// Base class for Behavior Tree nodes.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API BehaviorTreeNode : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorTreeNode, SerializableScriptingObject);

public:
    /// <summary>
    /// Node user name (eg. Follow Enemy, or Pick up Weapon).
    /// </summary>
    API_FIELD() String Name;

    // TODO: decorators/conditionals
    // TODO: instance data ctor/dtor
    // TODO: start/stop methods
    // TODO: update method

    /// <summary>
    /// Initializes node state. Called after whole tree is loaded and nodes hierarchy is setup.
    /// </summary>
    /// <param name="tree">Node owner asset.</param>
    API_FUNCTION() virtual void Init(BehaviorTree* tree)
    {
    }

    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
