// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Asset.h"

/// <summary>
/// Asset reference utility. Keeps reference to the linked asset object and handles load/unload events.
/// </summary>
class FLAXENGINE_API AssetReferenceBase
{
protected:
    Asset* _asset = nullptr;

public:
    /// <summary>
    /// The asset loaded event (fired when asset gets loaded or is already loaded after change).
    /// </summary>
    Action Loaded;

    /// <summary>
    /// The asset unloading event (should cleanup refs to it).
    /// </summary>
    Action Unload;

    /// <summary>
    /// Action fired when field gets changed (link a new asset or change to the another value).
    /// </summary>
    Action Changed;

public:
    NON_COPYABLE(AssetReferenceBase);

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetReferenceBase"/> class.
    /// </summary>
    AssetReferenceBase() = default;

    /// <summary>
    /// Finalizes an instance of the <see cref="AssetReferenceBase"/> class.
    /// </summary>
    ~AssetReferenceBase();

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
    void OnLoaded(Asset* asset);
    void OnUnloaded(Asset* asset);
};

/// <summary>
/// Asset reference utility. Keeps reference to the linked asset object and handles load/unload events.
/// </summary>
template<typename T>
API_CLASS(InBuild) class AssetReference : public AssetReferenceBase
{
public:
    typedef T AssetType;
    typedef AssetReference<T> Type;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AssetReference"/> class.
    /// </summary>
    AssetReference()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetReference"/> class.
    /// </summary>
    /// <param name="asset">The asset to set.</param>
    AssetReference(T* asset)
    {
        OnSet((Asset*)asset);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetReference"/> class.
    /// </summary>
    /// <param name="other">The other.</param>
    AssetReference(const AssetReference& other)
    {
        OnSet(other._asset);
    }

    AssetReference(AssetReference&& other) noexcept
    {
        OnSet(other._asset);
        other.OnSet(nullptr);
    }

    AssetReference& operator=(AssetReference&& other)
    {
        if (&other != this)
        {
            OnSet(other._asset);
            other.OnSet(nullptr);
        }
        return *this;
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="AssetReference"/> class.
    /// </summary>
    ~AssetReference()
    {
    }

public:
    FORCE_INLINE AssetReference& operator=(const AssetReference& other)
    {
        OnSet(other._asset);
        return *this;
    }

    FORCE_INLINE AssetReference& operator=(T* other)
    {
        OnSet((Asset*)other);
        return *this;
    }

    FORCE_INLINE AssetReference& operator=(const Guid& id)
    {
        OnSet(::LoadAsset(id, T::TypeInitializer));
        return *this;
    }

    FORCE_INLINE bool operator==(T* other) const
    {
        return _asset == other;
    }

    FORCE_INLINE bool operator==(const AssetReference& other) const
    {
        return _asset == other._asset;
    }

    FORCE_INLINE bool operator!=(T* other) const
    {
        return _asset != other;
    }

    FORCE_INLINE bool operator!=(const AssetReference& other) const
    {
        return _asset != other._asset;
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
        OnSet((Asset*)asset);
    }
};

template<typename T>
uint32 GetHash(const AssetReference<T>& key)
{
    return GetHash(key.GetID());
}
