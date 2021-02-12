// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "GPUResource.h"

/// <summary>
/// GPU Resource container utility object.
/// </summary>
template<typename T = GPUResource>
class GPUResourceProperty
{
private:

    T* _resource;

private:

    // Disable copy actions
    GPUResourceProperty(const GPUResourceProperty& other) = delete;

public:

    /// <summary>
    /// Action fired when resource gets unloaded (reference gets cleared bu async tasks should stop execution).
    /// </summary>
    Delegate<GPUResourceProperty*> OnUnload;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    GPUResourceProperty()
        : _resource(nullptr)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    /// <param name="resource">The resource.</param>
    GPUResourceProperty(T* resource)
        : _resource(nullptr)
    {
        Set(resource);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResourceProperty"/> class.
    /// </summary>
    ~GPUResourceProperty()
    {
        // Check if object has been binded
        if (_resource)
        {
            // Unlink
            _resource->Releasing.template Unbind<GPUResourceProperty, &GPUResourceProperty::onResourceUnload>(this);
            _resource = nullptr;
        }
    }

public:

    FORCE_INLINE bool operator==(T* other) const
    {
        return Get() == other;
    }

    FORCE_INLINE bool operator==(GPUResourceProperty& other) const
    {
        return Get() == other.Get();
    }

    GPUResourceProperty& operator=(const GPUResourceProperty& other)
    {
        if (this != &other)
            Set(other.Get());
        return *this;
    }

    FORCE_INLINE GPUResourceProperty& operator=(T& other)
    {
        Set(&other);
        return *this;
    }

    FORCE_INLINE GPUResourceProperty& operator=(T* other)
    {
        Set(other);
        return *this;
    }

    /// <summary>
    /// Implicit conversion to GPU Resource
    /// </summary>
    /// <returns>Resource</returns>
    FORCE_INLINE operator T*() const
    {
        return _resource;
    }

    /// <summary>
    /// Implicit conversion to resource
    /// </summary>
    /// <returns>True if resource has been binded, otherwise false</returns>
    FORCE_INLINE operator bool() const
    {
        return _resource != nullptr;
    }

    /// <summary>
    /// Implicit conversion to resource
    /// </summary>
    /// <returns>Resource</returns>
    FORCE_INLINE T* operator->() const
    {
        return _resource;
    }

    /// <summary>
    /// Gets linked resource
    /// </summary>
    /// <returns>Resource</returns>
    FORCE_INLINE T* Get() const
    {
        return _resource;
    }

    /// <summary>
    /// Checks if resource has been binded
    /// </summary>
    /// <returns>True if resource has been binded, otherwise false</returns>
    FORCE_INLINE bool IsBinded() const
    {
        return _resource != nullptr;
    }

    /// <summary>
    /// Checks if resource is missing
    /// </summary>
    /// <returns>True if resource is missing, otherwise false</returns>
    FORCE_INLINE bool IsMissing() const
    {
        return _resource == nullptr;
    }

public:

    /// <summary>
    /// Set resource
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(T* value)
    {
        if (_resource != value)
        {
            // Remove reference from the old one
            if (_resource)
                _resource->Releasing.template Unbind<GPUResourceProperty, &GPUResourceProperty::onResourceUnload>(this);

            // Change referenced object
            _resource = value;

            // Add reference to the new one
            if (_resource)
                _resource->Releasing.template Bind<GPUResourceProperty, &GPUResourceProperty::onResourceUnload>(this);
        }
    }

    /// <summary>
    /// Unlink resource
    /// </summary>
    void Unlink()
    {
        if (_resource)
        {
            // Remove reference from the old one
            _resource->Releasing.template Unbind<GPUResourceProperty, &GPUResourceProperty::onResourceUnload>(this);
            _resource = nullptr;
        }
    }

private:

    void onResourceUnload()
    {
        if (_resource)
        {
            _resource = nullptr;
            OnUnload(this);
        }
    }
};

typedef GPUResourceProperty<GPUResource> GPUResourceReference;
typedef GPUResourceProperty<class GPUTexture> GPUTextureReference;
typedef GPUResourceProperty<class GPUBuffer> BufferReference;
