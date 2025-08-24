// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Serialization/SerializationFwd.h"

/// <summary>
/// The objects layers selection mask (from layers and tags settings). Uses 1 bit per layer (up to 32 layers).
/// </summary>
API_STRUCT() struct FLAXENGINE_API LayersMask
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LayersMask);

    /// <summary>
    /// The layers selection mask.
    /// </summary>
    API_FIELD() uint32 Mask = MAX_uint32;

public:
    FORCE_INLINE LayersMask()
    {
    }

    FORCE_INLINE LayersMask(uint32 mask)
        : Mask(mask)
    {
    }

    /// <summary>
    /// Determines whether the specified layer index is set in the mask.
    /// </summary>
    /// <param name="layerIndex">Index of the layer (zero-based).</param>
    /// <returns><c>true</c> if the specified layer is set; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool HasLayer(int32 layerIndex) const
    {
        return (Mask & (1 << layerIndex)) != 0;
    }

    /// <summary>
    /// Determines whether the specified layer name is set in the mask.
    /// </summary>
    /// <param name="layerName">Name of the layer (from Layers settings).</param>
    /// <returns><c>true</c> if the specified layer is set; otherwise, <c>false</c>.</returns>
    bool HasLayer(const StringView& layerName) const;

    /// <summary>
    /// Gets a layers mask from a specific layer name.
    /// </summary>
    /// <param name="layerNames">The names of the layers (from Layers settings).</param>
    /// <returns>A layers mask with the Mask set to the same Mask as the layer name passed in. Returns a LayersMask with a mask of 0 if no layer found.</returns>
    static LayersMask GetMask(Span<StringView> layerNames);

    operator uint32() const
    {
        return Mask;
    }

    bool operator==(const LayersMask& other) const
    {
        return Mask == other.Mask;
    }

    bool operator!=(const LayersMask& other) const
    {
        return Mask != other.Mask;
    }

    LayersMask operator+(const LayersMask& other) const
    {
        return Mask | other.Mask;
    }

    LayersMask operator-(const LayersMask& other) const
    {
        return Mask & ~other.Mask;
    }

    LayersMask operator&(const LayersMask& other) const
    {
        return Mask & other.Mask;
    }

    LayersMask operator|(const LayersMask& other) const
    {
        return Mask | other.Mask;
    }

    LayersMask operator^(const LayersMask& other) const
    {
        return Mask ^ other.Mask;
    }

    LayersMask operator-() const
    {
        return ~Mask;
    }

    LayersMask operator~() const
    {
        return ~Mask;
    }

    LayersMask& operator|=(const LayersMask& other)
    {
        Mask |= other.Mask;
        return *this;
    }

    LayersMask& operator&=(const LayersMask& other)
    {
        Mask &= other.Mask;
        return *this;
    }

    LayersMask& operator^=(const LayersMask& other)
    {
        Mask ^= other.Mask;
        return *this;
    }
};

// @formatter:off
namespace Serialization
{
    inline bool ShouldSerialize(const LayersMask& v, const void* otherObj)
    {
        return !otherObj || v != *(LayersMask*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const LayersMask& v, const void* otherObj)
    {
        stream.Uint(v.Mask);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, LayersMask& v, ISerializeModifier* modifier)
    {
        v.Mask = stream.GetUint();
    }
}
// @formatter:on
