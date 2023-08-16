// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Behavior logic component knowledge data container. Contains blackboard values, sensors data and goals storage for Behavior Tree execution.
/// </summary>
API_CLASS() class FLAXENGINE_API BehaviorKnowledge : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(BehaviorKnowledge, ScriptingObject);

    // TODO: blackboard
    // TODO: sensors data
    // TODO: goals
    // TODO: GetGoal/HasGoal
};
