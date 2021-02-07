// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Asset.h"
#include "Engine/Serialization/ISerializable.h"
#include "Engine/Serialization/Json.h"

/// <summary>
/// Base class for all Json-format assets.
/// </summary>
/// <seealso cref="Asset" />
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API JsonAssetBase : public Asset
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(JsonAssetBase);
protected:

    String _path;

protected:

    /// <summary>
    /// Initializes a new instance of the <see cref="JsonAssetBase"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    /// <param name="info">The asset object information.</param>
    explicit JsonAssetBase(const SpawnParams& params, const AssetInfo* info);

public:

    /// <summary>
    /// The parsed json document.
    /// </summary>
    ISerializable::SerializeDocument Document;

    /// <summary>
    /// The data node (reference from Document).
    /// </summary>
    ISerializable::DeserializeStream* Data;

    /// <summary>
    /// The data type name from the header. Allows to recognize the data type.
    /// </summary>
    API_FIELD(ReadOnly) String DataTypeName;

    /// <summary>
    /// The serialized data engine build number. Can be used to convert/upgrade data between different formats across different engine versions.
    /// </summary>
    API_FIELD(ReadOnly) int32 DataEngineBuild;

    /// <summary>
    /// The Json data (as string).
    /// </summary>
    API_PROPERTY() String GetData() const;

public:

    // [Asset]
    const String& GetPath() const override;
#if USE_EDITOR
    void GetReferences(Array<Guid, HeapAllocation>& output) const override;
#endif

protected:

    // [Asset]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
#if USE_EDITOR
    void onRename(const StringView& newPath) override;
#endif
};

/// <summary>
/// Generic type of Json-format asset. It provides the managed representation of this resource data so it can be accessed via C# API.
/// </summary>
/// <seealso cref="JsonAssetBase" />
API_CLASS(NoSpawn) class FLAXENGINE_API JsonAsset : public JsonAssetBase
{
DECLARE_ASSET_HEADER(JsonAsset);

public:

    /// <summary>
    /// The deserialized unmanaged object instance (e.g. PhysicalMaterial).
    /// </summary>
    ISerializable* Instance;

protected:

    // [JsonAssetBase]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
};
