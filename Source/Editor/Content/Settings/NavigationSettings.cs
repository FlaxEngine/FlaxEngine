// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The navigation system settings container.
    /// </summary>
    public sealed class NavigationSettings : SettingsBase
    {
        /// <summary>
        /// The height of a grid cell in the navigation mesh building steps using heightfields. 
        /// A lower number means higher precision on the vertical axis but longer build times.
        /// </summary>
        [DefaultValue(10.0f), Limit(1, 400)]
        [EditorOrder(10), EditorDisplay("Nav Mesh Options"), Tooltip("The height of a grid cell in the navigation mesh building steps using heightfields. A lower number means higher precision on the vertical axis but longer build times.")]
        public float CellHeight = 10.0f;

        /// <summary>
        /// The width/height of a grid cell in the navigation mesh building steps using heightfields. 
        /// A lower number means higher precision on the horizontal axes but longer build times.
        /// </summary>
        [DefaultValue(30.0f), Limit(1, 400)]
        [EditorOrder(20), EditorDisplay("Nav Mesh Options"), Tooltip("The width/height of a grid cell in the navigation mesh building steps using heightfields. A lower number means higher precision on the vertical axis but longer build times.")]
        public float CellSize = 30.0f;

        /// <summary>
        /// Tile size used for Navigation mesh tiles, the final size of a tile is CellSize*TileSize.
        /// </summary>
        [DefaultValue(64), Limit(8, 4096)]
        [EditorOrder(30), EditorDisplay("Nav Mesh Options"), Tooltip("Tile size used for Navigation mesh tiles, the final size of a tile is CellSize*TileSize.")]
        public int TileSize = 64;

        /// <summary>
        /// The minimum number of cells allowed to form isolated island areas.
        /// </summary>
        [DefaultValue(0), Limit(0, 100)]
        [EditorOrder(40), EditorDisplay("Nav Mesh Options"), Tooltip("The minimum number of cells allowed to form isolated island areas.")]
        public int MinRegionArea = 0;

        /// <summary>
        /// Any regions with a span count smaller than this value will, if possible, be merged with larger regions.
        /// </summary>
        [DefaultValue(20), Limit(0, 100)]
        [EditorOrder(50), EditorDisplay("Nav Mesh Options"), Tooltip("Any regions with a span count smaller than this value will, if possible, be merged with larger regions.")]
        public int MergeRegionArea = 20;

        /// <summary>
        /// The maximum allowed length for contour edges along the border of the mesh.
        /// </summary>
        [DefaultValue(1200.0f), Limit(100)]
        [EditorOrder(60), EditorDisplay("Nav Mesh Options", "Max Edge Length"), Tooltip("The maximum allowed length for contour edges along the border of the mesh.")]
        public float MaxEdgeLen = 1200.0f;

        /// <summary>
        /// The maximum distance a simplified contour's border edges should deviate from the original raw contour.
        /// </summary>
        [DefaultValue(1.3f), Limit(0.1f, 4)]
        [EditorOrder(70), EditorDisplay("Nav Mesh Options"), Tooltip("The maximum distance a simplified contour's border edges should deviate from the original raw contour.")]
        public float MaxEdgeError = 1.3f;

        /// <summary>
        /// The sampling distance to use when generating the detail mesh. For height detail only.
        /// </summary>
        [DefaultValue(600.0f), Limit(1)]
        [EditorOrder(80), EditorDisplay("Nav Mesh Options", "Detail Sampling Distance"), Tooltip("The sampling distance to use when generating the detail mesh.")]
        public float DetailSamplingDist = 600.0f;

        /// <summary>
        /// The maximum distance the detail mesh surface should deviate from heightfield data. For height detail only.
        /// </summary>
        [DefaultValue(1.0f), Limit(0, 3)]
        [EditorOrder(90), EditorDisplay("Nav Mesh Options"), Tooltip("The maximum distance the detail mesh surface should deviate from heightfield data.")]
        public float MaxDetailSamplingError = 1.0f;

        /// <summary>
        /// The radius of the smallest objects to traverse this nav mesh. Objects can't pass through gaps of less than twice the radius.
        /// </summary>
        [DefaultValue(34.0f), Limit(0)]
        [EditorOrder(1000), EditorDisplay("Agent Options"), Tooltip("The radius of the smallest objects to traverse this nav mesh. Objects can't pass through gaps of less than twice the radius.")]
        public float WalkableRadius = 34.0f;

        /// <summary>
        /// The height of the smallest objects to traverse this nav mesh. Objects can't enter areas with ceilings lower than this value.
        /// </summary>
        [DefaultValue(144.0f), Limit(0)]
        [EditorOrder(1010), EditorDisplay("Agent Options"), Tooltip("The height of the smallest objects to traverse this nav mesh. Objects can't enter areas with ceilings lower than this value.")]
        public float WalkableHeight = 144.0f;

        /// <summary>
        /// The maximum ledge height that is considered to still be traversable.
        /// </summary>
        [DefaultValue(35.0f), Limit(0)]
        [EditorOrder(1020), EditorDisplay("Agent Options"), Tooltip("The maximum ledge height that is considered to still be traversable.")]
        public float WalkableMaxClimb = 35.0f;

        /// <summary>
        /// The maximum slope that is considered walkable (in degrees). Objects can't go up or down slopes higher than this value.
        /// </summary>
        [DefaultValue(60.0f), Limit(0, 89.0f)]
        [EditorOrder(1030), EditorDisplay("Agent Options"), Tooltip("The maximum slope that is considered walkable (in degrees). Objects can't go up or down slopes higher than this value.")]
        public float WalkableMaxSlopeAngle = 60.0f;
    }
}
