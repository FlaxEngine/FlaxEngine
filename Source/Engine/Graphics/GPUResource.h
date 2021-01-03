// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Common.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Config.h"

/// <summary>
/// Releases GPU resource memory and deletes object.
/// </summary>
#define SAFE_DELETE_GPU_RESOURCE(x) if (x) { (x)->DeleteObjectNow(); (x) = nullptr; }

/// <summary>
/// The base class for all GPU resources.
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API GPUResource : public PersistentScriptingObject, public NonCopyable
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUResource);
public:

    /// <summary>
    /// GPU Resources types.
    /// </summary>
    DECLARE_ENUM_9(ResourceType, RenderTarget, Texture, CubeTexture, VolumeTexture, Buffer, Shader, PipelineState, Descriptor, Query);

    /// <summary>
    /// GPU Resources object types. Used to detect Texture objects from subset of Types:  RenderTarget, Texture, CubeTexture, VolumeTexture which use the same API object.
    /// </summary>
    DECLARE_ENUM_3(ObjectType, Texture, Buffer, Other);

protected:

    uint64 _memoryUsage = 0;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResource"/> class.
    /// </summary>
    GPUResource()
        : PersistentScriptingObject(SpawnParams(Guid::New(), GPUResource::TypeInitializer))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResource"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    GPUResource(const SpawnParams& params)
        : PersistentScriptingObject(params)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResource"/> class.
    /// </summary>
    virtual ~GPUResource()
    {
#if !BUILD_RELEASE
        ASSERT(_memoryUsage == 0);
#endif
    }

public:

    /// <summary>
    /// Action fired when resource GPU state gets released. All objects and async tasks using this resource should release references to this object nor use its data.
    /// </summary>
    Action Releasing;

public:

    /// <summary>
    /// Gets the resource type.
    /// </summary>
    /// <returns>The type.</returns>
    virtual ResourceType GetResourceType() const = 0;

    /// <summary>
    /// Gets resource object type.
    /// </summary>
    /// <returns>The object type.</returns>
    virtual ObjectType GetObjectType() const
    {
        return ObjectType::Other;
    }

    /// <summary>
    /// Gets amount of GPU memory used by this resource (in bytes).
    /// It's a rough estimation. GPU memory may be fragmented, compressed or sub-allocated so the actual memory pressure from this resource may vary (also depends on the current graphics backend).
    /// </summary>
    /// <returns>The current memory usage.</returns>
    API_PROPERTY() FORCE_INLINE uint64 GetMemoryUsage() const
    {
        return _memoryUsage;
    }

#if GPU_ENABLE_RESOURCE_NAMING

    /// <summary>
    /// Gets the resource name.
    /// </summary>
    /// <returns>The name of the resource.</returns>
    virtual String GetName() const
    {
        return String::Empty;
    }

#endif

    /// <summary>
    /// Releases GPU resource data.
    /// </summary>
    API_FUNCTION() void ReleaseGPU()
    {
        if (_memoryUsage != 0)
        {
            Releasing();
            OnReleaseGPU();
            _memoryUsage = 0;
        }
    }

    /// <summary>
    /// Action called when GPU device is disposing.
    /// </summary>
    virtual void OnDeviceDispose()
    {
        // By default we want to release resource data but keep it alive
        ReleaseGPU();
    }

protected:

    /// <summary>
    /// Releases GPU resource data (implementation).
    /// </summary>
    virtual void OnReleaseGPU()
    {
    }

public:

    // [PersistentScriptingObject]
    String ToString() const override
    {
#if GPU_ENABLE_RESOURCE_NAMING
        return GetName();
#else
		return TEXT("GPU Resource");
#endif
    }

    void OnDeleteObject() override
    {
        ReleaseGPU();

        PersistentScriptingObject::OnDeleteObject();
    }
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

private:

#if GPU_ENABLE_RESOURCE_NAMING
    String _name;
#endif

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceBase"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUResourceBase(DeviceType* device, const StringView& name) noexcept
        : _device(device)
#if GPU_ENABLE_RESOURCE_NAMING
        , _name(name.Get(), name.Length())
#endif
    {
        ASSERT(device);

        // Register
        device->Resources.Add(this);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResourceBase"/> class.
    /// </summary>
    virtual ~GPUResourceBase()
    {
        // Unregister
        if (_device)
            _device->Resources.Remove(this);
    }

public:

    /// <summary>
    /// Gets the graphics device.
    /// </summary>
    /// <returns>The device.</returns>
    FORCE_INLINE DeviceType* GetDevice() const
    {
        return _device;
    }

public:

    // [GPUResource]
#if GPU_ENABLE_RESOURCE_NAMING
    String GetName() const override
    {
        return _name;
    }
#endif
    void OnDeviceDispose() override
    {
        // Base
        GPUResource::OnDeviceDispose();

        // Unlink device handle
        _device = nullptr;
    }
};

/// <summary>
/// Interface for GPU resources views. Shared base class for texture and buffer views.
/// </summary>
API_CLASS(Abstract, NoSpawn, Attributes="HideInEditor") class FLAXENGINE_API GPUResourceView : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(GPUResourceView);
protected:

    explicit GPUResourceView(const SpawnParams& params)
        : PersistentScriptingObject(params)
    {
    }

public:

    /// <summary>
    /// Gets the native pointer to the underlying view. It's a platform-specific handle.
    /// </summary>
    /// <returns>The pointer.</returns>
    virtual void* GetNativePtr() const = 0;
};
