// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

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

    /// <summary>
    /// Initializes the virtual Json asset with custom data.
    /// </summary>
    /// <remarks>Can be used only for virtual assets created at runtime.</remarks>
    /// <param name="dataTypeName">The data type name from the header. Allows to recognize the data type.</param>
    /// <param name="dataJson">The Json with serialized data.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool Init(const StringView& dataTypeName, const StringAnsiView& dataJson);

#if USE_EDITOR
    /// <summary>
    /// Parses Json string to find any object references inside it. It can produce list of references to assets and/or scene objects. Supported only in Editor.
    /// </summary>
    /// <param name="json">The Json string.</param>
    /// <param name="output">The output list of object IDs references by the asset (appended, not cleared).</param>
    API_FUNCTION() static void GetReferences(const StringAnsiView& json, API_PARAM(Out) Array<Guid, HeapAllocation>& output);
#endif

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
private:
    ScriptingType::Dtor _dtor;

public:
    /// <summary>
    /// The scripting type of the deserialized unmanaged object instance (e.g. PhysicalMaterial).
    /// </summary>
    ScriptingTypeHandle InstanceType;

    /// <summary>
    /// The deserialized unmanaged object instance (e.g. PhysicalMaterial).
    /// </summary>
    void* Instance;

    /// <summary>
    /// Gets the deserialized native object instance of the given type. Returns null if asset is not loaded or loaded object has different type.
    /// </summary>
    /// <returns>The asset instance object or null.</returns>
    template<typename T>
    T* GetInstance() const
    {
        return Instance && InstanceType.IsAssignableFrom(T::TypeInitializer) ? (T*)Instance : nullptr;
    }

protected:
    // [JsonAssetBase]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;

private:
    bool CreateInstance();
    void DeleteInstance();
#if USE_EDITOR
    void OnScriptsReloadStart();
    void OnScriptsReloaded();
#endif
};
