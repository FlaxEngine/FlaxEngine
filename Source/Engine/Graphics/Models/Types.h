// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Packed.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"

class Model;
class SkinnedModel;
class ModelBase;
class Mesh;
class SkinnedMesh;
class MeshBase;
class ModelData;
class ModelInstanceEntries;
class Model;
class SkinnedModel;
class MeshDeformation;
class GPUContext;
struct RenderView;

/// <summary>
/// Importing model lightmap UVs source
/// </summary>
API_ENUM(Attributes="HideInEditor") enum class ModelLightmapUVsSource
{
    // No lightmap UVs.
    Disable = 0,
    // Generate lightmap UVs from model geometry.
    Generate,
    /// The texcoords channel 0.
    Channel0,
    // The texcoords channel 1.
    Channel1,
    // The texcoords channel 2.
    Channel2,
    // The texcoords channel 3.
    Channel3,
};

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
    Float3 Position;
    Half2 TexCoord;
    Float1010102 Normal;
    Float1010102 Tangent;
    });

PACK_STRUCT(struct ModelVertex18
    {
    Float3 Position;
    Half2 TexCoord;
    Float1010102 Normal;
    Float1010102 Tangent;
    Half2 LightmapUVs;
    });

PACK_STRUCT(struct ModelVertex19
    {
    Float3 Position;
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
    Float3 Position;
    Float2 TexCoord;
    Float3 Normal;
    Float3 Tangent;
    Float3 Bitangent;
    Float2 LightmapUVs;
    Color Color;
};

// For vertex data we use three buffers: one with positions, one with other attributes, and one with colors
PACK_STRUCT(struct VB0ElementType15
    {
    Float3 Position;
    });

PACK_STRUCT(struct VB1ElementType15
    {
    Half2 TexCoord;
    Float1010102 Normal;
    Float1010102 Tangent;
    });

PACK_STRUCT(struct VB0ElementType18
    {
    Float3 Position;
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
    Float3 Position;
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
    Float3 Position;
    Float2 TexCoord;
    Float3 Normal;
    Float3 Tangent;
    Float3 Bitangent;
    Int4 BlendIndices;
    Float4 BlendWeights;
};

PACK_STRUCT(struct VB0SkinnedElementType1
    {
    Float3 Position;
    Half2 TexCoord;
    Float1010102 Normal;
    Float1010102 Tangent;
    Color32 BlendIndices;
    Color32 BlendWeights;
    });

PACK_STRUCT(struct VB0SkinnedElementType2
    {
    Float3 Position;
    Half2 TexCoord;
    Float1010102 Normal;
    Float1010102 Tangent;
    Color32 BlendIndices;
    Half4 BlendWeights;
    });

typedef VB0SkinnedElementType2 VB0SkinnedElementType;
//
