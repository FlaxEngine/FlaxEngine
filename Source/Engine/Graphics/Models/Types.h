// Copyright (c) Wojciech Figat. All rights reserved.

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
class GPUVertexLayout;
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
API_ENUM(Attributes="HideInEditor") enum class MeshBufferType
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

    MAX,
};

// Vertex structure for all models (versioned)
// [Deprecated in v1.10]
PACK_STRUCT(struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") ModelVertex19
    {
    Float3 Position;
    Half2 TexCoord;
    FloatR10G10B10A2 Normal;
    FloatR10G10B10A2 Tangent;
    Half2 LightmapUVs;
    Color32 Color;
    });

PRAGMA_DISABLE_DEPRECATION_WARNINGS
// [Deprecated in v1.10]
typedef ModelVertex19 ModelVertex;
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// [Deprecated in v1.10]
struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") RawModelVertex
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
// [Deprecated in v1.10]
PACK_STRUCT(struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") VB0ElementType18
    {
    Float3 Position;

    static GPUVertexLayout* GetLayout();
    });

// [Deprecated in v1.10]
PACK_STRUCT(struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.")  VB1ElementType18
    {
    Half2 TexCoord;
    FloatR10G10B10A2 Normal;
    FloatR10G10B10A2 Tangent;
    Half2 LightmapUVs;

    static GPUVertexLayout* GetLayout();
    });

// [Deprecated in v1.10]
PACK_STRUCT(struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") VB2ElementType18
    {
    Color32 Color;

    static GPUVertexLayout* GetLayout();
    });

PRAGMA_DISABLE_DEPRECATION_WARNINGS
// [Deprecated in v1.10]
typedef VB0ElementType18 VB0ElementType;
// [Deprecated in v1.10]
typedef VB1ElementType18 VB1ElementType;
// [Deprecated in v1.10]
typedef VB2ElementType18 VB2ElementType;
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// Vertex structure for all skinned models (versioned)
// [Deprecated in v1.10]
PACK_STRUCT(struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") SkinnedModelVertex1
    {
    Float3 Position;
    Half2 TexCoord;
    FloatR10G10B10A2 Normal;
    FloatR10G10B10A2 Tangent;
    Color32 BlendIndices;
    Color32 BlendWeights;
    });

PRAGMA_DISABLE_DEPRECATION_WARNINGS
// [Deprecated in v1.10]
typedef SkinnedModelVertex1 SkinnedModelVertex;
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// [Deprecated in v1.10]
struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") RawSkinnedModelVertex
{
    Float3 Position;
    Float2 TexCoord;
    Float3 Normal;
    Float3 Tangent;
    Float3 Bitangent;
    Int4 BlendIndices;
    Float4 BlendWeights;
};

// [Deprecated on 28.04.2023, expires on 01.01.2024]
PACK_STRUCT(struct DEPRECATED("Use newer format.") VB0SkinnedElementType1
    {
    Float3 Position;
    Half2 TexCoord;
    FloatR10G10B10A2 Normal;
    FloatR10G10B10A2 Tangent;
    Color32 BlendIndices;
    Color32 BlendWeights;
    });

// [Deprecated in v1.10]
PACK_STRUCT(struct DEPRECATED("Use new MeshAccessor and depend on GPUVertexLayout when accessing mesh data.") VB0SkinnedElementType2
    {
    Float3 Position;
    Half2 TexCoord;
    FloatR10G10B10A2 Normal;
    FloatR10G10B10A2 Tangent;
    Color32 BlendIndices;
    Half4 BlendWeights;

    static GPUVertexLayout* GetLayout();
    });

PRAGMA_DISABLE_DEPRECATION_WARNINGS
// [Deprecated in v1.10]
typedef VB0SkinnedElementType2 VB0SkinnedElementType;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
