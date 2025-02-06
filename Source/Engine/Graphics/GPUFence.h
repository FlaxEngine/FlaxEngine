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
    bool SignalCalled;
public:
    /// <summary>
    /// Updates a fence after all previous work has completed.
    /// </summary>
    API_FUNCTION() virtual void Signal() = 0;
    
    /// <summary>
    /// Waits for the work to complite
    /// </summary>
    API_FUNCTION() virtual void Wait() = 0;
};

template<class DeviceType, class BaseType>
class GPUFenceBase : public BaseType
{
protected:
    DeviceType* _device;
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceBase"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUFenceBase(DeviceType* device) noexcept
        : BaseType()
        , _device(device)
    {}
};
