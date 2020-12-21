// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "NavigationTypes.h"
#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"

/// <summary>
/// The navigation system settings container.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEditor.Content.Settings") class FLAXENGINE_API NavigationSettings : public SettingsBase<NavigationSettings>
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NavigationSettings);
public:

    /// <summary>
    /// The height of a grid cell in the navigation mesh building steps using heightfields. A lower number means higher precision on the vertical axis but longer build times.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(10.0f), Limit(1, 400), EditorOrder(10), EditorDisplay(\"Nav Mesh Options\")")
    float CellHeight = 10.0f;

    /// <summary>
    /// The width/height of a grid cell in the navigation mesh building steps using heightfields. A lower number means higher precision on the horizontal axes but longer build times.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(30.0f), Limit(1, 400), EditorOrder(20), EditorDisplay(\"Nav Mesh Options\")")
    float CellSize = 30.0f;

    /// <summary>
    /// Tile size used for Navigation mesh tiles, the final size of a tile is CellSize*TileSize.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(64), Limit(8, 4096), EditorOrder(30), EditorDisplay(\"Nav Mesh Options\")")
    int32 TileSize = 64;

    /// <summary>
    /// The minimum number of cells allowed to form isolated island areas.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(0), Limit(0, 100), EditorOrder(40), EditorDisplay(\"Nav Mesh Options\")")
    int32 MinRegionArea = 0;

    /// <summary>
    /// Any regions with a span count smaller than this value will, if possible, be merged with larger regions.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(20), Limit(0, 100), EditorOrder(50), EditorDisplay(\"Nav Mesh Options\")")
    int32 MergeRegionArea = 20;

    /// <summary>
    /// The maximum allowed length for contour edges along the border of the mesh.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1200.0f), Limit(100), EditorOrder(60), EditorDisplay(\"Nav Mesh Options\", \"Max Edge Length\")")
    float MaxEdgeLen = 1200.0f;

    /// <summary>
    /// The maximum distance a simplified contour's border edges should deviate from the original raw contour.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.3f), Limit(0.1f, 4), EditorOrder(70), EditorDisplay(\"Nav Mesh Options\")")
    float MaxEdgeError = 1.3f;

    /// <summary>
    /// The sampling distance to use when generating the detail mesh.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(600.0f), Limit(1), EditorOrder(80), EditorDisplay(\"Nav Mesh Options\", \"Detail Sampling Distance\")")
    float DetailSamplingDist = 600.0f;

    /// <summary>
    /// The maximum distance the detail mesh surface should deviate from heightfield data.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(1.0f), Limit(0, 3), EditorOrder(90), EditorDisplay(\"Nav Mesh Options\")")
    float MaxDetailSamplingError = 1.0f;

    /// <summary>
    /// The radius of the smallest objects to traverse this nav mesh. Objects can't pass through gaps of less than twice the radius.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(34.0f), Limit(0), EditorOrder(1000), EditorDisplay(\"Agent Options\")")
    float WalkableRadius = 34.0f;

    /// <summary>
    /// The height of the smallest objects to traverse this nav mesh. Objects can't enter areas with ceilings lower than this value.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(144.0f), Limit(0), EditorOrder(1010), EditorDisplay(\"Agent Options\")")
    float WalkableHeight = 144.0f;

    /// <summary>
    /// The maximum ledge height that is considered to still be traversable.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(35.0f), Limit(0), EditorOrder(1020), EditorDisplay(\"Agent Options\")")
    float WalkableMaxClimb = 35.0f;

    /// <summary>
    /// The maximum slope that is considered walkable (in degrees). Objects can't go up or down slopes higher than this value.
    /// </summary>
    API_FIELD(Attributes="DefaultValue(60.0f), Limit(0, 89.0f), EditorOrder(1030), EditorDisplay(\"Agent Options\")")
    float WalkableMaxSlopeAngle = 60.0f;

public:

    // [SettingsBase]
    void RestoreDefault() final override
    {
        CellHeight = 10.0f;
        CellSize = 30.0f;
        TileSize = 64;
        MinRegionArea = 0;
        MergeRegionArea = 20;
        MaxEdgeLen = 1200.0f;
        MaxEdgeError = 1.3f;
        DetailSamplingDist = 600.0f;
        MaxDetailSamplingError = 1.0f;
        WalkableRadius = 34.0f;
        WalkableHeight = 144.0f;
        WalkableMaxClimb = 35.0f;
        WalkableMaxSlopeAngle = 60.0f;
    }

    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override
    {
        DESERIALIZE(CellHeight);
        DESERIALIZE(CellSize);
        DESERIALIZE(TileSize);
        DESERIALIZE(MinRegionArea);
        DESERIALIZE(MergeRegionArea);
        DESERIALIZE(MaxEdgeLen);
        DESERIALIZE(MaxEdgeError);
        DESERIALIZE(DetailSamplingDist);
        DESERIALIZE(MaxDetailSamplingError);
        DESERIALIZE(WalkableRadius);
        DESERIALIZE(WalkableHeight);
        DESERIALIZE(WalkableMaxClimb);
        DESERIALIZE(WalkableMaxSlopeAngle);
    }
};
