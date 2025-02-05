// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Config.h"

/// <summary>
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API GPUFence : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUBuffer);
    static GPUFence* Spawn(const SpawnParams& params);
    static GPUFence* New();
protected:
    GPUFence();
public:
};
