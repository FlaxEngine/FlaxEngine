// Copyright (c) Wojciech Figat. All rights reserved.

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
        // Vertex position. Maps to 'POSITION' semantic in the shader.
        Position = 1,
        // Vertex color. Maps to 'COLOR' semantic in the shader.
        Color = 2,
        // Vertex normal vector. Maps to 'NORMAL' semantic in the shader.
        Normal = 3,
        // Vertex tangent vector. Maps to 'TANGENT' semantic in the shader.
        Tangent = 4,
        // Skinned bone blend indices. Maps to 'BLENDINDICES' semantic in the shader.
        BlendIndices = 5,
        // Skinned bone blend weights. Maps to 'BLENDWEIGHTS' semantic in the shader.
        BlendWeights = 6,
        // Primary texture coordinate (UV). Maps to 'TEXCOORD0' semantic in the shader.
        TexCoord0 = 7,
        // Additional texture coordinate (UV1). Maps to 'TEXCOORD1' semantic in the shader.
        TexCoord1 = 8,
        // Additional texture coordinate (UV2). Maps to 'TEXCOORD2' semantic in the shader.
        TexCoord2 = 9,
        // Additional texture coordinate (UV3). Maps to 'TEXCOORD3' semantic in the shader.
        TexCoord3 = 10,
        // Additional texture coordinate (UV4). Maps to 'TEXCOORD4' semantic in the shader.
        TexCoord4 = 11,
        // Additional texture coordinate (UV5). Maps to 'TEXCOORD5' semantic in the shader.
        TexCoord5 = 12,
        // Additional texture coordinate (UV6). Maps to 'TEXCOORD6' semantic in the shader.
        TexCoord6 = 13,
        // Additional texture coordinate (UV7). Maps to 'TEXCOORD7' semantic in the shader.
        TexCoord7 = 14,
        // General purpose attribute (at index 0). Maps to 'ATTRIBUTE0' semantic in the shader.
        Attribute0 = 15,
        // General purpose attribute (at index 1). Maps to 'ATTRIBUTE1' semantic in the shader.
        Attribute1 = 16,
        // General purpose attribute (at index 2). Maps to 'ATTRIBUTE2' semantic in the shader.
        Attribute2 = 17,
        // General purpose attribute (at index 3). Maps to 'ATTRIBUTE3' semantic in the shader.
        Attribute3 = 18,
        // Lightmap UVs that usually map one of the texture coordinate channels. Maps to 'LIGHTMAP' semantic in the shader.
        Lightmap = 30,
        // Texture coordinate. Maps to 'TEXCOORD' semantic in the shader.
        TexCoord = TexCoord0,
        // General purpose attribute. Maps to 'ATTRIBUTE0' semantic in the shader.
        Attribute = Attribute0,
        MAX
    };

    // Type of the vertex element data.
    API_FIELD() Types Type;
    // Index of the input vertex buffer slot (as provided in GPUContext::BindVB).
    API_FIELD() byte Slot;
    // Byte offset of this element relative to the start of a vertex buffer. Use value 0 to use auto-calculated offset based on previous elements in the layout (except when explicitOffsets is false).
    API_FIELD() byte Offset;
    // Flag used to mark data using hardware-instancing (element will be repeated for every instance). Empty to step data per-vertex when reading input buffer stream (rather than per-instance step).
    API_FIELD() byte PerInstance;
    // Format of the vertex element data.
    API_FIELD() PixelFormat Format;

    String ToString() const;

    bool operator==(const VertexElement& other) const;

    FORCE_INLINE bool operator!=(const VertexElement& other) const
    {
        return !operator==(other);
    }
} PACK_END();

uint32 FLAXENGINE_API GetHash(const VertexElement& key);
