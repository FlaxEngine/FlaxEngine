// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "NavigationTypes.h"
#include "Engine/Core/Config/Settings.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// The navigation system settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings", NoConstructor) class FLAXENGINE_API NavigationSettings : public SettingsBase
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NavigationSettings);
public:

    /// <summary>
    /// If checked, enables automatic navmesh actors spawning on a scenes that are using it during navigation building.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Navigation\")")
    bool AutoAddMissingNavMeshes = true;

    /// <summary>
    /// If checked, enables automatic navmesh actors removing from a scenes that are not using it during navigation building.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(110), EditorDisplay(\"Navigation\")")
    bool AutoRemoveMissingNavMeshes = true;

public:

    /// <summary>
    /// The height of a grid cell in the navigation mesh building steps using heightfields. A lower number means higher precision on the vertical axis but longer build times.
    /// </summary>
    API_FIELD(Attributes="Limit(1, 400), EditorOrder(210), EditorDisplay(\"Nav Mesh Options\")")
    float CellHeight = 10.0f;

    /// <summary>
    /// The width/height of a grid cell in the navigation mesh building steps using heightfields. A lower number means higher precision on the horizontal axes but longer build times.
    /// </summary>
    API_FIELD(Attributes="Limit(1, 400), EditorOrder(220), EditorDisplay(\"Nav Mesh Options\")")
    float CellSize = 30.0f;

    /// <summary>
    /// Tile size used for Navigation mesh tiles, the final size of a tile is CellSize*TileSize.
    /// </summary>
    API_FIELD(Attributes="Limit(8, 4096), EditorOrder(230), EditorDisplay(\"Nav Mesh Options\")")
    int32 TileSize = 64;

    /// <summary>
    /// The minimum number of cells allowed to form isolated island areas.
    /// </summary>
    API_FIELD(Attributes="Limit(0, 100), EditorOrder(240), EditorDisplay(\"Nav Mesh Options\")")
    int32 MinRegionArea = 0;

    /// <summary>
    /// Any regions with a span count smaller than this value will, if possible, be merged with larger regions.
    /// </summary>
    API_FIELD(Attributes="Limit(0, 100), EditorOrder(250), EditorDisplay(\"Nav Mesh Options\")")
    int32 MergeRegionArea = 20;

    /// <summary>
    /// The maximum allowed length for contour edges along the border of the mesh.
    /// </summary>
    API_FIELD(Attributes="Limit(100), EditorOrder(260), EditorDisplay(\"Nav Mesh Options\", \"Max Edge Length\")")
    float MaxEdgeLen = 1200.0f;

    /// <summary>
    /// The maximum distance a simplified contour's border edges should deviate from the original raw contour.
    /// </summary>
    API_FIELD(Attributes="Limit(0.1f, 4), EditorOrder(270), EditorDisplay(\"Nav Mesh Options\")")
    float MaxEdgeError = 1.3f;

    /// <summary>
    /// The sampling distance to use when generating the detail mesh.
    /// </summary>
    API_FIELD(Attributes="Limit(1), EditorOrder(280), EditorDisplay(\"Nav Mesh Options\", \"Detail Sampling Distance\")")
    float DetailSamplingDist = 600.0f;

    /// <summary>
    /// The maximum distance the detail mesh surface should deviate from heightfield data.
    /// </summary>
    API_FIELD(Attributes="Limit(0, 3), EditorOrder(290), EditorDisplay(\"Nav Mesh Options\")")
    float MaxDetailSamplingError = 1.0f;

public:

    /// <summary>
    /// The configuration for navmeshes.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1000), EditorDisplay(\"Agents\", EditorDisplayAttribute.InlineStyle)")
    Array<NavMeshProperties> NavMeshes;

    /// <summary>
    /// The configuration for nav areas.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(2000), EditorDisplay(\"Areas\", EditorDisplayAttribute.InlineStyle)")
    Array<NavAreaProperties> NavAreas;

public:

    NavigationSettings();

    /// <summary>
    /// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
    /// </summary>
    static NavigationSettings* Get();

    // [SettingsBase]
    void Apply() override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
};
