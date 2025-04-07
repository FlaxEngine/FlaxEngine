// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Physics simulation statistics container for profiler.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API PhysicsStatistics
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(PhysicsStatistics);

    // Number of active dynamic bodies for the current simulation step. Does not include active kinematic bodies.
    API_FIELD() uint32 ActiveDynamicBodies;
    // Number of active kinematic bodies for the current simulation step.
    API_FIELD() uint32 ActiveKinematicBodies;
    // Number of active joints object for the current simulation step.
    API_FIELD() uint32 ActiveJoints;
    // Number of static bodies for the current simulation step.
    API_FIELD() uint32 StaticBodies;
    // Number of dynamic bodies for the current simulation step.
    API_FIELD() uint32 DynamicBodies;
    // Number of kinematic bodies for the current simulation step.
    API_FIELD() uint32 KinematicBodies;
    // Number of new pairs found during this frame.
    API_FIELD() uint32 NewPairs;
    // Number of lost pairs during this frame.
    API_FIELD() uint32 LostPairs;
    // Number of new touches found during this frame. 
    API_FIELD() uint32 NewTouches;
    // Number of lost touches during this frame.
    API_FIELD() uint32 LostTouches;

    PhysicsStatistics()
    {
        Platform::MemoryClear(this, sizeof(PhysicsStatistics));
    }
};
