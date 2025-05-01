// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Asset.h"

/// <summary>
/// Asset reference utility that doesn't add reference to that asset. Handles asset unload event.
/// </summary>
API_CLASS(InBuild) class WeakAssetReferenceBase
{
public:
    typedef Delegate<> EventType;

protected:
    Asset* _asset = nullptr;

public:
    /// <summary>
    /// The asset unloading event (should cleanup refs to it).
    /// </summary>
    EventType Unload;

public:
    NON_COPYABLE(WeakAssetReferenceBase);

    /// <summary>
    /// Initializes a new instance of the <see cref="WeakAssetReferenceBase"/> class.
    /// </summary>
    WeakAssetReferenceBase() = default;

    /// <summary>
    /// Finalizes an instance of the <see cref="WeakAssetReferenceBase"/> class.
    /// </summary>
    ~WeakAssetReferenceBase();

public:
    /// <summary>
    /// Gets the asset ID or Guid::Empty if not set.
    /// </summary>
    FORCE_INLINE Guid GetID() const
    {
        return _asset ? _asset->GetID() : Guid::Empty;
    }

    /// <summary>
    /// Gets managed instance object (or null if no asset set).
    /// </summary>
    FORCE_INLINE MObject* GetManagedInstance() const
    {
        return _asset ? _asset->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Gets the asset property value as string.
    /// </summary>
    String ToString() const;

protected:
    void OnSet(Asset* asset);
    void OnUnloaded(Asset* asset);
};

/// <summary>
/// Asset reference utility that doesn't add reference to that asset. Handles asset unload event.
/// </summary>
template<typename T>
API_CLASS(InBuild) class WeakAssetReference : public WeakAssetReferenceBase
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="WeakAssetReference"/> class.
    /// </summary>
    WeakAssetReference()
        : WeakAssetReferenceBase()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="WeakAssetReference"/> class.
    /// </summary>
    /// <param name="asset">The asset to set.</param>
    WeakAssetReference(T* asset)
        : WeakAssetReferenceBase()
    {
        OnSet(asset);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="WeakAssetReference"/> class.
    /// </summary>
    /// <param name="other">The other.</param>
    WeakAssetReference(const WeakAssetReference& other)
    {
        OnSet(other.Get());
    }

    WeakAssetReference(WeakAssetReference&& other)
    {
        OnSet(other.Get());
        other.OnSet(nullptr);
    }

    WeakAssetReference& operator=(WeakAssetReference&& other)
    {
        if (&other != this)
        {
            OnSet(other.Get());
            other.OnSet(nullptr);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="WeakAssetReference"/> class.
    /// </summary>
    ~WeakAssetReference()
    {
    }

public:
    FORCE_INLINE WeakAssetReference& operator=(const WeakAssetReference& other)
    {
        OnSet(other.Get());
        return *this;
    }

    FORCE_INLINE WeakAssetReference& operator=(T* other)
    {
        OnSet((Asset*)other);
        return *this;
    }

    FORCE_INLINE WeakAssetReference& operator=(const Guid& id)
    {
        OnSet((Asset*)::LoadAsset(id, T::TypeInitializer));
        return *this;
    }

    FORCE_INLINE bool operator==(T* other) const
    {
        return _asset == other;
    }

    FORCE_INLINE bool operator==(const WeakAssetReference& other) const
    {
        return _asset == other._asset;
    }

    /// <summary>
    /// Implicit conversion to the bool.
    /// </summary>
    FORCE_INLINE operator T*() const
    {
        return (T*)_asset;
    }

    /// <summary>
    /// Implicit conversion to the asset.
    /// </summary>
    FORCE_INLINE operator bool() const
    {
        return _asset != nullptr;
    }

    /// <summary>
    /// Implicit conversion to the asset.
    /// </summary>
    FORCE_INLINE T* operator->() const
    {
        return (T*)_asset;
    }

    /// <summary>
    /// Gets the asset.
    /// </summary>
    FORCE_INLINE T* Get() const
    {
        return (T*)_asset;
    }

    /// <summary>
    /// Gets the asset as a given type (static cast).
    /// </summary>
    template<typename U>
    FORCE_INLINE U* As() const
    {
        return (U*)_asset;
    }

public:
    /// <summary>
    /// Sets the asset reference.
    /// </summary>
    /// <param name="asset">The asset.</param>
    void Set(T* asset)
    {
        OnSet(asset);
    }
};

template<typename T>
uint32 GetHash(const WeakAssetReference<T>& key)
{
    return GetHash(key.GetID());
}
