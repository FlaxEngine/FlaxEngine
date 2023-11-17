// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Guid.h"
#include "Engine/Graphics/Enums.h"
#include "Types.h"
#include "Config.h"
#include "SkeletonData.h"
#include "BlendShape.h"
#include "Engine/Animations/AnimationData.h"

class WriteStream;

/// <summary>
/// Data container for the common model meshes data. Supports holding all types of data related to the models pipeline.
/// </summary>
class FLAXENGINE_API MeshData
{
public:
    /// <summary>
    /// The slot index in the model materials to use during rendering.
    /// </summary>
    int32 MaterialSlotIndex = 0;

    /// <summary>
    /// The model skeleton node index. Used during importing and by the animated models.
    /// </summary>
    int32 NodeIndex = 0;

    /// <summary>
    /// The name of the mesh.
    /// </summary>
    String Name;

    /// <summary>
    /// Mesh positions buffer
    /// </summary>
    Array<Float3> Positions;

    /// <summary>
    /// Texture coordinates
    /// </summary>
    Array<Float2> UVs;

    /// <summary>
    /// Normals vector
    /// </summary>
    Array<Float3> Normals;

    /// <summary>
    /// Tangents vectors
    /// </summary>
    Array<Float3> Tangents;

    /// <summary>
    /// Bitangents vectors signs (used for bitangent reconstruction). Can be +1 or -1.
    /// bitangent = cross(normal, tangent) * sign
    /// sign = dot(cross(bitangent, normal), tangent)
    /// </summary>
    Array<float> BitangentSigns;

    /// <summary>
    /// Mesh index buffer
    /// </summary>
    Array<uint32> Indices;

    /// <summary>
    /// Lightmap UVs
    /// </summary>
    Array<Float2> LightmapUVs;

    /// <summary>
    /// Vertex colors
    /// </summary>
    Array<Color> Colors;

    /// <summary>
    /// Skinned mesh blend indices (max 4 per bone)
    /// </summary>
    Array<Int4> BlendIndices;

    /// <summary>
    /// Skinned mesh index buffer (max 4 per bone)
    /// </summary>
    Array<Float4> BlendWeights;

    /// <summary>
    /// Blend shapes used by this mesh
    /// </summary>
    Array<BlendShape> BlendShapes;

    /// <summary>
    /// Global translation for this mesh to be at it's local origin.
    /// </summary>
    Vector3 OriginTranslation = Vector3::Zero;

    /// <summary>
    /// Orientation for this mesh at it's local origin.
    /// </summary>
    Quaternion OriginOrientation = Quaternion::Identity;

    /// <summary>
    /// Meshes scaling.
    /// </summary>
    Vector3 Scaling = Vector3::One;

public:
    /// <summary>
    /// Determines whether this instance has any mesh data.
    /// </summary>
    FORCE_INLINE bool HasData() const
    {
        return Indices.HasItems();
    }

public:
    /// <summary>
    /// Clear arrays
    /// </summary>
    void Clear();

    /// <summary>
    /// Ensure that buffers will have given space for data
    /// </summary>
    /// <param name="vertices">Amount of vertices.</param>
    /// <param name="indices">Amount of indices.</param>
    /// <param name="preserveContents">Failed if clear data otherwise will try to preserve the buffers contents.</param>
    /// <param name="withColors">True if use vertex colors buffer.</param>
    /// <param name="withSkin">True if use vertex blend indices and weights buffer.</param>
    void EnsureCapacity(int32 vertices, int32 indices, bool preserveContents = false, bool withColors = true, bool withSkin = true);

    /// <summary>
    /// Swaps the vertex and index buffers contents (without a data copy) with the other mesh.
    /// </summary>
    /// <param name="other">The other mesh to swap data with.</param>
    void SwapBuffers(MeshData& other);

    /// <summary>
    /// Clean data
    /// </summary>
    void Release();

public:
    /// <summary>
    /// Init from model vertices array
    /// </summary>
    /// <param name="vertices">Array of vertices</param>
    /// <param name="verticesCount">Amount of vertices</param>
    void InitFromModelVertices(ModelVertex19* vertices, uint32 verticesCount);

    /// <summary>
    /// Init from model vertices array
    /// </summary>
    /// <param name="vertices">Array of vertices</param>
    /// <param name="verticesCount">Amount of vertices</param>
    void InitFromModelVertices(ModelVertex18* vertices, uint32 verticesCount);

    /// <summary>
    /// Init from model vertices array
    /// </summary>
    /// <param name="vertices">Array of vertices</param>
    /// <param name="verticesCount">Amount of vertices</param>
    void InitFromModelVertices(ModelVertex15* vertices, uint32 verticesCount);

    /// <summary>
    /// Init from model vertices array
    /// </summary>
    /// <param name="vb0">Array of data for vertex buffer 0</param>
    /// <param name="vb1">Array of data for vertex buffer 1</param>
    /// <param name="verticesCount">Amount of vertices</param>
    void InitFromModelVertices(VB0ElementType18* vb0, VB1ElementType18* vb1, uint32 verticesCount);

    /// <summary>
    /// Init from model vertices array
    /// </summary>
    /// <param name="vb0">Array of data for vertex buffer 0</param>
    /// <param name="vb1">Array of data for vertex buffer 1</param>
    /// <param name="vb2">Array of data for vertex buffer 2</param>
    /// <param name="verticesCount">Amount of vertices</param>
    void InitFromModelVertices(VB0ElementType18* vb0, VB1ElementType18* vb1, VB2ElementType18* vb2, uint32 verticesCount);

    /// <summary>
    /// Init from model vertices array
    /// </summary>
    /// <param name="vb0">Array of data for vertex buffer 0</param>
    /// <param name="vb1">Array of data for vertex buffer 1</param>
    /// <param name="verticesCount">Amount of vertices</param>
    void InitFromModelVertices(VB0ElementType15* vb0, VB1ElementType15* vb1, uint32 verticesCount);

    /// <summary>
    /// Sets the index buffer data.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <param name="indicesCount">The indices count.</param>
    void SetIndexBuffer(void* data, uint32 indicesCount);

public:
    /// <summary>
    /// Pack mesh data to the stream
    /// </summary>
    /// <param name="stream">Output stream</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    bool Pack2Model(WriteStream* stream) const;

    /// <summary>
    /// Pack skinned mesh data to the stream
    /// </summary>
    /// <param name="stream">Output stream</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    bool Pack2SkinnedModel(WriteStream* stream) const;

    /// <summary>
    /// Calculate bounding box for the mesh
    /// </summary>
    /// <param name="result">Output box</param>
    void CalculateBox(BoundingBox& result) const;

    /// <summary>
    /// Calculate bounding sphere for the mesh
    /// </summary>
    /// <param name="result">Output sphere</param>
    void CalculateSphere(BoundingSphere& result) const;

public:
#if COMPILE_WITH_MODEL_TOOL

    /// <summary>
    /// Generate lightmap uvs for the mesh entry
    /// </summary>
    /// <returns>True if generating lightmap uvs failed, otherwise false</returns>
    bool GenerateLightmapUVs();

    /// <summary>
    /// Iterates over the vertex buffers to remove the duplicated vertices and generate the optimized index buffer.
    /// </summary>
    void BuildIndexBuffer();

    /// <summary>
    /// Generate lightmap uvs for the mesh entry
    /// </summary>
    /// <param name="position">The target position to check.</param>
    /// <param name="epsilon">The position comparision epsilon.</param>
    /// <param name="result">The output vertices indices array.</param>
    void FindPositions(const Float3& position, float epsilon, Array<int32>& result);

    /// <summary>
    /// Generates the normal vectors for the mesh geometry.
    /// </summary>
    /// <param name="smoothingAngle">Specifies the maximum angle (in degrees) that may be between two face normals at the same vertex position that their are smoothed together.</param>
    /// <returns>True if generating failed, otherwise false</returns>
    bool GenerateNormals(float smoothingAngle = 175.0f);

    /// <summary>
    /// Generates the tangent vectors for the mesh geometry. Requires normal vector and texture coords channel to be valid.
    /// </summary>
    /// <param name="smoothingAngle">Specifies the maximum angle (in degrees) that may be between two vertex tangents that their tangents and bi-tangents are smoothed.</param>
    /// <returns>True if generating failed, otherwise false.</returns>
    bool GenerateTangents(float smoothingAngle = 45.0f);

    /// <summary>
    /// Reorders all triangles for improved vertex cache locality. It tries to arrange all triangles to fans and to render triangles which share vertices directly one after the other.
    /// </summary>
    void ImproveCacheLocality();

    /// <summary>
    /// Sums the area of all triangles in the mesh.
    /// </summary>
    /// <returns>The area sum of all mesh triangles.</returns>
    float CalculateTrianglesArea() const;

#endif

    /// <summary>
    /// Transform a vertex buffer positions, normals, tangents and bitangents using the given matrix.
    /// </summary>
    /// <param name="matrix">The matrix to use for the transformation.</param>
    void TransformBuffer(const Matrix& matrix);

    /// <summary>
    /// Normalizes the blend weights. Requires to have vertices with positions and blend weights setup.
    /// </summary>
    void NormalizeBlendWeights();

    /// <summary>
    /// Merges this mesh data with the specified other mesh.
    /// </summary>
    /// <param name="other">The other mesh to merge with.</param>
    void Merge(MeshData& other);
};

/// <summary>
/// Model texture resource descriptor.
/// </summary>
struct FLAXENGINE_API TextureEntry
{
    enum class TypeHint
    {
        ColorRGB,
        ColorRGBA,
        Normals,
    };

    /// <summary>
    /// The absolute path to the file.
    /// </summary>
    String FilePath;

    /// <summary>
    /// The texture contents hint based on the usage/context.
    /// </summary>
    TypeHint Type;

    /// <summary>
    /// The texture asset identifier.
    /// </summary>
    Guid AssetID;
};

/// <summary>
/// Model material slot entry that describes model mesh appearance.
/// </summary>
struct FLAXENGINE_API MaterialSlotEntry
{
    /// <summary>
    /// The slot name.
    /// </summary>
    String Name;

    /// <summary>
    /// Gets or sets shadows casting mode by this visual element
    /// </summary>
    ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

    /// <summary>
    /// The material asset identifier (material or material instance).
    /// </summary>
    Guid AssetID;

    struct
    {
        Color Color = Color::White;
        int32 TextureIndex = -1;
        bool HasAlphaMask = false;
    } Diffuse;

    struct
    {
        Color Color = Color::Transparent;
        int32 TextureIndex = -1;
    } Emissive;

    struct
    {
        float Value = 1.0f;
        int32 TextureIndex = -1;
    } Opacity;

    struct
    {
        int32 TextureIndex = -1;
    } Normals;

    bool TwoSided = false;

    bool UsesProperties() const;
};

/// <summary>
/// Data container for LOD metadata and sub meshes.
/// </summary>
class FLAXENGINE_API ModelLodData
{
public:
    /// <summary>
    /// The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.
    /// </summary>
    float ScreenSize = 1.0f;

    /// <summary>
    /// The meshes array.
    /// </summary>
    Array<MeshData*> Meshes;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ModelLodData"/> class.
    /// </summary>
    ModelLodData()
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="ModelLodData"/> class.
    /// </summary>
    ~ModelLodData()
    {
        Meshes.ClearDelete();
    }

    /// <summary>
    /// Gets the bounding box combined for all meshes in this model LOD.
    /// </summary>
    BoundingBox GetBox() const;
};

/// <summary>
/// Data container for model metadata and LODs.
/// </summary>
class FLAXENGINE_API ModelData
{
public:
    /// <summary>
    /// The minimum screen size to draw model (the bottom limit).
    /// </summary>
    float MinScreenSize = 0.0f;

    /// <summary>
    /// The texture slots.
    /// </summary>
    Array<TextureEntry> Textures;

    /// <summary>
    /// The material slots.
    /// </summary>
    Array<MaterialSlotEntry> Materials;

    /// <summary>
    /// Array with all LODs. The first element is the top most LOD0 followed by the LOD1, LOD2, etc.
    /// </summary>
    Array<ModelLodData> LODs;

    /// <summary>
    /// The skeleton bones hierarchy.
    /// </summary>
    SkeletonData Skeleton;

    /// <summary>
    /// The node animations.
    /// </summary>
    AnimationData Animation;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ModelData"/> class.
    /// </summary>
    ModelData()
    {
    }

public:
    /// <summary>
    /// Gets the valid level of details count.
    /// </summary>
    /// <returns>The LOD count.</returns>
    FORCE_INLINE int32 GetLODsCount() const
    {
        return LODs.Count();
    }

    /// <summary>
    /// Determines whether this instance has valid skeleton structure.
    /// </summary>
    /// <returns>True if has skeleton, otherwise false.</returns>
    FORCE_INLINE bool HasSkeleton() const
    {
        return Skeleton.Bones.HasItems();
    }

public:
    /// <summary>
    /// Automatically calculates the screen size for every model LOD for a proper transitions.
    /// </summary>
    void CalculateLODsScreenSizes();

    /// <summary>
    /// Transform a vertex buffer positions, normals, tangents and bitangents using the given matrix. Applies to all the LODs and meshes.
    /// </summary>
    /// <param name="matrix">The matrix to use for the transformation.</param>
    void TransformBuffer(const Matrix& matrix);

public:
    /// <summary>
    /// Pack mesh data to the header stream
    /// </summary>
    /// <param name="stream">Output stream</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    bool Pack2ModelHeader(WriteStream* stream) const;

    /// <summary>
    /// Pack skinned mesh data to the header stream
    /// </summary>
    /// <param name="stream">Output stream</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    bool Pack2SkinnedModelHeader(WriteStream* stream) const;

    /// <summary>
    /// Pack animation data to the header stream
    /// </summary>
    /// <param name="stream">Output stream</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    bool Pack2AnimationHeader(WriteStream* stream) const;
};
