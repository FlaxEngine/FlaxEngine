// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

class Terrain;
class Texture;

/// <summary>
/// Terrain* tools for editor. Allows to create and modify terrain.
/// </summary>
API_CLASS(Static, Namespace="FlaxEditor") class TerrainTools
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(TerrainTools);

    /// <summary>
    /// Checks if a given ray hits any of the terrain patches sides to add a new patch there.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="ray">The ray to use for tracing (eg. mouse ray in world space).</param>
    /// <param name="result">The result patch coordinates (x and z). Valid only when method returns true.</param>
    /// <returns>True if result is valid, otherwise nothing to add there.</returns>
    API_FUNCTION() static bool TryGetPatchCoordToAdd(Terrain* terrain, const Ray& ray, API_PARAM(Out) Int2& result);

    /// <summary>
    /// Generates the terrain from the input heightmap and splat maps.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="numberOfPatches">The number of patches (X and Z axis).</param>
    /// <param name="heightmap">The heightmap texture.</param>
    /// <param name="heightmapScale">The heightmap scale. Applied to adjust the normalized heightmap values into the world units.</param>
    /// <param name="splatmap1">The custom terrain splat map used as a source of the terrain layers weights. Each channel from RGBA is used as an independent layer weight for terrain layers composting. It's optional.</param>
    /// <param name="splatmap2">The custom terrain splat map used as a source of the terrain layers weights. Each channel from RGBA is used as an independent layer weight for terrain layers composting. It's optional.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool GenerateTerrain(Terrain* terrain, API_PARAM(Ref) const Int2& numberOfPatches, Texture* heightmap, float heightmapScale, Texture* splatmap1, Texture* splatmap2);

    /// <summary>
    /// Serializes the terrain chunk data to JSON string.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z).</param>
    /// <returns>The serialized chunk data.</returns>
    API_FUNCTION() static StringAnsi SerializePatch(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord);

    /// <summary>
    /// Deserializes the terrain chunk data from the JSON string.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z).</param>
    /// <param name="value">The JSON string with serialized patch data.</param>
    API_FUNCTION() static void DeserializePatch(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord, const StringAnsiView& value);

    /// <summary>
    /// Initializes the patch heightmap and collision to the default flat level.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to initialize it.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool InitializePatch(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord);

    /// <summary>
    /// Modifies the terrain patch heightmap with the given samples.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to modify it.</param>
    /// <param name="samples">The samples. The array length is size.X*size.Y. It has to be type of float.</param>
    /// <param name="offset">The offset from the first row and column of the heightmap data (offset destination x and z start position).</param>
    /// <param name="size">The size of the heightmap to modify (x and z). Amount of samples in each direction.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ModifyHeightMap(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord, const float* samples, API_PARAM(Ref) const Int2& offset, API_PARAM(Ref) const Int2& size);

    /// <summary>
    /// Modifies the terrain patch holes mask with the given samples.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to modify it.</param>
    /// <param name="samples">The samples. The array length is size.X*size.Y. It has to be type of byte.</param>
    /// <param name="offset">The offset from the first row and column of the mask data (offset destination x and z start position).</param>
    /// <param name="size">The size of the mask to modify (x and z). Amount of samples in each direction.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ModifyHolesMask(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord, const byte* samples, API_PARAM(Ref) const Int2& offset, API_PARAM(Ref) const Int2& size);

    /// <summary>
    /// Modifies the terrain patch splat map (layers mask) with the given samples.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to modify it.</param>
    /// <param name="index">The zero-based splatmap texture index.</param>
    /// <param name="samples">The samples. The array length is size.X*size.Y. It has to be type of <see cref="Color32"/>.</param>
    /// <param name="offset">The offset from the first row and column of the splatmap data (offset destination x and z start position).</param>
    /// <param name="size">The size of the splatmap to modify (x and z). Amount of samples in each direction.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ModifySplatMap(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord, int32 index, const Color32* samples, API_PARAM(Ref) const Int2& offset, API_PARAM(Ref) const Int2& size);

    /// <summary>
    /// Gets the raw pointer to the heightmap data (cached internally by the c++ core in editor).
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to gather it.</param>
    /// <returns>The pointer to the array of floats with terrain patch heights data. Null if failed to gather the data.</returns>
    API_FUNCTION() static float* GetHeightmapData(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord);

    /// <summary>
    /// Gets the raw pointer to the holes mask data (cached internally by the c++ core in editor).
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to gather it.</param>
    /// <returns>The pointer to the array of bytes with terrain patch holes mask data. Null if failed to gather the data.</returns>
    API_FUNCTION() static byte* GetHolesMaskData(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord);

    /// <summary>
    /// Gets the raw pointer to the splatmap data (cached internally by the c++ core in editor).
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="patchCoord">The patch coordinates (x and z) to gather it.</param>
    /// <param name="index">The zero-based splatmap texture index.</param>
    /// <returns>The pointer to the array of Color32 with terrain patch packed splatmap data. Null if failed to gather the data.</returns>
    API_FUNCTION() static Color32* GetSplatMapData(Terrain* terrain, API_PARAM(Ref) const Int2& patchCoord, int32 index);

    /// <summary>
    /// Export terrain's heightmap as a texture.
    /// </summary>
    /// <param name="terrain">The terrain.</param>
    /// <param name="outputFolder">The output folder path</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool ExportTerrain(Terrain* terrain, String outputFolder);
};
