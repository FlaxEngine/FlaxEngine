// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Asset.h"
#include "Engine/Core/ISerializable.h"
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
    bool _isVirtualDocument = false;
    bool _isResaving = false;

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
    /// The data node (reference from Document or Document itself).
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
    /// The Json data (as string).
    /// </summary>
    API_PROPERTY() void SetData(const StringView& value);

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
    /// <param name="assets">The output list of object IDs references by the asset (appended, not cleared).</param>
    API_FUNCTION() static void GetReferences(const StringAnsiView& json, API_PARAM(Out) Array<Guid, HeapAllocation>& assets);

    /// <summary>
    /// Saves this asset to the Json Writer buffer (both ID, Typename header and Data contents). Supported only in Editor.
    /// </summary>
    /// <param name="writer">The output Json Writer to write asset.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    bool Save(JsonWriter& writer) const;
#endif

protected:
    // Gets the serialized Json data (from runtime state).
    virtual void OnGetData(rapidjson_flax::StringBuffer& buffer) const;

public:
    // [Asset]
    const String& GetPath() const override;
    uint64 GetMemoryUsage() const override;
#if USE_EDITOR
    void GetReferences(Array<Guid, HeapAllocation>& assets, Array<String, HeapAllocation>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [Asset]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
#if USE_EDITOR
    void onRename(const StringView& newPath) override;
    bool saveInternal(JsonWriter& writer) const;
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
    bool _isAfterReload = false;

public:
    /// <summary>
    /// The scripting type of the deserialized unmanaged object instance (e.g. PhysicalMaterial).
    /// </summary>
    ScriptingTypeHandle InstanceType;

    /// <summary>
    /// The deserialized unmanaged object instance (e.g. PhysicalMaterial). Might be null if asset was loaded before binary module with that asset was loaded (use GetInstance for this case).
    /// </summary>
    void* Instance;

    /// <summary>
    /// Gets the deserialized native object instance of the given type. Returns null if asset is not loaded or loaded object has different type.
    /// </summary>
    /// <returns>The asset instance object or null.</returns>
    template<typename T>
    T* GetInstance() const
    {
        const_cast<JsonAsset*>(this)->CreateInstance();
        const ScriptingTypeHandle& type = T::TypeInitializer;
        return Instance && type.IsAssignableFrom(InstanceType) ? (T*)Instance : nullptr;
    }

public:
    // [JsonAssetBase]
    uint64 GetMemoryUsage() const override;

protected:
    // [JsonAssetBase]
    void OnGetData(rapidjson_flax::StringBuffer& buffer) const override;
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
    void onLoaded_MainThread() override;

private:
    bool CreateInstance();
    void DeleteInstance();
#if USE_EDITOR
    void OnScriptsReloadStart();
    void OnScriptsReloaded();
#endif
};
