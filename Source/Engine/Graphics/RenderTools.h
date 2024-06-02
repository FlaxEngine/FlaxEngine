// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "PixelFormat.h"
#include "RenderView.h"
#include "Engine/Scripting/ScriptingType.h"

class Model;
class SkinnedModel;
struct RenderContext;
struct FloatR10G10B10A2;

GPU_CB_STRUCT(QuadShaderData {
    Float4 Color;
    });

/// <summary>
/// Set of utilities for rendering.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API RenderTools
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(RenderTools);
public:
    // Calculate a subresource index for a texture
    FORCE_INLINE static int32 CalcSubresourceIndex(uint32 mipSlice, int32 arraySlice, int32 mipLevels)
    {
        return mipSlice + arraySlice * mipLevels;
    }

    /// <summary>
    /// Computes the feature level for the given shader profile.
    /// </summary>
    /// <param name="profile">The shader profile.</param>
    /// <returns>The feature level matching the given shader profile.</returns>
    static FeatureLevel GetFeatureLevel(ShaderProfile profile);

    /// <summary>
    /// Check if the given shader profile supports the tessellation. Runtime can reject tessellation support but it defines if given shader profile CAN support tessellation.
    /// </summary>
    /// <param name="profile">The profile.</param>
    /// <returns>True if can support tessellation shaders, otherwise false.</returns>
    static bool CanSupportTessellation(ShaderProfile profile);

    /// <summary>
    /// Computes the image row pitch in bytes, and the slice pitch (size in bytes of the image) based on format, width, and height.
    /// </summary>
    /// <param name="format">The pixel format of the surface.</param>
    /// <param name="width">The surface width.</param>
    /// <param name="height">The surface height.</param>
    /// <param name="rowPitch">The number of bytes in a scanline of pixels in the image. A standard pitch is 'byte' aligned and therefore it is equal to bytes-per-pixel * width-of-image. For block-compressed (BC) formats, this is the number of bytes in a row of blocks (which covers up to 4 scanlines at once). The rowPitch can be larger than the number of valid pixels in the image due to alignment padding.</param>
    /// <param name="slicePitch">For volume (3D) textures, slicePitch is the number of bytes in each depth slice. For 1D and 2D images, this is simply the total size of the image including any alignment padding.</param>
    static void ComputePitch(PixelFormat format, int32 width, int32 height, uint32& rowPitch, uint32& slicePitch);

public:
    static void UpdateModelLODTransition(byte& lodTransition);
    static uint64 CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 mipLevels);
    static uint64 CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 depth, int32 mipLevels);

    /// <summary>
    /// Computes the bounds screen radius squared.
    /// </summary>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="view">The render view.</param>
    /// <returns>The squared radius.</returns>
    FORCE_INLINE static float ComputeBoundsScreenRadiusSquared(const Float3& origin, const float radius, const RenderView& view)
    {
        return ComputeBoundsScreenRadiusSquared(origin, radius, view.Position, view.Projection);
    }

    /// <summary>
    /// Computes the bounds screen radius squared.
    /// </summary>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="viewOrigin">The render view position.</param>
    /// <param name="projectionMatrix">The render view projection matrix.</param>
    /// <returns>The squared radius.</returns>
    static float ComputeBoundsScreenRadiusSquared(const Float3& origin, float radius, const Float3& viewOrigin, const Matrix& projectionMatrix);

    /// <summary>
    /// Computes the model LOD index to use during rendering.
    /// </summary>
    /// <param name="model">The model.</param>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>The zero-based LOD index. Returns -1 if model should not be rendered.</returns>
    API_FUNCTION() static int32 ComputeModelLOD(const Model* model, API_PARAM(Ref) const Float3& origin, float radius, API_PARAM(Ref) const RenderContext& renderContext);

    /// <summary>
    /// Computes the skinned model LOD index to use during rendering.
    /// </summary>
    /// <param name="model">The skinned model.</param>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>The zero-based LOD index. Returns -1 if model should not be rendered.</returns>
    API_FUNCTION() static int32 ComputeSkinnedModelLOD(const SkinnedModel* model, API_PARAM(Ref) const Float3& origin, float radius, API_PARAM(Ref) const RenderContext& renderContext);

    /// <summary>
    /// Computes the sorting key for depth value (quantized)
    /// Reference: http://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/
    /// </summary>
    FORCE_INLINE static uint32 ComputeDistanceSortKey(float distance)
    {
        const uint32 distanceI = *((uint32*)&distance);
        return ((uint32)(-(int32)(distanceI >> 31)) | 0x80000000) ^ distanceI;
    }

    // Calculates the update frequency and phrase for the given cached data (eg. cascaded shadow map or global sdf cascade contents). Lower data indices are updated first and more frequent.
    static void ComputeCascadeUpdateFrequency(int32 cascadeIndex, int32 cascadeCount, int32& updateFrequency, int32& updatePhrase, int32 updateMaxCountPerFrame = 1);

    // Checks if cached data should be updated during the given frame.
    FORCE_INLINE static bool ShouldUpdateCascade(int32 frameIndex, int32 cascadeIndex, int32 cascadeCount, int32 updateMaxCountPerFrame = 1, bool updateForce = false)
    {
        int32 updateFrequency, updatePhrase;
        ComputeCascadeUpdateFrequency(cascadeIndex, cascadeCount, updateFrequency, updatePhrase, updateMaxCountPerFrame);
        return (frameIndex % updateFrequency == updatePhrase) || updateForce;
    }

    // Calculates temporal offset in the dithering factor that gets cleaned out by TAA.
    // Returns 0-1 value based on unscaled draw time for temporal effects to reduce artifacts from screen-space dithering when using Temporal Anti-Aliasing.
    static float ComputeTemporalTime();

    static void CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal);
    static void CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal, const Float3& tangent);

    static void ComputeSphereModelDrawMatrix(const RenderView& view, const Float3& position, float radius, Matrix& resultWorld, bool& resultIsViewInside);
};

// Calculate mip levels count for a texture 1D
// @param width Most detailed mip width
// @param useMipLevels True if use mip levels, otherwise false (use only 1 mip)
// @returns Mip levels count
extern int32 MipLevelsCount(int32 width, bool useMipLevels = true);

// Calculate mip levels count for a texture 2D
// @param width Most detailed mip width
// @param height Most detailed mip height
// @param useMipLevels True if use mip levels, otherwise false (use only 1 mip)
// @returns Mip levels count
extern int32 MipLevelsCount(int32 width, int32 height, bool useMipLevels = true);

// Calculate mip levels count for a texture 3D
// @param width Most detailed mip width
// @param height Most detailed mip height
// @param depth Most detailed mip depths
// @param useMipLevels True if use mip levels, otherwise false (use only 1 mip)
// @returns Mip levels count
extern int32 MipLevelsCount(int32 width, int32 height, int32 depth, bool useMipLevels = true);
