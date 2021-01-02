// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Enums.h"

class String;

/// <summary>
/// GPU sampler filter modes.
/// </summary>
enum class GPUSamplerFilter
{
    Point = 0,
    Bilinear = 1,
    Trilinear = 2,
    Anisotropic = 3,

    MAX
};

/// <summary>
/// GPU sampler address modes.
/// </summary>
enum class GPUSamplerAddressMode
{
    Wrap = 0,
    Clamp = 1,
    Mirror = 2,
    Border = 3,

    MAX
};

/// <summary>
/// GPU sampler comparision function types.
/// </summary>
enum class GPUSamplerCompareFunction
{
    Never = 0,
    Less = 1,

    MAX
};

/// <summary>
/// GPU sampler border color types.
/// </summary>
enum class GPUSamplerBorderColor
{
    /// <summary>
    /// Indicates black, with the alpha component as fully transparent.
    /// </summary>
    TransparentBlack = 0,

    /// <summary>
    /// Indicates black, with the alpha component as fully opaque.
    /// </summary>
    OpaqueBlack = 1,

    /// <summary>
    /// Indicates white, with the alpha component as fully opaque.
    /// </summary>
    OpaqueWhite = 2,

    Maximum
};

/// <summary>
/// A common description for all samplers.
/// </summary>
struct FLAXENGINE_API GPUSamplerDescription
{
public:

    /// <summary>
    /// The filtering method to use when sampling a texture.
    /// </summary>
    GPUSamplerFilter Filter;

    /// <summary>
    /// The addressing mode for outside [0..1] range for U coordinate.
    /// </summary>
    GPUSamplerAddressMode AddressU;

    /// <summary>
    /// The addressing mode for outside [0..1] range for V coordinate.
    /// </summary>
    GPUSamplerAddressMode AddressV;

    /// <summary>
    /// The addressing mode for outside [0..1] range for W coordinate.
    /// </summary>
    GPUSamplerAddressMode AddressW;

    /// <summary>
    /// The mip bias to be added to mipmap LOD calculation.
    /// </summary>
    float MipBias;

    /// <summary>
    /// The minimum mip map level that will be used, where 0 is the highest resolution mip level.
    /// </summary>
    float MinMipLevel;

    /// <summary>
    /// The maximum mip map level that will be used, where 0 is the highest resolution mip level. To have no upper limit on LOD set this to a large value such as MAX_float.
    /// </summary>
    float MaxMipLevel;

    /// <summary>
    /// The maximum anisotropy value.
    /// </summary>
    int32 MaxAnisotropy;

    /// <summary>
    /// The border color to use if Border is specified for AddressU, AddressV, or AddressW.
    /// </summary>
    GPUSamplerBorderColor BorderColor;

    /// <summary>
    /// A function that compares sampled data against existing sampled data.
    /// </summary>
    GPUSamplerCompareFunction SamplerComparisonFunction;

public:

    /// <summary>
    /// Clears description to the default values.
    /// </summary>
    void Clear();

public:

    /// <summary>
    /// Compares with other instance of SamplerDescription
    /// </summary>
    /// <param name="other">The other object to compare.</param>
    /// <returns>True if objects are the same, otherwise false.</returns>
    bool Equals(const GPUSamplerDescription& other) const;

    /// <summary>
    /// Implements the operator ==.
    /// </summary>
    /// <param name="other">The other description.</param>
    /// <returns>The result of the operator.</returns>
    FORCE_INLINE bool operator==(const GPUSamplerDescription& other) const
    {
        return Equals(other);
    }

    /// <summary>
    /// Implements the operator !=.
    /// </summary>
    /// <param name="other">The other description.</param>
    /// <returns>The result of the operator.</returns>
    FORCE_INLINE bool operator!=(const GPUSamplerDescription& other) const
    {
        return !Equals(other);
    }

public:

    String ToString() const;
};

uint32 GetHash(const GPUSamplerDescription& key);
