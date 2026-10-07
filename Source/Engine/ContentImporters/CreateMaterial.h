// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Types.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Content/Assets/Material.h"

/// <summary>
/// Creating materials utility
/// </summary>
class CreateMaterial
{
public:
    struct Options
    {
        MaterialInfo Info;

        struct
        {
            Color Color = Color::White;
            Guid Texture = Guid::Empty;
            bool HasAlphaMask = false;
            /// <summary>
            /// When true and a texture is provided, the color multiplier is created as a public material parameter ("Color") instead of a hardcoded constant node.
            /// </summary>
            bool ColorAsParameter = false;
        } Diffuse;

        struct
        {
            Color Color = Color::Transparent;
            Guid Texture = Guid::Empty;
            /// <summary>
            /// When true, the emissive color multiplier is created as a public material parameter ("EmissiveColor", default black = no emission) instead of a hardcoded constant node. With an emissive texture it modulates it; without one it drives the Emissive input directly.
            /// </summary>
            bool ColorAsParameter = false;
        } Emissive;

        struct
        {
            float Value = 1.0f;
            Guid Texture = Guid::Empty;
        } Opacity;

        struct
        {
            float Value = 0.5f;
            uint8 Channel = 0;
            Guid Texture = Guid::Empty;
        } Roughness;

        struct
        {
            float Value = 0.0f;
            uint8 Channel = 0;
            Guid Texture = Guid::Empty;
        } Metalness;

        struct
        {
            float Value = 1.0f;
            uint8 Channel = 0;
            Guid Texture = Guid::Empty;
        } AmbientOcclusion;

        struct
        {
            Color Value = Color::Black;
            uint8 Channel = 0;
            Guid Texture = Guid::Empty;
        } Height;

        struct
        {
            Guid Texture = Guid::Empty;
            /// <summary>
            /// When true and a normal texture is provided, the normal scale is created as a public material parameter ("NormalStrength", default 1.0) and wired into the Normal input via a scale subgraph: normalize(float3(normal.xy * strength, normal.z)).
            /// </summary>
            bool StrengthAsParameter = false;
        } Normals;

        Options();
    };

    /// <summary>
    /// Creates the material asset.
    /// </summary>
    /// <param name="context">The importing context.</param>
    /// <returns>Result.</returns>
    static CreateAssetResult Create(CreateAssetContext& context);
};

#endif
