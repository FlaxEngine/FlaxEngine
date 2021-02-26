// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Content/AssetReference.h"
#include "Asset.h"

/// <summary>
/// Assets Container allows to load collection of assets and keep references to them.
/// </summary>
class AssetsContainer : public Array<AssetReference<Asset>>
{
public:

    /// <summary>
    /// Loads an asset.
    /// </summary>
    /// <param name="id">The asset id.</param>
    /// <returns>Loaded asset of null.</returns>
    template<typename T>
    T* LoadAsync(const Guid& id)
    {
        for (auto& e : *this)
        {
            if (e.GetID() == id)
                return (T*)e.Get();
        }
        auto asset = (T*)::LoadAsset(id, T::TypeInitializer);
        if (asset)
            Add(asset);
        return asset;
    }

    /// <summary>
    /// Release all referenced assets.
    /// </summary>
    void ReleaseAll()
    {
        Resize(0);
    }
};
