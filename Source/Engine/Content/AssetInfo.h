// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Guid.h"

/// <summary>
/// Contains short information about an asset.
/// </summary>
API_STRUCT() struct AssetInfo
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AssetInfo);

    /// <summary>
    /// Unique ID.
    /// </summary>
    API_FIELD() Guid ID;

    /// <summary>
    /// The stored data full typename. Used to recognize asset type.
    /// </summary>
    API_FIELD() String TypeName;

    /// <summary>
    /// Cached path.
    /// </summary>
    API_FIELD() String Path;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AssetInfo"/> struct.
    /// </summary>
    AssetInfo()
    {
        ID = Guid::Empty;
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetInfo"/> struct.
    /// </summary>
    /// <param name="id">The other info.</param>
    AssetInfo(const AssetInfo& other)
        : ID(other.ID)
        , TypeName(other.TypeName)
        , Path(other.Path)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetInfo"/> struct.
    /// </summary>
    /// <param name="id">The other info.</param>
    AssetInfo(AssetInfo&& other) noexcept
        : ID(other.ID)
        , TypeName(MoveTemp(other.TypeName))
        , Path(MoveTemp(other.Path))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetInfo"/> struct.
    /// </summary>
    /// <param name="id">The identifier.</param>
    /// <param name="typeName">The typename identifier.</param>
    /// <param name="path">The path.</param>
    AssetInfo(const Guid& id, String&& typeName, String&& path)
        : ID(id)
        , TypeName(MoveTemp(typeName))
        , Path(MoveTemp(path))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="AssetInfo"/> struct.
    /// </summary>
    /// <param name="id">The identifier.</param>
    /// <param name="typeName">The typename identifier.</param>
    /// <param name="path">The path.</param>
    AssetInfo(const Guid& id, const StringView& typeName, const StringView& path)
        : ID(id)
        , TypeName(typeName)
        , Path(path)
    {
    }

    AssetInfo& operator=(const AssetInfo& other) noexcept
    {
        if (this != &other)
        {
            ID = other.ID;
            TypeName = other.TypeName;
            Path = other.Path;
        }
        return *this;
    }

    AssetInfo& operator=(AssetInfo&& other) noexcept
    {
        if (this != &other)
        {
            ID = other.ID;
            TypeName = MoveTemp(other.TypeName);
            Path = MoveTemp(other.Path);
        }
        return *this;
    }

public:
    /// <summary>
    /// Gets the string.
    /// </summary>
    /// <returns>The string.</returns>
    String ToString() const;
};
