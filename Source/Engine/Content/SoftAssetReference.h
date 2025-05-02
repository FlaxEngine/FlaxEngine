// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Asset.h"

/// <summary>
/// The asset soft reference. Asset gets referenced (loaded) on actual use (ID reference is resolving it).
/// </summary>
class FLAXENGINE_API SoftAssetReferenceBase
{
protected:
    Asset* _asset = nullptr;
    Guid _id = Guid::Empty;

public:
    /// <summary>
    /// Action fired when field gets changed (link a new asset or change to the another value).
    /// </summary>
    Delegate<> Changed;

public:
    NON_COPYABLE(SoftAssetReferenceBase);

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftAssetReferenceBase"/> class.
    /// </summary>
    SoftAssetReferenceBase() = default;

    /// <summary>
    /// Finalizes an instance of the <see cref="SoftAssetReferenceBase"/> class.
    /// </summary>
    ~SoftAssetReferenceBase();

public:
    /// <summary>
    /// Gets the asset ID or Guid::Empty if not set.
    /// </summary>
    FORCE_INLINE Guid GetID() const
    {
        return _id;
    }

    /// <summary>
    /// Gets the asset property value as string.
    /// </summary>
    String ToString() const;

protected:
    void OnSet(Asset* asset);
    void OnSet(const Guid& id);
    void OnResolve(const ScriptingTypeHandle& type);
    void OnUnloaded(Asset* asset);
};

/// <summary>
/// The asset soft reference. Asset gets referenced (loaded) on actual use (ID reference is resolving it).
/// </summary>
template<typename T>
API_CLASS(InBuild) class SoftAssetReference : public SoftAssetReferenceBase
{
public:
    typedef T AssetType;
    typedef SoftAssetReference<T> Type;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SoftAssetReference"/> class.
    /// </summary>
    SoftAssetReference()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftAssetReference"/> class.
    /// </summary>
    /// <param name="asset">The asset to set.</param>
    SoftAssetReference(T* asset)
    {
        OnSet(asset);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="SoftAssetReference"/> class.
    /// </summary>
    /// <param name="other">The other.</param>
    SoftAssetReference(const SoftAssetReference& other)
    {
        OnSet(other.GetID());
    }

    SoftAssetReference(const Guid& id)
    {
        OnSet(id);
    }

    SoftAssetReference(SoftAssetReference&& other)
    {
        OnSet(other.GetID());
        other.OnSet(Guid::Empty);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="SoftAssetReference"/> class.
    /// </summary>
    ~SoftAssetReference()
    {
    }

public:
    FORCE_INLINE bool operator==(T* other) const
    {
        return Get() == other;
    }
    FORCE_INLINE bool operator==(const SoftAssetReference& other) const
    {
        return GetID() == other.GetID();
    }
    FORCE_INLINE bool operator==(const Guid& other) const
    {
        return GetID() == other;
    }
    FORCE_INLINE bool operator!=(T* other) const
    {
        return Get() != other;
    }
    FORCE_INLINE bool operator!=(const SoftAssetReference& other) const
    {
        return GetID() != other.GetID();
    }
    FORCE_INLINE bool operator!=(const Guid& other) const
    {
        return GetID() != other;
    }
    SoftAssetReference& operator=(const SoftAssetReference& other)
    {
        if (this != &other)
            OnSet(other.GetID());
        return *this;
    }
    SoftAssetReference& operator=(SoftAssetReference&& other)
    {
        if (this != &other)
        {
            OnSet(other.GetID());
            other.OnSet(nullptr);
        }
        return *this;
    }
    FORCE_INLINE SoftAssetReference& operator=(const T& other)
    {
        OnSet(&other);
        return *this;
    }
    FORCE_INLINE SoftAssetReference& operator=(T* other)
    {
        OnSet((Asset*)other);
        return *this;
    }
    FORCE_INLINE SoftAssetReference& operator=(const Guid& id)
    {
        OnSet(id);
        return *this;
    }
    FORCE_INLINE operator T*() const
    {
        return (T*)Get();
    }
    FORCE_INLINE operator bool() const
    {
        return Get() != nullptr;
    }
    FORCE_INLINE T* operator->() const
    {
        return (T*)Get();
    }
    template<typename U>
    FORCE_INLINE U* As() const
    {
        return static_cast<U*>(Get());
    }

public:
    /// <summary>
    /// Gets the asset (or null if unassigned).
    /// </summary>
    T* Get() const
    {
        if (!_asset)
            const_cast<SoftAssetReference*>(this)->OnResolve(T::TypeInitializer);
        return (T*)_asset;
    }

    /// <summary>
    /// Gets managed instance object (or null if no asset linked).
    /// </summary>
    MObject* GetManagedInstance() const
    {
        auto asset = Get();
        return asset ? asset->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Determines whether asset is assigned and managed instance of the asset is alive.
    /// </summary>
    bool HasManagedInstance() const
    {
        auto asset = Get();
        return asset && asset->HasManagedInstance();
    }

    /// <summary>
    /// Gets the managed instance object or creates it if missing or null if not assigned.
    /// </summary>
    MObject* GetOrCreateManagedInstance() const
    {
        auto asset = Get();
        return asset ? asset->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Sets the asset.
    /// </summary>
    /// <param name="id">The object ID. Uses Scripting to find the registered asset of the given ID.</param>
    FORCE_INLINE void Set(const Guid& id)
    {
        OnSet(id);
    }

    /// <summary>
    /// Sets the asset.
    /// </summary>
    /// <param name="asset">The asset.</param>
    FORCE_INLINE void Set(T* asset)
    {
        OnSet(asset);
    }
};

template<typename T>
uint32 GetHash(const SoftAssetReference<T>& key)
{
    return GetHash(key.GetID());
}
