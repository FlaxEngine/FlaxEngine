// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Asset.h"

/// <summary>
/// Asset reference utility. Keeps reference to the linked asset object and handles load/unload events.
/// </summary>
class FLAXENGINE_API AssetReferenceBase
{
public:

    typedef Delegate<> EventType;

protected:

    Asset* _asset;

public:

    /// <summary>
    /// The asset loaded event (fired when asset gets loaded or is already loaded after change).
    /// </summary>
    EventType Loaded;

    /// <summary>
    /// The asset unloading event (should cleanup refs to it).
    /// </summary>
    EventType Unload;

    /// <summary>
    /// Action fired when field gets changed (link a new asset or change to the another value).
    /// </summary>
    EventType Changed;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetReferenceBase"/> class.
    /// </summary>
    AssetReferenceBase()
        : _asset(nullptr)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="AssetReferenceBase"/> class.
    /// </summary>
    ~AssetReferenceBase()
    {
        if (_asset)
        {
            _asset->OnLoaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnAssetLoaded>(this);
            _asset->OnUnloaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnAssetUnloaded>(this);
            _asset->RemoveReference();
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
            {
                e->OnLoaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnAssetLoaded>(this);
                e->OnUnloaded.Unbind<AssetReferenceBase, &AssetReferenceBase::OnAssetUnloaded>(this);
                e->RemoveReference();
            }
            _asset = e = asset;
            if (e)
            {
                e->AddReference();
                e->OnLoaded.Bind<AssetReferenceBase, &AssetReferenceBase::OnAssetLoaded>(this);
                e->OnUnloaded.Bind<AssetReferenceBase, &AssetReferenceBase::OnAssetUnloaded>(this);
            }
            Changed();
            if (e && e->IsLoaded())
                Loaded();
        }
    }

    void OnAssetLoaded(Asset* asset)
    {
        if (_asset == asset)
        {
            Loaded();
        }
    }

    void OnAssetUnloaded(Asset* asset)
    {
        if (_asset == asset)
        {
            Unload();
            OnSet(nullptr);
        }
    }
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
        OnSet(asset);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetReference"/> class.
    /// </summary>
    /// <param name="other">The other.</param>
    AssetReference(const AssetReference& other)
    {
        OnSet(other.Get());
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
        OnSet(other.Get());
        return *this;
    }

    FORCE_INLINE AssetReference& operator=(T* other)
    {
        OnSet(other);
        return *this;
    }

    FORCE_INLINE AssetReference& operator=(const Guid& id)
    {
        OnSet((T*)::LoadAsset(id, T::TypeInitializer));
        return *this;
    }

    FORCE_INLINE bool operator==(T* other)
    {
        return _asset == other;
    }

    FORCE_INLINE bool operator==(const AssetReference& other)
    {
        return _asset == other._asset;
    }

    FORCE_INLINE bool operator!=(T* other)
    {
        return _asset != other;
    }

    FORCE_INLINE bool operator!=(const AssetReference& other)
    {
        return _asset != other._asset;
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
