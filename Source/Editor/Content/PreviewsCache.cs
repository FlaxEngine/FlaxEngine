// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor
{
    [HideInEditor]
    partial class PreviewsCache
    {
        /// <summary>
        /// The default asset previews icon size (both width and height since it's a square).
        /// </summary>
        public const int AssetIconSize = 64;

        /// <summary>
        /// The default assets previews atlas size
        /// </summary>
        public const int AssetIconsAtlasSize = 1024;

        /// <summary>
        /// The default assets previews atlas margin (between icons)
        /// </summary>
        public const int AssetIconsAtlasMargin = 4;

        /// <summary>
        /// The amount of asset icons per atlas row.
        /// </summary>
        public const int AssetIconsPerRow = (int)((float)AssetIconsAtlasSize / (AssetIconSize + AssetIconsAtlasMargin));

        /// <summary>
        /// The amount of asset icons per atlas.
        /// </summary>
        public const int AssetIconsPerAtlas = AssetIconsPerRow * AssetIconsPerRow;

        /// <summary>
        /// The default format of previews atlas.
        /// </summary>
        public const PixelFormat AssetIconsAtlasFormat = PixelFormat.R8G8B8A8_UNorm;
    }
}
