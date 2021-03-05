// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "../Enums.h"
#include "Engine/Core/Math/Math.h"

/// <summary>
/// Material domain type. Material domain defines the target usage of the material shader.
/// </summary>
API_ENUM() enum class MaterialDomain : byte
{
    /// <summary>
    /// The surface material. Can be used to render the scene geometry including models and skinned models.
    /// </summary>
    Surface = 0,

    /// <summary>
    /// The post process material. Can be used to perform custom post-processing of the rendered frame.
    /// </summary>
    PostProcess = 1,

    /// <summary>
    /// The deferred decal material. Can be used to apply custom overlay or surface modifications to the object surfaces in the world.
    /// </summary>
    Decal = 2,

    /// <summary>
    /// The GUI shader. Can be used to draw custom control interface elements or to add custom effects to the GUI.
    /// </summary>
    GUI = 3,

    /// <summary>
    /// The terrain shader. Can be used only with landscape chunks geometry that use optimized vertex data and support multi-layered blending.
    /// </summary>
    Terrain = 4,

    /// <summary>
    /// The particle shader. Can be used only with particles geometry (sprites, trails and ribbons). Supports reading particle data on a GPU.
    /// </summary>
    Particle = 5,

    /// <summary>
    /// The deformable shader. Can be used only with objects that can be deformed (spline models).
    /// </summary>
    Deformable = 6,

    /// <summary>
    /// The particle shader used for volumetric effects rendering such as Volumetric Fog.
    /// </summary>
    VolumeParticle = 7,
};

/// <summary>
/// Material blending modes.
/// </summary>
API_ENUM() enum class MaterialBlendMode : byte
{
    /// <summary>
    /// The opaque material. Used during GBuffer pass rendering.
    /// </summary>
    Opaque = 0,

    /// <summary>
    /// The transparent material. Used during Forward pass rendering.
    /// </summary>
    Transparent = 1,

    /// <summary>
    /// The additive blend. Material color is used to add to color of the objects behind the surface. Used during Forward pass rendering.
    /// </summary>
    Additive = 2,

    /// <summary>
    /// The multiply blend. Material color is used to multiply color of the objects behind the surface. Used during Forward pass rendering.
    /// </summary>
    Multiply = 3,
};

// Old material blending mode used before introducing MaterialShadingModel
// [Deprecated on 10.09.2018, expires on 10.05.2019]
enum class OldMaterialBlendMode : byte
{
    Opaque = 0,
    Transparent = 1,
    Unlit = 2,
};

/// <summary>
/// Material shading modes. Defines how material inputs and properties are combined to result the final surface color.
/// </summary>
API_ENUM() enum class MaterialShadingModel : byte
{
    /// <summary>
    /// The unlit material. Emissive channel is used as an output color. Can perform custom lighting operations or just glow. Won't be affected by the lighting pipeline.
    /// </summary>
    Unlit = 0,

    /// <summary>
    /// The default lit material. The most common choice for the material surfaces.
    /// </summary>
    Lit = 1,

    /// <summary>
    /// The subsurface material. Intended for materials like vax or skin that need light scattering to transport simulation through the object.
    /// </summary>
    Subsurface = 2,

    /// <summary>
    /// The foliage material. Intended for foliage materials like leaves and grass that need light scattering to transport simulation through the thin object.
    /// </summary>
    Foliage = 3,
};

/// <summary>
/// Material transparent lighting modes.
/// [Deprecated on 24.07.2019, expires on 10.05.2021]
/// </summary>
enum class MaterialTransparentLighting_Deprecated : byte
{
    /// <summary>
    /// Shading is disabled.
    /// </summary>
    None = 0,

    /// <summary>
    /// Shading is performed per pixel for single directional light.
    /// </summary>
    SingleDirectionalPerPixel = 1
};

/// <summary>
/// Material usage flags (deprecated).
/// [Deprecated on 24.07.2019, expires on 10.05.2021]
/// </summary>
enum class MaterialFlags_Deprecated : uint32
{
    /// <summary>
    /// The none.
    /// </summary>
    None = 0,

    /// <summary>
    /// Material is using mask to discard some pixels.
    /// Masked materials are using full vertex buffer during shadow maps and depth pass rendering (need UVs).
    /// </summary>
    UseMask = 1 << 0,

    /// <summary>
    /// The two sided material. No triangle normal culling is performed.
    /// </summary>
    TwoSided = 1 << 1,

    /// <summary>
    /// The wireframe material.
    /// </summary>
    Wireframe = 1 << 2,

    /// <summary>
    /// The material is using emissive light.
    /// </summary>
    UseEmissive = 1 << 3,

    /// <summary>
    /// The transparent materials option. Disable depth test (material ignores depth).
    /// </summary>
    TransparentDisableDepthTest = 1 << 4,

    /// <summary>
    /// The transparent materials option. Disable fog.
    /// </summary>
    TransparentDisableFog = 1 << 5,

    /// <summary>
    /// The transparent materials option. Disable reflections.
    /// </summary>
    TransparentDisableReflections = 1 << 6,

    /// <summary>
    /// The transparent materials option. Disable depth buffer write (won't modify depth buffer value after rendering).
    /// </summary>
    DisableDepthWrite = 1 << 7,

    /// <summary>
    /// The transparent materials option. Disable distortion.
    /// </summary>
    TransparentDisableDistortion = 1 << 8,

    /// <summary>
    /// The material is using world position offset (it may be animated inside a shader).
    /// </summary>
    UsePositionOffset = 1 << 9,

    /// <summary>
    /// The material is using vertex colors. The render will try to feed the pipeline with a proper buffer so material can gather valid data.
    /// </summary>
    UseVertexColor = 1 << 10,

    /// <summary>
    /// The material is using per-pixel normal mapping.
    /// </summary>
    UseNormal = 1 << 11,

    /// <summary>
    /// The material is using position displacement (in domain shader).
    /// </summary>
    UseDisplacement = 1 << 12,

    /// <summary>
    /// The flag used to indicate that material input normal vector is defined in the world space rather than tangent space.
    /// </summary>
    InputWorldSpaceNormal = 1 << 13,

    /// <summary>
    /// The flag used to indicate that material uses dithered model LOD transition for smoother LODs switching.
    /// </summary>
    UseDitheredLODTransition = 1 << 14,

    /// <summary>
    /// The flag used to indicate that material uses refraction feature.
    /// </summary>
    UseRefraction = 1 << 15,
};

DECLARE_ENUM_OPERATORS(MaterialFlags_Deprecated);

/// <summary>
/// Material features flags.
/// </summary>
API_ENUM(Attributes="Flags") enum class MaterialFeaturesFlags : uint32
{
    /// <summary>
    /// No flags.
    /// </summary>
    None = 0,

    /// <summary>
    /// The wireframe material.
    /// </summary>
    Wireframe = 1 << 1,

    /// <summary>
    /// The depth test is disabled (material ignores depth).
    /// </summary>
    DisableDepthTest = 1 << 2,

    /// <summary>
    /// Disable depth buffer write (won't modify depth buffer value during rendering).
    /// </summary>
    DisableDepthWrite = 1 << 3,

    /// <summary>
    /// The flag used to indicate that material input normal vector is defined in the world space rather than tangent space.
    /// </summary>
    InputWorldSpaceNormal = 1 << 4,

    /// <summary>
    /// The flag used to indicate that material uses dithered model LOD transition for smoother LODs switching.
    /// </summary>
    DitheredLODTransition = 1 << 5,

    /// <summary>
    /// The flag used to disable fog. The Forward Pass materials option.
    /// </summary>
    DisableFog = 1 << 6,

    /// <summary>
    /// The flag used to disable reflections. The Forward Pass materials option.
    /// </summary>
    DisableReflections = 1 << 7,

    /// <summary>
    /// The flag used to disable distortion effect (light refraction). The Forward Pass materials option.
    /// </summary>
    DisableDistortion = 1 << 8,

    /// <summary>
    /// The flag used to enable refraction offset based on the difference between the per-pixel normal and the per-vertex normal. Useful for large water-like surfaces.
    /// </summary>
    PixelNormalOffsetRefraction = 1 << 9,
};

DECLARE_ENUM_OPERATORS(MaterialFeaturesFlags);

/// <summary>
/// Material features usage flags. Detected by the material generator to help graphics pipeline optimize rendering of material shaders.
/// </summary>
API_ENUM(Attributes="Flags") enum class MaterialUsageFlags : uint32
{
    /// <summary>
    /// No flags.
    /// </summary>
    None = 0,

    /// <summary>
    /// Material is using mask to discard some pixels. Masked materials are using full vertex buffer during shadow maps and depth pass rendering (need UVs).
    /// </summary>
    UseMask = 1 << 0,

    /// <summary>
    /// The material is using emissive light.
    /// </summary>
    UseEmissive = 1 << 1,

    /// <summary>
    /// The material is using world position offset (it may be animated inside a shader).
    /// </summary>
    UsePositionOffset = 1 << 2,

    /// <summary>
    /// The material is using vertex colors. The render will try to feed the pipeline with a proper buffer so material can gather valid data.
    /// </summary>
    UseVertexColor = 1 << 3,

    /// <summary>
    /// The material is using per-pixel normal mapping.
    /// </summary>
    UseNormal = 1 << 4,

    /// <summary>
    /// The material is using position displacement (in domain shader).
    /// </summary>
    UseDisplacement = 1 << 5,

    /// <summary>
    /// The flag used to indicate that material uses refraction feature.
    /// </summary>
    UseRefraction = 1 << 6,
};

DECLARE_ENUM_OPERATORS(MaterialUsageFlags);

/// <summary>
/// Decal material blending modes.
/// </summary>
API_ENUM() enum class MaterialDecalBlendingMode : byte
{
    /// <summary>
    /// Decal will be fully blended with the material surface.
    /// </summary>
    Translucent = 0,

    /// <summary>
    /// Decal color will be blended with the material surface color (using multiplication).
    /// </summary>
    Stain = 1,

    /// <summary>
    /// Decal will blend the normal vector only.
    /// </summary>
    Normal = 2,

    /// <summary>
    /// Decal will apply the emissive light only.
    /// </summary>
    Emissive = 3,
};

/// <summary>
/// Material input scene textures. Special inputs from the graphics pipeline.
/// </summary>
API_ENUM() enum class MaterialSceneTextures
{
    /// <summary>
    /// The scene color.
    /// </summary>
    SceneColor = 0,

    /// <summary>
    /// The scene depth.
    /// </summary>
    SceneDepth = 1,

    /// <summary>
    /// The material diffuse color.
    /// </summary>
    DiffuseColor = 2,

    /// <summary>
    /// The material specular color.
    /// </summary>
    SpecularColor = 3,

    /// <summary>
    /// The material world space normal.
    /// </summary>
    WorldNormal = 4,

    /// <summary>
    /// The ambient occlusion.
    /// </summary>
    AmbientOcclusion = 5,

    /// <summary>
    /// The material metalness value.
    /// </summary>
    Metalness = 6,

    /// <summary>
    /// The material roughness value.
    /// </summary>
    Roughness = 7,

    /// <summary>
    /// The material specular value.
    /// </summary>
    Specular = 8,

    /// <summary>
    /// The material color.
    /// </summary>
    BaseColor = 9,

    /// <summary>
    /// The material shading mode.
    /// </summary>
    ShadingModel = 10,
};

/// <summary>
/// Material info structure - version 1
/// [Deprecated on 10.09.2018, expires on 10.05.2019]
/// </summary>
struct MaterialInfo1
{
    int32 Version;
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo1& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && Flags == other.Flags;
    }
};

/// <summary>
/// Material info structure - version 2
/// [Deprecated on 10.09.2018, expires on 10.05.2019]
/// </summary>
struct MaterialInfo2
{
    int32 Version;
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo2& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && TransparentLighting == other.TransparentLighting
                && Flags == other.Flags;
    }
};

/// <summary>
/// Material info structure - version 3
/// [Deprecated on 10.09.2018, expires on 10.05.2019]
/// </summary>
struct MaterialInfo3
{
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo3& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && TransparentLighting == other.TransparentLighting
                && Flags == other.Flags;
    }
};

/// <summary>
/// Material info structure - version 4
/// [Deprecated on 10.09.2018, expires on 10.05.2019]
/// </summary>
struct MaterialInfo4
{
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;
    MaterialPostFxLocation PostFxLocation;

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo4& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && TransparentLighting == other.TransparentLighting
                && PostFxLocation == other.PostFxLocation
                && Flags == other.Flags;
    }
};

/// <summary>
/// Material info structure - version 5
/// [Deprecated on 10.09.2018, expires on 10.05.2019]
/// </summary>
struct MaterialInfo5
{
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;
    MaterialPostFxLocation PostFxLocation;
    float MaskThreshold;
    float OpacityThreshold;

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo5& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && TransparentLighting == other.TransparentLighting
                && PostFxLocation == other.PostFxLocation
                && Math::NearEqual(MaskThreshold, other.MaskThreshold)
                && Math::NearEqual(OpacityThreshold, other.OpacityThreshold)
                && Flags == other.Flags;
    }
};

/// <summary>
/// Material info structure - version 6
/// [Deprecated on 10.09.2018, expires on 10.05.2019]
/// </summary>
struct MaterialInfo6
{
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;
    MaterialDecalBlendingMode DecalBlendingMode;
    MaterialPostFxLocation PostFxLocation;
    float MaskThreshold;
    float OpacityThreshold;

    MaterialInfo6()
    {
    }

    MaterialInfo6(const MaterialInfo5& other)
    {
        Domain = other.Domain;
        BlendMode = other.BlendMode;
        Flags = other.Flags;
        TransparentLighting = other.TransparentLighting;
        DecalBlendingMode = MaterialDecalBlendingMode::Translucent;
        PostFxLocation = other.PostFxLocation;
        MaskThreshold = other.MaskThreshold;
        OpacityThreshold = other.OpacityThreshold;
    }

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo6& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && TransparentLighting == other.TransparentLighting
                && DecalBlendingMode == other.DecalBlendingMode
                && PostFxLocation == other.PostFxLocation
                && Math::NearEqual(MaskThreshold, other.MaskThreshold)
                && Math::NearEqual(OpacityThreshold, other.OpacityThreshold)
                && Flags == other.Flags;
    }
};

/// <summary>
/// Material info structure - version 7
/// [Deprecated on 13.09.2018, expires on 13.12.2018]
/// </summary>
struct MaterialInfo7
{
    MaterialDomain Domain;
    OldMaterialBlendMode BlendMode;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;
    MaterialDecalBlendingMode DecalBlendingMode;
    MaterialPostFxLocation PostFxLocation;
    float MaskThreshold;
    float OpacityThreshold;
    TessellationMethod TessellationMode;
    int32 MaxTessellationFactor;

    MaterialInfo7()
    {
    }

    MaterialInfo7(const MaterialInfo6& other)
    {
        Domain = other.Domain;
        BlendMode = other.BlendMode;
        Flags = other.Flags;
        TransparentLighting = other.TransparentLighting;
        DecalBlendingMode = other.DecalBlendingMode;
        PostFxLocation = other.PostFxLocation;
        MaskThreshold = other.MaskThreshold;
        OpacityThreshold = other.OpacityThreshold;
        TessellationMode = TessellationMethod::None;
        MaxTessellationFactor = 15;
    }

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo7& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && TransparentLighting == other.TransparentLighting
                && DecalBlendingMode == other.DecalBlendingMode
                && PostFxLocation == other.PostFxLocation
                && Math::NearEqual(MaskThreshold, other.MaskThreshold)
                && Math::NearEqual(OpacityThreshold, other.OpacityThreshold)
                && Flags == other.Flags
                && TessellationMode == other.TessellationMode
                && MaxTessellationFactor == other.MaxTessellationFactor;
    }
};

/// <summary>
/// Material info structure - version 8
/// [Deprecated on 24.07.2019, expires on 10.05.2021]
/// </summary>
struct MaterialInfo8
{
    MaterialDomain Domain;
    MaterialBlendMode BlendMode;
    MaterialShadingModel ShadingModel;
    MaterialFlags_Deprecated Flags;
    MaterialTransparentLighting_Deprecated TransparentLighting;
    MaterialDecalBlendingMode DecalBlendingMode;
    MaterialPostFxLocation PostFxLocation;
    float MaskThreshold;
    float OpacityThreshold;
    TessellationMethod TessellationMode;
    int32 MaxTessellationFactor;

    MaterialInfo8()
    {
    }

    MaterialInfo8(const MaterialInfo7& other)
    {
        Domain = other.Domain;
        switch (other.BlendMode)
        {
        case OldMaterialBlendMode::Opaque:
            BlendMode = MaterialBlendMode::Opaque;
            ShadingModel = MaterialShadingModel::Lit;
            break;
        case OldMaterialBlendMode::Transparent:
            BlendMode = MaterialBlendMode::Transparent;
            ShadingModel = MaterialShadingModel::Lit;
            break;
        case OldMaterialBlendMode::Unlit:
            BlendMode = MaterialBlendMode::Opaque;
            ShadingModel = MaterialShadingModel::Unlit;
            break;
        }
        Flags = other.Flags;
        TransparentLighting = other.TransparentLighting;
        DecalBlendingMode = other.DecalBlendingMode;
        PostFxLocation = other.PostFxLocation;
        MaskThreshold = other.MaskThreshold;
        OpacityThreshold = other.OpacityThreshold;
        TessellationMode = other.TessellationMode;
        MaxTessellationFactor = other.MaxTessellationFactor;
    }

    /// <summary>
    /// Compare structure with other one
    /// </summary>
    /// <param name="other">Other structure to compare</param>
    /// <returns>True if both structures are equal</returns>
    bool operator==(const MaterialInfo8& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && ShadingModel == other.ShadingModel
                && TransparentLighting == other.TransparentLighting
                && DecalBlendingMode == other.DecalBlendingMode
                && PostFxLocation == other.PostFxLocation
                && Math::NearEqual(MaskThreshold, other.MaskThreshold)
                && Math::NearEqual(OpacityThreshold, other.OpacityThreshold)
                && Flags == other.Flags
                && TessellationMode == other.TessellationMode
                && MaxTessellationFactor == other.MaxTessellationFactor;
    }
};

/// <summary>
/// Structure with basic information about the material surface. It describes how material is reacting on light and which graphical features of it requires to render.
/// </summary>
API_STRUCT() struct FLAXENGINE_API MaterialInfo
{
DECLARE_SCRIPTING_TYPE_MINIMAL(MaterialInfo);

    /// <summary>
    /// The material shader domain.
    /// </summary>
    API_FIELD() MaterialDomain Domain;

    /// <summary>
    /// The blending mode for rendering.
    /// </summary>
    API_FIELD() MaterialBlendMode BlendMode;

    /// <summary>
    /// The shading mode for lighting.
    /// </summary>
    API_FIELD() MaterialShadingModel ShadingModel;

    /// <summary>
    /// The usage flags.
    /// </summary>
    API_FIELD() MaterialUsageFlags UsageFlags;

    /// <summary>
    /// The features usage flags.
    /// </summary>
    API_FIELD() MaterialFeaturesFlags FeaturesFlags;

    /// <summary>
    /// The decal material blending mode.
    /// </summary>
    API_FIELD() MaterialDecalBlendingMode DecalBlendingMode;

    /// <summary>
    /// The post fx material rendering location.
    /// </summary>
    API_FIELD() MaterialPostFxLocation PostFxLocation;

    /// <summary>
    /// The primitives culling mode.
    /// </summary>
    API_FIELD() CullMode CullMode;

    /// <summary>
    /// The mask threshold.
    /// </summary>
    API_FIELD() float MaskThreshold;

    /// <summary>
    /// The opacity threshold.
    /// </summary>
    API_FIELD() float OpacityThreshold;

    /// <summary>
    /// The tessellation mode.
    /// </summary>
    API_FIELD() TessellationMethod TessellationMode;

    /// <summary>
    /// The maximum tessellation factor (used only if material uses tessellation).
    /// </summary>
    API_FIELD() int32 MaxTessellationFactor;

    MaterialInfo()
    {
    }

    MaterialInfo(const MaterialInfo8& other)
    {
        Domain = other.Domain;
        BlendMode = other.BlendMode;
        ShadingModel = other.ShadingModel;
        UsageFlags = MaterialUsageFlags::None;
        if (other.Flags & MaterialFlags_Deprecated::UseMask)
            UsageFlags |= MaterialUsageFlags::UseMask;
        if (other.Flags & MaterialFlags_Deprecated::UseEmissive)
            UsageFlags |= MaterialUsageFlags::UseEmissive;
        if (other.Flags & MaterialFlags_Deprecated::UsePositionOffset)
            UsageFlags |= MaterialUsageFlags::UsePositionOffset;
        if (other.Flags & MaterialFlags_Deprecated::UseVertexColor)
            UsageFlags |= MaterialUsageFlags::UseVertexColor;
        if (other.Flags & MaterialFlags_Deprecated::UseNormal)
            UsageFlags |= MaterialUsageFlags::UseNormal;
        if (other.Flags & MaterialFlags_Deprecated::UseDisplacement)
            UsageFlags |= MaterialUsageFlags::UseDisplacement;
        if (other.Flags & MaterialFlags_Deprecated::UseRefraction)
            UsageFlags |= MaterialUsageFlags::UseRefraction;
        FeaturesFlags = MaterialFeaturesFlags::None;
        if (other.Flags & MaterialFlags_Deprecated::Wireframe)
            FeaturesFlags |= MaterialFeaturesFlags::Wireframe;
        if (other.Flags & MaterialFlags_Deprecated::TransparentDisableDepthTest && BlendMode != MaterialBlendMode::Opaque)
            FeaturesFlags |= MaterialFeaturesFlags::DisableDepthTest;
        if (other.Flags & MaterialFlags_Deprecated::TransparentDisableFog && BlendMode != MaterialBlendMode::Opaque)
            FeaturesFlags |= MaterialFeaturesFlags::DisableFog;
        if (other.Flags & MaterialFlags_Deprecated::TransparentDisableReflections && BlendMode != MaterialBlendMode::Opaque)
            FeaturesFlags |= MaterialFeaturesFlags::DisableReflections;
        if (other.Flags & MaterialFlags_Deprecated::DisableDepthWrite)
            FeaturesFlags |= MaterialFeaturesFlags::DisableDepthWrite;
        if (other.Flags & MaterialFlags_Deprecated::TransparentDisableDistortion && BlendMode != MaterialBlendMode::Opaque)
            FeaturesFlags |= MaterialFeaturesFlags::DisableDistortion;
        if (other.Flags & MaterialFlags_Deprecated::InputWorldSpaceNormal)
            FeaturesFlags |= MaterialFeaturesFlags::InputWorldSpaceNormal;
        if (other.Flags & MaterialFlags_Deprecated::UseDitheredLODTransition)
            FeaturesFlags |= MaterialFeaturesFlags::DitheredLODTransition;
        if (other.BlendMode != MaterialBlendMode::Opaque && other.TransparentLighting == MaterialTransparentLighting_Deprecated::None)
            ShadingModel = MaterialShadingModel::Unlit;
        DecalBlendingMode = other.DecalBlendingMode;
        PostFxLocation = other.PostFxLocation;
        CullMode = other.Flags & MaterialFlags_Deprecated::TwoSided ? ::CullMode::TwoSided : ::CullMode::Normal;
        MaskThreshold = other.MaskThreshold;
        OpacityThreshold = other.OpacityThreshold;
        TessellationMode = other.TessellationMode;
        MaxTessellationFactor = other.MaxTessellationFactor;
    }

    bool operator==(const MaterialInfo& other) const
    {
        return Domain == other.Domain
                && BlendMode == other.BlendMode
                && ShadingModel == other.ShadingModel
                && UsageFlags == other.UsageFlags
                && FeaturesFlags == other.FeaturesFlags
                && DecalBlendingMode == other.DecalBlendingMode
                && PostFxLocation == other.PostFxLocation
                && CullMode == other.CullMode
                && Math::NearEqual(MaskThreshold, other.MaskThreshold)
                && Math::NearEqual(OpacityThreshold, other.OpacityThreshold)
                && TessellationMode == other.TessellationMode
                && MaxTessellationFactor == other.MaxTessellationFactor;
    }
};

// The current material info descriptor version used by the material pipeline
typedef MaterialInfo MaterialInfo9;
#define MaterialInfo_Version 9
