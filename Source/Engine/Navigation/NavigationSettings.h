// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/Serialization.h"

/// <summary>
/// The navigation system settings container.
/// </summary>
/// <seealso cref="Settings{NavigationSettings}" />
class NavigationSettings : public Settings<NavigationSettings>
{
public:

    /// <summary>
    /// The height of a grid cell in the navigation mesh building steps using heightfields. 
    /// A lower number means higher precision on the vertical axis but longer build times.
    /// </summary>
    float CellHeight = 10.0f;

    /// <summary>
    /// The width/height of a grid cell in the navigation mesh building steps using heightfields. 
    /// A lower number means higher precision on the horizontal axes but longer build times.
    /// </summary>
    float CellSize = 30.0f;

    /// <summary>
    /// Tile size used for Navigation mesh tiles, the final size of a tile is CellSize*TileSize.
    /// </summary>
    int32 TileSize = 64;

    /// <summary>
    /// The minimum number of cells allowed to form isolated island areas.
    /// </summary>
    int32 MinRegionArea = 0;

    /// <summary>
    /// Any regions with a span count smaller than this value will, if possible, be merged with larger regions.
    /// </summary>
    int32 MergeRegionArea = 20;

    /// <summary>
    /// The maximum allowed length for contour edges along the border of the mesh.
    /// </summary>
    float MaxEdgeLen = 1200.0f;

    /// <summary>
    /// The maximum distance a simplified contour's border edges should deviate from the original raw contour.
    /// </summary>
    float MaxEdgeError = 1.3f;

    /// <summary>
    /// The sampling distance to use when generating the detail mesh.
    /// </summary>
    float DetailSamplingDist = 600.0f;

    /// <summary>
    /// The maximum distance the detail mesh surface should deviate from heightfield data.
    /// </summary>
    float MaxDetailSamplingError = 1.0f;

    /// <summary>
    /// The radius of the smallest objects to traverse this nav mesh. Objects can't pass through gaps of less than twice the radius.
    /// </summary>
    float WalkableRadius = 34.0f;

    /// <summary>
    /// The height of the smallest objects to traverse this nav mesh. Objects can't enter areas with ceilings lower than this value.
    /// </summary>
    float WalkableHeight = 144.0f;

    /// <summary>
    /// The maximum ledge height that is considered to still be traversable.
    /// </summary>
    float WalkableMaxClimb = 35.0f;

    /// <summary>
    /// The maximum slope that is considered walkable (in degrees). Objects can't go up or down slopes higher than this value.
    /// </summary>
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
