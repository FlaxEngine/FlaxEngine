// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
