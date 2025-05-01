// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Singleton.h"
#include "Engine/Core/Object.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Profiler/Profiler.h"
#include "Config.h"

class RendererUtils
{
public:

    static float TemporalHalton(int32 index, int32 base)
    {
        float result = 0.0f;
        const float invBase = 1.0f / base;
        float fraction = invBase;
        while (index > 0)
        {
            result += (index % base) * fraction;
            index /= base;
            fraction *= invBase;
        }
        return result;
    }
};

/// <summary>
/// Base class for renderer components called render pass.
/// Each render pass supports proper resources initialization and disposing.
/// </summary>
/// <seealso cref="Object" />
class FLAXENGINE_API RendererPassBase : public Object
{
protected:

    bool _hasValidResources;

    /// <summary>
    /// Init
    /// </summary>
    RendererPassBase()
    {
        _hasValidResources = false;
    }

public:

    /// <summary>
    /// Initialize service.
    /// </summary>
    virtual bool Init()
    {
        return false;
    }

    /// <summary>
    /// Cleanup service data.
    /// </summary>
    virtual void Dispose()
    {
        // Clear flag
        _hasValidResources = false;
    }

    /// <summary>
    /// Determines whether can render this pass. Checks if pass is ready and has valid resources loaded.
    /// </summary>
    /// <returns><c>true</c> if can render pass; otherwise, <c>false</c>.</returns>
    bool IsReady()
    {
        return !checkIfSkipPass();
    }

protected:

    bool checkIfSkipPass()
    {
        if (_hasValidResources)
            return false;

        const bool setupFailed = setupResources();
        _hasValidResources = !setupFailed;
        return setupFailed;
    }

    void invalidateResources()
    {
        // Clear flag
        _hasValidResources = false;
    }

    virtual bool setupResources()
    {
        return false;
    }
};

/// <summary>
/// Singleton render pass template.
/// </summary>
/// <seealso cref="Object" />
template<class T>
class RendererPass : public Singleton<T>, public RendererPassBase
{
};

#define REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, index, dataType) LOG(Fatal, "Shader {0} has incorrect constant buffer {1} size: {2} bytes. Expected: {3} bytes", shader->ToString(), index, shader->GetCB(index)->GetSize(), sizeof(dataType));
