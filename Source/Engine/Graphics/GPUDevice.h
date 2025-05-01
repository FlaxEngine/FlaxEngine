// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "GPUAdapter.h"
#include "GPULimits.h"
#include "Enums.h"
#include "Config.h"

class ITextureOwner;
class RenderTask;
class GPUResource;
class GPUContext;
class GPUShader;
class GPUTimerQuery;
class GPUTexture;
class GPUBuffer;
class GPUSampler;
class GPUPipelineState;
class GPUConstantBuffer;
class GPUVertexLayout;
class GPUTasksContext;
class GPUTasksExecutor;
class GPUSwapChain;
class GPUTasksManager;
class Shader;
class Model;
class Material;
class MaterialBase;

/// <summary>
/// Graphics device object for rendering on GPU.
/// </summary>
API_CLASS(Sealed, NoSpawn) class FLAXENGINE_API GPUDevice : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUDevice);
public:
    /// <summary>
    /// Graphics Device states that describe its lifetime.
    /// </summary>
    enum class DeviceState
    {
        Missing = 0,
        Created,
        Ready,
        Removed,
        Disposing,
        Disposed
    };

    /// <summary>
    /// Describes a video output display mode.
    /// </summary>
    API_STRUCT() struct VideoOutputMode
    {
        DECLARE_SCRIPTING_TYPE_NO_SPAWN(VideoOutputMode);

        /// <summary>
        /// The resolution width (in pixel).
        /// </summary>
        API_FIELD() uint32 Width;

        /// <summary>
        /// The resolution height (in pixel).
        /// </summary>
        API_FIELD() uint32 Height;

        /// <summary>
        /// The screen refresh rate (in hertz).
        /// </summary>
        API_FIELD() uint32 RefreshRate;
    };

    /// <summary>
    /// The singleton instance of the graphics device.
    /// </summary>
    API_FIELD(ReadOnly) static GPUDevice* Instance;

protected:
    // State
    DeviceState _state;
    bool _isRendering;
    bool _wasVSyncUsed;
    int32 _drawGpuEventIndex;
    RendererType _rendererType;
    ShaderProfile _shaderProfile;
    FeatureLevel _featureLevel;

    // Private resources (hidden with declaration)
    struct PrivateData;
    PrivateData* _res;
    Array<GPUResource*> _resources;
    CriticalSection _resourcesLock;

    void OnRequestingExit();

protected:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUDevice"/> class.
    /// </summary>
    /// <param name="type">The renderer type.</param>
    /// <param name="profile">The shader profile.</param>
    GPUDevice(RendererType type, ShaderProfile profile);

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="GPUDevice"/> class.
    /// </summary>
    virtual ~GPUDevice();

public:
    /// <summary>
    /// The graphics device locking mutex.
    /// </summary>
    CriticalSection Locker;

    /// <summary>
    /// The total amount of graphics memory in bytes.
    /// </summary>
    API_FIELD(ReadOnly) uint64 TotalGraphicsMemory;

    /// <summary>
    /// Indicates that debug tool is profiling device (eg. RenderDoc).
    /// </summary>
    API_FIELD(ReadOnly) bool IsDebugToolAttached;

    /// <summary>
    /// The GPU limits.
    /// </summary>
    API_FIELD(ReadOnly) GPULimits Limits;

    /// <summary>
    /// The available video output modes.
    /// </summary>
    API_FIELD(ReadOnly) Array<VideoOutputMode> VideoOutputModes;

    /// <summary>
    /// Quad rendering shader
    /// </summary>
    API_FIELD(ReadOnly) GPUShader* QuadShader;

    /// <summary>
    /// The current task being executed.
    /// </summary>
    RenderTask* CurrentTask;

    /// <summary>
    /// The supported features for the specified format (index is the pixel format value).
    /// </summary>
    FormatFeatures FeaturesPerFormat[static_cast<int32>(PixelFormat::MAX)];

    /// <summary>
    /// Gets the supported features for the specified format (index is the pixel format value).
    /// </summary>
    /// <param name="format">The format.</param>
    /// <returns>The format features flags.</returns>
    API_FUNCTION() FormatFeatures GetFormatFeatures(const PixelFormat format) const
    {
        return FeaturesPerFormat[(int32)format];
    }

public:
    /// <summary>
    /// Gets current device state.
    /// </summary>
    FORCE_INLINE DeviceState GetState() const
    {
        return _state;
    }

    /// <summary>
    /// Returns true if device is during rendering state, otherwise false.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsRendering() const
    {
        return _isRendering;
    }

    /// <summary>
    /// Returns true if VSync was used during the last frame present.
    /// </summary>
    FORCE_INLINE bool WasVSyncUsed() const
    {
        return _wasVSyncUsed;
    }

    /// <summary>
    /// Gets the device renderer type.
    /// </summary>
    API_PROPERTY() FORCE_INLINE RendererType GetRendererType() const
    {
        return _rendererType;
    }

    /// <summary>
    /// Gets device shader profile type.
    /// </summary>
    API_PROPERTY() FORCE_INLINE ShaderProfile GetShaderProfile() const
    {
        return _shaderProfile;
    }

    /// <summary>
    /// Gets device feature level type.
    /// </summary>
    API_PROPERTY() FORCE_INLINE FeatureLevel GetFeatureLevel() const
    {
        return _featureLevel;
    }

    /// <summary>
    /// Gets the main GPU context.
    /// </summary>
    API_PROPERTY() virtual GPUContext* GetMainContext() = 0;

    /// <summary>
    /// Gets the adapter device.
    /// </summary>
    API_PROPERTY() virtual GPUAdapter* GetAdapter() const = 0;

    /// <summary>
    /// Gets the native pointer to the underlying graphics device. It's a low-level platform-specific handle.
    /// </summary>
    API_PROPERTY() virtual void* GetNativePtr() const = 0;

    /// <summary>
    /// Gets the amount of memory usage by all the GPU resources (in bytes).
    /// </summary>
    API_PROPERTY() uint64 GetMemoryUsage() const;

    /// <summary>
    /// Gets the list with all active GPU resources.
    /// </summary>
    API_PROPERTY() Array<GPUResource*> GetResources() const;

    /// <summary>
    /// Gets the GPU asynchronous work manager.
    /// </summary>
    GPUTasksManager* GetTasksManager() const;

    /// <summary>
    /// Gets the default material.
    /// </summary>
    API_PROPERTY() MaterialBase* GetDefaultMaterial() const;

    /// <summary>
    /// Gets the default material (Deformable domain).
    /// </summary>
    MaterialBase* GetDefaultDeformableMaterial() const;

    /// <summary>
    /// Gets the default normal map texture.
    /// </summary>
    GPUTexture* GetDefaultNormalMap() const;

    /// <summary>
    /// Gets the default solid white texture.
    /// </summary>
    API_PROPERTY() GPUTexture* GetDefaultWhiteTexture() const;

    /// <summary>
    /// Gets the default solid black texture.
    /// </summary>
    API_PROPERTY() GPUTexture* GetDefaultBlackTexture() const;

    /// <summary>
    /// Gets the shader pipeline state object for linear, fullscreen texture copy.
    /// </summary>
    GPUPipelineState* GetCopyLinearPS() const;

    /// <summary>
    /// Gets the shader pipeline state object for solid-color texture clear.
    /// </summary>
    GPUPipelineState* GetClearPS() const;

    /// <summary>
    /// Gets the shader pipeline state object for YUY2 frame decoding to RGBA.
    /// </summary>
    GPUPipelineState* GetDecodeYUY2PS() const;

    /// <summary>
    /// Gets the shader pipeline state object for NV12 frame decoding to RGBA.
    /// </summary>
    GPUPipelineState* GetDecodeNV12PS() const;

    /// <summary>
    /// Gets the fullscreen-triangle vertex buffer.
    /// </summary>
    GPUBuffer* GetFullscreenTriangleVB() const;

public:
    /// <summary>
    /// Init device resources
    /// </summary>
    /// <returns>True if cannot init, otherwise false.</returns>
    virtual bool Init();

    /// <summary>
    /// Load private device content (called when Content Pool is created)
    /// </summary>
    /// <returns>True if cannot load data in a proper way, otherwise false.</returns>
    virtual bool LoadContent();

    /// <summary>
    /// Checks if GPU can render frame now (all data is ready), otherwise will skip frame rendering.
    /// </summary>
    /// <returns>True if skip rendering, otherwise false.</returns>
    virtual bool CanDraw();

    /// <summary>
    /// Call frame rendering and process data using GPU
    /// </summary>
    virtual void Draw();

    /// <summary>
    /// Clean all allocated data by device
    /// </summary>
    virtual void Dispose();

    /// <summary>
    /// Wait for GPU end doing submitted work
    /// </summary>
    virtual void WaitForGPU() = 0;

public:
    void AddResource(GPUResource* resource);
    void RemoveResource(GPUResource* resource);

    /// <summary>
    /// Dumps all GPU resources information to the log.
    /// </summary>
    void DumpResourcesToLog() const;

protected:
    virtual void preDispose();

    /// <summary>
    /// Called during Draw method before any frame rendering initialization. Cannot be used to submit commands to GPU.
    /// </summary>
    virtual void DrawBegin();

    /// <summary>
    /// Called during Draw method after rendering. Cannot be used to submit commands to GPU.
    /// </summary>
    virtual void DrawEnd();

    /// <summary>
    /// Called during Draw method after rendering begin. Can be used to submit commands to the GPU after opening GPU command list.
    /// </summary>
    virtual void RenderBegin();

    /// <summary>
    /// Called during Draw method before rendering end. Can be used to submit commands to the GPU before closing GPU command list.
    /// </summary>
    virtual void RenderEnd();

public:
    /// <summary>
    /// Creates the texture.
    /// </summary>
    /// <param name="name">The resource name.</param>
    /// <returns>The texture.</returns>
    API_FUNCTION() virtual GPUTexture* CreateTexture(const StringView& name = StringView::Empty) = 0;

    /// <summary>
    /// Creates the shader.
    /// </summary>
    /// <param name="name">The resource name.</param>
    /// <returns>The shader.</returns>
    virtual GPUShader* CreateShader(const StringView& name = StringView::Empty) = 0;

    /// <summary>
    /// Creates the GPU pipeline state object.
    /// </summary>
    /// <returns>The pipeline state.</returns>
    virtual GPUPipelineState* CreatePipelineState() = 0;

    /// <summary>
    /// Creates the timer query object.
    /// </summary>
    /// <returns>The timer query.</returns>
    virtual GPUTimerQuery* CreateTimerQuery() = 0;

    /// <summary>
    /// Creates the buffer.
    /// </summary>
    /// <param name="name">The resource name.</param>
    /// <returns>The buffer.</returns>
    API_FUNCTION() virtual GPUBuffer* CreateBuffer(const StringView& name = StringView::Empty) = 0;

    /// <summary>
    /// Creates the texture sampler.
    /// </summary>
    /// <returns>The sampler.</returns>
    API_FUNCTION() virtual GPUSampler* CreateSampler() = 0;

    /// <summary>
    /// Creates the vertex buffer layout.
    /// </summary>
    /// <returns>The vertex buffer layout.</returns>
    API_FUNCTION() virtual GPUVertexLayout* CreateVertexLayout(const Array<struct VertexElement, FixedAllocation<GPU_MAX_VS_ELEMENTS>>& elements, bool explicitOffsets = false) = 0;
    typedef Array<VertexElement, FixedAllocation<GPU_MAX_VS_ELEMENTS>> VertexElements;

    /// <summary>
    /// Creates the native window swap chain.
    /// </summary>
    /// <param name="window">The output window.</param>
    /// <returns>The native window swap chain.</returns>
    virtual GPUSwapChain* CreateSwapChain(Window* window) = 0;

    /// <summary>
    /// Creates the constant buffer.
    /// </summary>
    /// <param name="name">The resource name.</param>
    /// <param name="size">The buffer size (in bytes).</param>
    /// <returns>The constant buffer.</returns>
    virtual GPUConstantBuffer* CreateConstantBuffer(uint32 size, const StringView& name = StringView::Empty) = 0;

    /// <summary>
    /// Creates the GPU tasks context.
    /// </summary>
    /// <returns>The GPU tasks context.</returns>
    virtual GPUTasksContext* CreateTasksContext();

    /// <summary>
    /// Creates the GPU tasks executor.
    /// </summary>
    /// <returns>The GPU tasks executor.</returns>
    virtual GPUTasksExecutor* CreateTasksExecutor();
};

/// <summary>
/// Utility structure to safety graphics device locking.
/// </summary>
struct FLAXENGINE_API GPUDeviceLock : NonCopyable
{
    const GPUDevice* Device;

    GPUDeviceLock(const GPUDevice* device)
        : Device(device)
    {
        Device->Locker.Lock();
    }

    ~GPUDeviceLock()
    {
        Device->Locker.Unlock();
    }
};
