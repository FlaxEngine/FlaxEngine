// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "NavMeshData.h"
#include "NavigationTypes.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/RawDataAsset.h"
#include "Engine/Level/Actor.h"

class NavMeshBoundsVolume;
class NavMeshRuntime;

/// <summary>
/// The navigation mesh actor that holds a navigation data for a scene.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Navigation/Nav Mesh\")") 
class FLAXENGINE_API NavMesh : public Actor
{
    DECLARE_SCENE_OBJECT(NavMesh);
public:
    /// <summary>
    /// The flag used to mark that navigation data has been modified since load. Used to save runtime data to the file on scene serialization.
    /// </summary>
    bool IsDataDirty;

    /// <summary>
    /// The navmesh tiles data.
    /// </summary>
    NavMeshData Data;

    /// <summary>
    /// The cached navmesh data asset.
    /// </summary>
    AssetReference<RawDataAsset> DataAsset;

#if USE_EDITOR

    /// <summary>
    /// If checked, the navmesh will be drawn in debug view when showing navigation data.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1), EditorDisplay(\"Nav Mesh\")") bool ShowDebugDraw = true;

#endif

    /// <summary>
    /// The navigation mesh properties.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(10), EditorDisplay(\"Nav Mesh\")") NavMeshProperties Properties;

public:
    /// <summary>
    /// Saves the nav mesh tiles data to the asset. Supported only in builds with assets saving enabled (eg. editor) and not during gameplay (eg. design time).
    /// </summary>
    void SaveNavMesh();

    /// <summary>
    /// Clears the data.
    /// </summary>
    void ClearData();

    /// <summary>
    /// Gets the navmesh runtime object that matches with properties.
    /// </summary>
    API_FUNCTION() NavMeshRuntime* GetRuntime(bool createIfMissing = true) const;

private:
    void AddTiles();
    void RemoveTiles();
    void OnDataAssetLoaded();

private:
    bool _navMeshActive = false;

public:
    // [Actor]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void Initialize() override;
};
