// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class Terrain
    {
        /// <summary>
        /// The constant amount of units per terrain geometry vertex (can be adjusted per terrain instance using non-uniform scale factor).
        /// </summary>
        public const float UnitsPerVertex = 100.0f;

        /// <summary>
        /// The maximum amount of levels of detail for the terrain chunks.
        /// </summary>
        public const int MaxLODs = 6;

        /// <summary>
        /// The constant amount of terrain chunks per terrain patch object.
        /// </summary>
        public const int PatchChunksCount = 16;

        /// <summary>
        /// The constant amount of terrain chunks on terrain patch object edge.
        /// </summary>
        public const int PatchEdgeChunksCount = 4;

        /// <summary>
        /// The terrain splatmaps amount limit. Each splatmap can hold up to 4 layer weights.
        /// </summary>
        public const int MaxSplatmapsCount = 2;

        /// <summary>
        /// Setups the terrain patch using the specified heightmap data.
        /// </summary>
        /// <param name="patchCoord">The patch location (x and z coordinates).</param>
        /// <param name="heightMap">The height map. Each array item contains a height value (2D inlined array). It should has size equal (chunkSize*4+1)^2.</param>
        /// <param name="holesMask">The holes mask (optional). Normalized to 0-1 range values with holes mask per-vertex. Must match the heightmap dimensions.</param>
        /// <param name="forceUseVirtualStorage">If set to <c>true</c> patch will use virtual storage by force. Otherwise it can use normal texture asset storage on drive (valid only during Editor). Runtime-created terrain can only use virtual storage (in RAM).</param>
        /// <returns>True if failed, otherwise false.</returns>
        public unsafe bool SetupPatchHeightMap(ref Int2 patchCoord, float[] heightMap, byte[] holesMask = null, bool forceUseVirtualStorage = false)
        {
            if (heightMap == null)
                throw new ArgumentNullException(nameof(heightMap));

            fixed (float* heightMapPtr = heightMap)
            {
                fixed (byte* holesMaskPtr = holesMask)
                {
                    return SetupPatchHeightMap(ref patchCoord, heightMap.Length, heightMapPtr, holesMaskPtr, forceUseVirtualStorage);
                }
            }
        }

        /// <summary>
        /// Setups the terrain patch using the specified heightmap data.
        /// </summary>
        /// <param name="patchCoord">The patch location (x and z coordinates).</param>
        /// <param name="index">The zero-based index of the splatmap texture.</param>
        /// <param name="splatMap">The splat map. Each array item contains 4 layer weights. It must match the terrain descriptor, so it should be (chunkSize*4+1)^2. Patch is a 4 by 4 square made of chunks. Each chunk has chunkSize quads on edge.</param>
        /// <param name="forceUseVirtualStorage">If set to <c>true</c> patch will use virtual storage by force. Otherwise it can use normal texture asset storage on drive (valid only during Editor). Runtime-created terrain can only use virtual storage (in RAM).</param>
        /// <returns>True if failed, otherwise false.</returns>
        public unsafe bool SetupPatchSplatMap(ref Int2 patchCoord, int index, Color32[] splatMap, bool forceUseVirtualStorage = false)
        {
            if (splatMap == null)
                throw new ArgumentNullException(nameof(splatMap));

            fixed (Color32* splatMapPtr = splatMap)
            {
                return SetupPatchSplatMap(ref patchCoord, index, splatMap.Length, splatMapPtr, forceUseVirtualStorage);
            }
        }
    }
}
