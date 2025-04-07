// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "GPUResource.h"

/// <summary>
/// GPU Resource container utility object.
/// </summary>
class FLAXENGINE_API GPUResourcePropertyBase
{
protected:
    GPUResource* _resource = nullptr;

public:
    NON_COPYABLE(GPUResourcePropertyBase);

    GPUResourcePropertyBase() = default;
    ~GPUResourcePropertyBase();

public:
    /// <summary>
    /// Action fired when resource gets released (reference gets cleared bu async tasks should stop execution).
    /// </summary>
    Action Released;

protected:
    void OnSet(GPUResource* resource);
    void OnReleased();
};

/// <summary>
/// GPU Resource container utility object.
/// </summary>
template<typename T = GPUResource>
class GPUResourceProperty : public GPUResourcePropertyBase
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    GPUResourceProperty()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    /// <param name="resource">The resource.</param>
    GPUResourceProperty(T* resource)
    {
        OnSet(resource);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    /// <param name="other">The other value.</param>
    GPUResourceProperty(const GPUResourceProperty& other)
    {
        OnSet(other.Get());
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    /// <param name="other">The other value.</param>
    GPUResourceProperty(GPUResourceProperty&& other)
    {
        OnSet(other.Get());
        other.OnSet(nullptr);
    }

    GPUResourceProperty& operator=(GPUResourceProperty&& other)
    {
        if (&other != this)
        {
            OnSet(other._resource);
            other.OnSet(nullptr);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    ~GPUResourceProperty()
    {
    }

public:
    FORCE_INLINE bool operator==(T* other) const
    {
        return Get() == other;
    }

    FORCE_INLINE bool operator==(const GPUResourceProperty& other) const
    {
        return Get() == other.Get();
    }

    FORCE_INLINE GPUResourceProperty& operator=(T& other)
    {
        OnSet(&other);
        return *this;
    }

    FORCE_INLINE GPUResourceProperty& operator=(T* other)
    {
        OnSet(other);
        return *this;
    }

    /// <summary>
    /// Implicit conversion to GPU Resource
    /// </summary>
    FORCE_INLINE operator T*() const
    {
        return (T*)_resource;
    }

    /// <summary>
    /// Implicit conversion to resource
    /// </summary>
    FORCE_INLINE operator bool() const
    {
        return _resource != nullptr;
    }

    /// <summary>
    /// Implicit conversion to resource
    /// </summary>
    FORCE_INLINE T* operator->() const
    {
        return (T*)_resource;
    }

    /// <summary>
    /// Gets linked resource
    /// </summary>
    FORCE_INLINE T* Get() const
    {
        return (T*)_resource;
    }

public:
    /// <summary>
    /// Set resource
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(T* value)
    {
        OnSet(value);
    }

    /// <summary>
    /// Unlink resource
    /// </summary>
    void Unlink()
    {
        OnSet(nullptr);
    }
};

typedef GPUResourceProperty<GPUResource> GPUResourceReference;
typedef GPUResourceProperty<class GPUTexture> GPUTextureReference;
typedef GPUResourceProperty<class GPUBuffer> BufferReference;
