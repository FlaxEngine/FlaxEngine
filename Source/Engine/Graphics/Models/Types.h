// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Enums.h"
#include "Engine/Core/Math/Packed.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/VectorInt.h"

class Model;
class SkinnedModel;
class Mesh;
class SkinnedMesh;
class ModelData;
class ModelInstanceEntries;
class Model;
class SkinnedModel;
struct RenderView;

/// <summary>
/// Importing model lightmap UVs source
/// </summary>
DECLARE_ENUM_6(ModelLightmapUVsSource, Disable, Generate, Channel0, Channel1, Channel2, Channel3);

/// <summary>
/// The mesh buffer types.
/// </summary>
enum class MeshBufferType
{
    /// <summary>
    /// The index buffer.
    /// </summary>
    Index = 0,

    /// <summary>
    /// The vertex buffer (first).
    /// </summary>
    Vertex0 = 1,

    /// <summary>
    /// The vertex buffer (second).
    /// </summary>
    Vertex1 = 2,

    /// <summary>
    /// The vertex buffer (third).
    /// </summary>
    Vertex2 = 3,
};

// Vertex structure for all models (versioned)
PACK_STRUCT(struct ModelVertex15
    {
        Vector3 Position;
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
    });

PACK_STRUCT(struct ModelVertex18
    {
        Vector3 Position;
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
        Half2 LightmapUVs;
    });

PACK_STRUCT(struct ModelVertex19
    {
        Vector3 Position;
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
        Half2 LightmapUVs;
        Color32 Color;
    });

typedef ModelVertex19 ModelVertex;

//
struct RawModelVertex
{
    Vector3 Position;
    Vector2 TexCoord;
    Vector3 Normal;
    Vector3 Tangent;
    Vector3 Bitangent;
    Vector2 LightmapUVs;
    Color Color;
};

// For vertex data we use three buffers: one with positions, one with other attributes, and one with colors
PACK_STRUCT(struct VB0ElementType15
    {
        Vector3 Position;
    });

PACK_STRUCT(struct VB1ElementType15
    {
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
    });

PACK_STRUCT(struct VB0ElementType18
    {
        Vector3 Position;
    });

PACK_STRUCT(struct VB1ElementType18
    {
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
        Half2 LightmapUVs;
    });

PACK_STRUCT(struct VB2ElementType18
    {
        Color32 Color;
    });

typedef VB0ElementType18 VB0ElementType;
typedef VB1ElementType18 VB1ElementType;
typedef VB2ElementType18 VB2ElementType;
//

// Vertex structure for all skinned models (versioned)
PACK_STRUCT(struct SkinnedModelVertex1
    {
        Vector3 Position;
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
        Color32 BlendIndices;
        Color32 BlendWeights;
    });

typedef SkinnedModelVertex1 SkinnedModelVertex;

//
struct RawSkinnedModelVertex
{
    Vector3 Position;
    Vector2 TexCoord;
    Vector3 Normal;
    Vector3 Tangent;
    Vector3 Bitangent;
    Int4 BlendIndices;
    Vector4 BlendWeights;
};

PACK_STRUCT(struct VB0SkinnedElementType1
    {
        Vector3 Position;
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
        Color32 BlendIndices;
        Color32 BlendWeights;
    });

PACK_STRUCT(struct VB0SkinnedElementType2
    {
        Vector3 Position;
        Half2 TexCoord;
        Float1010102 Normal;
        Float1010102 Tangent;
        Color32 BlendIndices;
        Half4 BlendWeights;
    });

typedef VB0SkinnedElementType2 VB0SkinnedElementType;
//
