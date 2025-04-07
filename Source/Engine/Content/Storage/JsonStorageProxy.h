// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/Guid.h"

struct AssetInfo;

/// <summary>
/// Helper class for loading/saving Json file resources
/// </summary>
class FLAXENGINE_API JsonStorageProxy
{
public:
    /// <summary>
    /// Determines whether the specified extension can be a json resource file.
    /// </summary>
    /// <param name="extension">The path.</param>
    /// <returns>True if can be a json resource extension, otherwise false.</returns>
    static bool IsValidExtension(const StringView& extension);

    /// <summary>
    /// Find asset info by path
    /// </summary>
    /// <param name="path">Asset path</param>
    /// <param name="resultId">Asset ID</param>
    /// <param name="resultDataTypeName">Asset data TypeName</param>
    /// <returns>True if found any asset, otherwise false.</returns>
    static bool GetAssetInfo(const StringView& path, Guid& resultId, String& resultDataTypeName);

    /// <summary>
    /// Changes asset ID.
    /// </summary>
    /// <param name="path">Asset path</param>
    /// <param name="newId">Asset ID to set</param>
    /// <returns>True if found any asset, otherwise false.</returns>
    static bool ChangeId(const StringView& path, const Guid& newId);
};
