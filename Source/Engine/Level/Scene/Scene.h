// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../Actor.h"
#include "../SceneInfo.h"
#include "Engine/Content/JsonAsset.h"
#include "SceneLightmapsData.h"
#include "SceneCSGData.h"
#include "SceneRendering.h"
#include "SceneTicking.h"

class MeshCollider;
class Level;
class NavigationScene;
class ReloadScriptsAction;

/// <summary>
/// The scene root object that contains a hierarchy of actors.
/// </summary>
API_CLASS() class FLAXENGINE_API Scene final : public Actor
{
DECLARE_SCENE_OBJECT(Scene);
    friend Level;
    friend ReloadScriptsAction;
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="Scene"/> class.
    /// </summary>
    ~Scene();

public:

    /// <summary>
    /// The scene metadata.
    /// </summary>
    SceneInfo Info;

    /// <summary>
    /// The last load time.
    /// </summary>
    DateTime LoadTime;

    /// <summary>
    /// The last save time.
    /// </summary>
    DateTime SaveTime;

public:

    /// <summary>
    /// The scene rendering manager.
    /// </summary>
    SceneRendering Rendering;

    /// <summary>
    /// The scene ticking manager.
    /// </summary>
    SceneTicking Ticking;

    /// <summary>
    /// The static light manager for this scene.
    /// </summary>
    SceneLightmapsData LightmapsData;

    /// <summary>
    /// The CSG data container for this scene.
    /// </summary>
    CSG::SceneCSGData CSGData;

    /// <summary>
    /// The navigation scene (always valid).
    /// </summary>
    NavigationScene* Navigation;

    /// <summary>
    /// Gets the lightmap settings (per scene).
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"Lightmap Settings\", EditorDisplayAttribute.InlineStyle)")
    LightmapSettings GetLightmapSettings() const;

    /// <summary>
    /// Sets the lightmap settings (per scene).
    /// </summary>
    API_PROPERTY() void SetLightmapSettings(const LightmapSettings& value);

public:

    /// <summary>
    /// Removes all baked lightmap textures from the scene.
    /// </summary>
    API_FUNCTION() void ClearLightmaps();

    /// <summary>
    /// Builds the CSG geometry for the given scene.
    /// </summary>
    /// <remarks>Requests are enqueued till the next game scripts update.</remarks>
    /// <param name="timeoutMs">The timeout to wait before building CSG (in milliseconds).</param>
    API_FUNCTION() void BuildCSG(float timeoutMs = 50);

public:

#if USE_EDITOR

    /// <summary>
    /// Gets path to the scene file
    /// </summary>
    API_PROPERTY() String GetPath() const;

    /// <summary>
    /// Gets filename of the scene file
    /// </summary>
    API_PROPERTY() String GetFilename() const;

    /// <summary>
    /// Gets path to the scene data folder
    /// </summary>
    API_PROPERTY() String GetDataFolderPath() const;

#endif

private:

    MeshCollider* TryGetCsgCollider();
    StaticModel* TryGetCsgModel();
    void CreateCsgCollider();
    void CreateCsgModel();
    void OnCsgCollisionDataChanged();
    void OnCsgModelChanged();
#if COMPILE_WITH_CSG_BUILDER
    void OnCSGBuildEnd()
    {
        if (CSGData.CollisionData && TryGetCsgCollider() == nullptr)
            CreateCsgCollider();
        if (CSGData.Model && TryGetCsgModel() == nullptr)
            CreateCsgModel();
    }
#endif

public:

    // [Actor]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnDeleteObject() override;
    void EndPlay() override;

protected:

    // [Scene]
    void PostLoad() override;
    void PostSpawn() override;
    void BeginPlay(SceneBeginData* data) override;
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};

/// <summary>
/// The scene asset.
/// </summary>
API_CLASS(NoSpawn) class SceneAsset : public JsonAsset
{
DECLARE_ASSET_HEADER(SceneAsset);
protected:
    bool IsInternalType() const override
    {
        return true;
    }
};
