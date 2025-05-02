// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"

/// <summary>
/// Json asset reference utility. References resource with a typed data type.
/// </summary>
/// <typeparam name="T">Type of the asset instance type.</typeparam>
template<typename T>
API_STRUCT(NoDefault, Template, MarshalAs=JsonAsset*) struct JsonAssetReference : AssetReference<JsonAsset>
{
    JsonAssetReference() = default;

    JsonAssetReference(JsonAsset* asset)
    {
        OnSet(asset);
    }

    /// <summary>
    /// Gets the deserialized native object instance of the given type. Returns null if asset is not loaded or loaded object has different type.
    /// </summary>
    /// <returns>The asset instance object or null.</returns>
    FORCE_INLINE T* GetInstance() const
    {
        return _asset ? Get()->template GetInstance<T>() : nullptr;
    }

    JsonAssetReference& operator=(JsonAsset* asset) noexcept
    {
        OnSet(asset);
        return *this;
    }

    operator JsonAsset*() const
    {
        return Get();
    }
};
