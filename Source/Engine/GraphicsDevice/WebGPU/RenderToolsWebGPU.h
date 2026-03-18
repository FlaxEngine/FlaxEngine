// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_WEBGPU

#include "Engine/Core/Types/String.h"
#include "IncludeWebGPU.h"

enum class PixelFormat : unsigned;

/// <summary>
/// User data used by AsyncCallbackWebGPU to track state adn result.
/// </summary>
struct AsyncCallbackDataWebGPU
{
    String Message;
    uint32 Status = 0;
    intptr Result = 0;
    double WaitTime = 0;

    FORCE_INLINE void Call(bool success, uint32 status, WGPUStringView message)
    {
        Status = status;
        if (!success)
            Message.Set(message.data, message.data ? (int32)message.length : 0);
        Platform::AtomicStore(&Result, success ? 1 : 2);
    }
};

struct AsyncWaitParamsWebGPU
{
    WGPUInstance Instance;
    AsyncCallbackDataWebGPU* Data;
};
WGPUWaitStatus WebGPUAsyncWait(AsyncWaitParamsWebGPU params);

/// <summary>
/// Helper utility to run WebGPU APIs that use async callback in sync by waiting on the spontaneous call back with an active-waiting loop.
/// </summary>
template<typename CallbackInfo, typename UserData = AsyncCallbackDataWebGPU>
struct AsyncCallbackWebGPU
{
    UserData Data;
    CallbackInfo Info;

    AsyncCallbackWebGPU(CallbackInfo callbackDefault)
        : Info(callbackDefault)
    {
        Info.mode = WGPUCallbackMode_AllowSpontaneous;
        Info.userdata1 = &Data;
    }

    FORCE_INLINE WGPUWaitStatus Wait(WGPUInstance instance)
    {
        return WebGPUAsyncWait({ instance, &Data });
    }
};

/// <summary>
/// Set of utilities for rendering on Web GPU platform.
/// </summary>
class RenderToolsWebGPU
{
public:
    static WGPUVertexFormat ToVertexFormat(PixelFormat format);
    static WGPUTextureFormat ToTextureFormat(PixelFormat format);
    static PixelFormat ToPixelFormat(WGPUTextureFormat format);
    static PixelFormat ToPixelFormat(WGPUVertexFormat format);
};

#endif
