// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Config.h"

/// <summary>
/// Releases GPU resource memory and deletes object.
/// </summary>
#define SAFE_DELETE_GPU_RESOURCE(x) if (x) { (x)->DeleteObjectNow(); (x) = nullptr; }
#define SAFE_DELETE_GPU_RESOURCES(x) for (auto& e : (x)) if (e) { e->DeleteObjectNow(); e = nullptr; }

/// <summary>
/// GPU resources types.
/// </summary>
API_ENUM() enum class GPUResourceType
{
    // GPU render target texture
    RenderTarget = 0,
    // GPU texture
    Texture,
    // GPU cube texture (cubemap)
    CubeTexture,
    // GPU volume texture (3D)
    VolumeTexture,
    // GPU buffer
    Buffer,
    // GPU shader
    Shader,
    // GPU pipeline state object (PSO)
    PipelineState,
    // GPU binding descriptor
    Descriptor,
    // GPU timer query
    Query,
    // GPU texture sampler
    Sampler,

    MAX
};

/// <summary>
/// The base class for all GPU resources.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API GPUResource : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUResource);
protected:
    uint64 _memoryUsage = 0;
#if GPU_ENABLE_RESOURCE_NAMING
    Char* _namePtr = nullptr;
    int32 _nameSize = 0, _nameCapacity = 0;
#endif

public:
    NON_COPYABLE(GPUResource);

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResource"/> class.
    /// </summary>
    GPUResource();

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResource"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    GPUResource(const SpawnParams& params);

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResource"/> class.
    /// </summary>
    virtual ~GPUResource();

public:
    // Points to the cache used by the resource for the resource visibility/usage detection. Written during rendering when resource is used.
    double LastRenderTime = -1;

    /// <summary>
    /// Action fired when resource GPU state gets released. All objects and async tasks using this resource should release references to this object nor use its data.
    /// </summary>
    Action Releasing;

public:
    /// <summary>
    /// Gets the GPU resource type.
    /// </summary>
    API_PROPERTY() virtual GPUResourceType GetResourceType() const = 0;

    /// <summary>
    /// Gets amount of GPU memory used by this resource (in bytes). It's a rough estimation. GPU memory may be fragmented, compressed or sub-allocated so the actual memory pressure from this resource may vary (also depends on the current graphics backend).
    /// </summary>
    API_PROPERTY() uint64 GetMemoryUsage() const;

#if !BUILD_RELEASE
    /// <summary>
    /// Gets the resource name.
    /// </summary>
    API_PROPERTY() StringView GetName() const;

    /// <summary>
    /// Sets the resource name.
    /// </summary>
    API_PROPERTY() void SetName(const StringView& name);
#endif

    /// <summary>
    /// Releases GPU resource data.
    /// </summary>
    API_FUNCTION() void ReleaseGPU();

    /// <summary>
    /// Action called when GPU device is disposing.
    /// </summary>
    virtual void OnDeviceDispose();

protected:
    /// <summary>
    /// Releases GPU resource data (implementation).
    /// </summary>
    virtual void OnReleaseGPU();

public:
    // [ScriptingObject]
    String ToString() const override;
    void OnDeleteObject() override;
};

/// <summary>
/// Describes base implementation of Graphics Device resource for rendering back-ends.
/// </summary>
/// <remarks>
/// DeviceType is GPU device typename, BaseType must be GPUResource type to inherit from).
/// </remarks>
template<class DeviceType, class BaseType>
class GPUResourceBase : public BaseType
{
protected:
    DeviceType* _device;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceBase"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUResourceBase(DeviceType* device, const StringView& name) noexcept
        : BaseType()
        , _device(device)
    {
#if GPU_ENABLE_RESOURCE_NAMING
        if (name.HasChars())
            GPUResource::SetName(name);
#endif
        device->AddResource(this);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResourceBase"/> class.
    /// </summary>
    virtual ~GPUResourceBase()
    {
        if (_device)
            _device->RemoveResource(this);
    }

public:
    /// <summary>
    /// Gets the graphics device.
    /// </summary>
    FORCE_INLINE DeviceType* GetDevice() const
    {
        return _device;
    }

public:
    // [GPUResource]
    void OnDeviceDispose() override
    {
        GPUResource::OnDeviceDispose();
        _device = nullptr;
    }
};

/// <summary>
/// Interface for GPU resources views. Shared base class for texture and buffer views.
/// </summary>
API_CLASS(Abstract, NoSpawn, Attributes="HideInEditor") class FLAXENGINE_API GPUResourceView : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUResourceView);
protected:
    static double DummyLastRenderTime;
    GPUResource* _parent = nullptr;

    explicit GPUResourceView(const SpawnParams& params)
        : ScriptingObject(params)
        , LastRenderTime(&DummyLastRenderTime)
    {
    }

public:
    // Points to the cache used by the resource for the resource visibility/usage detection. Written during rendering when resource view is used.
    double* LastRenderTime;

    /// <summary>
    /// Gets parent GPU resource owning that view.
    /// </summary>
    API_PROPERTY() FORCE_INLINE GPUResource* GetParent() const
    {
        return _parent;
    }

    /// <summary>
    /// Gets the native pointer to the underlying view. It's a platform-specific handle.
    /// </summary>
    virtual void* GetNativePtr() const = 0;
};
