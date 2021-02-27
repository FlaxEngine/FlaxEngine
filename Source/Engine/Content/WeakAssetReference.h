// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

    Asset* _asset;

public:

    /// <summary>
    /// The asset unloading event (should cleanup refs to it).
    /// </summary>
    EventType Unload;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="WeakAssetReferenceBase"/> class.
    /// </summary>
    WeakAssetReferenceBase()
        : _asset(nullptr)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="WeakAssetReferenceBase"/> class.
    /// </summary>
    ~WeakAssetReferenceBase()
    {
        if (_asset)
        {
            _asset->OnUnloaded.Unbind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnAssetUnloaded>(this);
            _asset = nullptr;
        }
    }

public:

    /// <summary>
    /// Gets the asset ID or Guid::Empty if not set.
    /// </summary>
    /// <returns>The asset ID or Guid::Empty if not set.</returns>
    FORCE_INLINE Guid GetID() const
    {
        return _asset ? _asset->GetID() : Guid::Empty;
    }

    /// <summary>
    /// Gets managed instance object (or null if no asset set).
    /// </summary>
    /// <returns>Mono managed object</returns>
    FORCE_INLINE MonoObject* GetManagedInstance() const
    {
        return _asset ? _asset->GetOrCreateManagedInstance() : nullptr;
    }

    /// <summary>
    /// Gets the asset property value as string.
    /// </summary>
    /// <returns>The string.</returns>
    String ToString() const
    {
        static String NullStr = TEXT("<null>");
        return _asset ? _asset->ToString() : NullStr;
    }

protected:

    void OnSet(Asset* asset)
    {
        auto e = _asset;
        if (e != asset)
        {
            if (e)
                e->OnUnloaded.Unbind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnAssetUnloaded>(this);
            _asset = e = asset;
            if (e)
                e->OnUnloaded.Bind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnAssetUnloaded>(this);
        }
    }

    void OnAssetUnloaded(Asset* asset)
    {
        ASSERT(_asset == asset);
        Unload();
        asset->OnUnloaded.Unbind<WeakAssetReferenceBase, &WeakAssetReferenceBase::OnAssetUnloaded>(this);
        asset = nullptr;
    }
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
        OnSet(other);
        return *this;
    }

    FORCE_INLINE WeakAssetReference& operator=(const Guid& id)
    {
        OnSet((T*)::LoadAsset(id, T::TypeInitializer));
        return *this;
    }

    FORCE_INLINE bool operator==(T* other)
    {
        return _asset == other;
    }

    FORCE_INLINE bool operator==(const WeakAssetReference& other)
    {
        return _asset == other._asset;
    }

    /// <summary>
    /// Implicit conversion to the bool.
    /// </summary>
    /// <returns>The asset.</returns>
    FORCE_INLINE operator T*() const
    {
        return (T*)_asset;
    }

    /// <summary>
    /// Implicit conversion to the asset.
    /// </summary>
    /// <returns>True if asset has been set, otherwise false.</returns>
    FORCE_INLINE operator bool() const
    {
        return _asset != nullptr;
    }

    /// <summary>
    /// Implicit conversion to the asset.
    /// </summary>
    /// <returns>The asset.</returns>
    FORCE_INLINE T* operator->() const
    {
        return (T*)_asset;
    }

    /// <summary>
    /// Gets the asset.
    /// </summary>
    /// <returns>The asset.</returns>
    FORCE_INLINE T* Get() const
    {
        return (T*)_asset;
    }

    /// <summary>
    /// Gets the asset as a given type (static cast).
    /// </summary>
    /// <returns>The asset.</returns>
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
