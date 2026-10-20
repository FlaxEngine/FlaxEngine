// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Singleton.h"
#include "Engine/Core/Object.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Profiler/Profiler.h"
#include "Config.h"

/// <summary>
/// Base class for renderer components called render pass.
/// Each render pass supports proper resources initialization and disposing.
/// </summary>
/// <seealso cref="Object" />
class FLAXENGINE_API RendererPassBase : public Object
{
protected:
#if !GPU_ENABLE_PRELOADING_RESOURCES
    bool _lazyInit = true;
#endif
    bool _hasValidResources = false;
#if COMPILE_WITH_PSO_PRECACHING
    volatile int32 _precacheHandle = 0;
#endif

    RendererPassBase()
    {
    }

public:
    /// <summary>
    /// Initializes the pass.
    /// </summary>
    virtual bool Init()
    {
        return false;
    }

#if COMPILE_WITH_PSO_PRECACHING
    /// <summary>
    /// Initializes PSOs precaching for the pass.
    /// </summary>
    void InitPrecachePSO();
#endif

    /// <summary>
    /// Cleanups pass data.
    /// </summary>
    virtual void Dispose();

    /// <summary>
    /// Determines whether can render this pass. Checks if pass is ready and has valid resources loaded.
    /// </summary>
    bool IsReady()
    {
        return !checkIfSkipPass();
    }

protected:
    bool checkIfSkipPass(bool precaching = false);

    void invalidateResources();

    virtual bool setupResources()
    {
        return false;
    }

#if COMPILE_WITH_PSO_PRECACHING
    /// <summary>
    /// Precaches shader pipelines to be ready ahead of time. PSOs used by this pass can warm-up in async before rendering happens with runtime-defined render target formats and vertex layouts.
    /// </summary>
    virtual void PrecachePSO()
    {
    }
#endif
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
#define CHECK_INVALID_SHADER_PASS_CB_SIZE(shader, index, dataType) ASSERT(shader && shader->GetCB(index)); if (shader->GetCB(index)->GetSize() != sizeof(dataType) && shader->GetCB(index)->GetSize() != 0) { REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, index, dataType); return true; }

#if COMPILE_WITH_DEV_ENV
#define BIND_SHADER_RELOADING(shader, pass, method) shader->Reloading.Bind<pass, &pass::method>(this)
#else
#define BIND_SHADER_RELOADING(shader, pass, method)
#endif

#if PLATFORM_WEB
// Hack to fix WebGPU limitation that requires to specify different sampler type manually (eg. to sample depth texture without filtering)
void SetWebGPUTextureViewSampler(GPUTextureView* view, uint32 samplerType);
#define GPU_WEBGPU_SAMPLER_TYPE_UNFILTERABLE_FLOAT 0x00000003 // WGPUTextureSampleType_UnfilterableFloat
#define GPU_WEBGPU_SAMPLER_TYPE_DEPTH 0x00000004 // WGPUTextureSampleType_Depth
#endif
