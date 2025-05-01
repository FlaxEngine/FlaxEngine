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
        } Diffuse;

        struct
        {
            Color Color = Color::Transparent;
            Guid Texture = Guid::Empty;
        } Emissive;

        struct
        {
            float Value = 1.0f;
            Guid Texture = Guid::Empty;
        } Opacity;

        struct
        {
            float Value = 0.5f;
            Guid Texture = Guid::Empty;
        } Roughness;

        struct
        {
            Guid Texture = Guid::Empty;
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
