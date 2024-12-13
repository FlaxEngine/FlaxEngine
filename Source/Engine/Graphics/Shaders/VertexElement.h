// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/PixelFormat.h"

/// <summary>
/// Vertex buffer data element. Defines access to data passed to Vertex Shader.
/// </summary>
API_STRUCT(NoDefault)
PACK_BEGIN() struct FLAXENGINE_API VertexElement
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(VertexElement);

    /// <summary>
    /// Types of vertex elements.
    /// </summary>
    API_ENUM() enum class Types : byte
    {
        // Undefined.
        Unknown = 0,
        // Vertex position.
        Position = 1,
        // Vertex color.
        Color = 2,
        // Vertex normal vector.
        Normal = 3,
        // Vertex tangent vector.
        Tangent = 4,
        // Skinned bone blend indices.
        BlendIndices = 5,
        // Skinned bone blend weights.
        BlendWeight = 6,
        // Primary texture coordinate (UV).
        TexCoord0 = 7,
        // Additional texture coordinate (UV1).
        TexCoord1 = 8,
        // Additional texture coordinate (UV2).
        TexCoord2 = 9,
        // Additional texture coordinate (UV3).
        TexCoord3 = 10,
        // Additional texture coordinate (UV4).
        TexCoord4 = 11,
        // Additional texture coordinate (UV5).
        TexCoord5 = 12,
        // Additional texture coordinate (UV6).
        TexCoord6 = 13,
        // Additional texture coordinate (UV7).
        TexCoord7 = 14,
        // General purpose attribute (at index 0).
        Attribute0 = 15,
        // General purpose attribute (at index 1).
        Attribute1 = 16,
        // General purpose attribute (at index 2).
        Attribute2 = 17,
        // General purpose attribute (at index 3).
        Attribute3 = 18,
        // Texture coordinate.
        TexCoord = TexCoord0,
        // General purpose attribute.
        Attribute = Attribute0,
        MAX
    };

    // Type of the vertex element data.
    Types Type;
    // Index of the input vertex buffer slot (as provided in GPUContext::BindVB).
    byte Slot;
    // Byte offset of this element relative to the start of a vertex buffer. Use value 0 to use auto-calculated offset based on previous elements in the layout (or for the first one).
    byte Offset;
    // Flag used to mark data using hardware-instancing (element will be repeated for every instance). Empty to step data per-vertex when reading input buffer stream (rather than per-instance step).
    byte PerInstance;
    // Format of the vertex element data.
    PixelFormat Format;

    String ToString() const;

    bool operator==(const VertexElement& other) const;

    FORCE_INLINE bool operator!=(const VertexElement& other) const
    {
        return !operator==(other);
    }
} PACK_END();

uint32 FLAXENGINE_API GetHash(const VertexElement& key);
