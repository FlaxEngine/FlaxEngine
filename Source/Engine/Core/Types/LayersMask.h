// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

    FORCE_INLINE bool HasLayer(int32 layerIndex) const
    {
        return (Mask & (1 << layerIndex)) != 0;
    }

    bool HasLayer(const StringView& layerName) const;

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
        return Mask && other.Mask;
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
