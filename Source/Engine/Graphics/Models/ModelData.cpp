// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ModelData.h"
#include "Engine/Core/Log.h"
#include "Engine/Animations/CurveSerialization.h"
#include "Engine/Serialization/WriteStream.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"

void MeshData::Clear()
{
    MaterialSlotIndex = 0;
    NodeIndex = 0;
    Positions.Clear();
    Indices.Clear();
    UVs.Clear();
    Normals.Clear();
    Tangents.Clear();
    LightmapUVs.Clear();
    Colors.Clear();
    BlendIndices.Clear();
    BlendWeights.Clear();
    BlendShapes.Clear();
}

void MeshData::EnsureCapacity(int32 vertices, int32 indices, bool preserveContents, bool withColors, bool withSkin)
{
    Positions.EnsureCapacity(vertices, preserveContents);
    Indices.EnsureCapacity(indices, preserveContents);
    UVs.EnsureCapacity(vertices, preserveContents);
    Normals.EnsureCapacity(vertices, preserveContents);
    Tangents.EnsureCapacity(vertices, preserveContents);
    LightmapUVs.EnsureCapacity(vertices, preserveContents);
    Colors.EnsureCapacity(withColors ? vertices : 0, preserveContents);
    BlendIndices.EnsureCapacity(withSkin ? vertices : 0, preserveContents);
    BlendWeights.EnsureCapacity(withSkin ? vertices : 0, preserveContents);
}

void MeshData::SwapBuffers(MeshData& other)
{
    Positions.Swap(other.Positions);
    Indices.Swap(other.Indices);
    UVs.Swap(other.UVs);
    Normals.Swap(other.Normals);
    Tangents.Swap(other.Tangents);
    LightmapUVs.Swap(other.LightmapUVs);
    Colors.Swap(other.Colors);
    BlendIndices.Swap(other.BlendIndices);
    BlendWeights.Swap(other.BlendWeights);
    BlendShapes.Swap(other.BlendShapes);
}

void MeshData::Release()
{
    MaterialSlotIndex = 0;
    Positions.Resize(0);
    Indices.Resize(0);
    UVs.Resize(0);
    Normals.Resize(0);
    Tangents.Resize(0);
    LightmapUVs.Resize(0);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);
}

void MeshData::InitFromModelVertices(ModelVertex19* vertices, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    LightmapUVs.Resize(verticesCount, false);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vertices->Position;
        UVs[i] = vertices->TexCoord.ToVector2();
        Normals[i] = vertices->Normal.ToVector3() * 2.0f - 1.0f;
        Tangents[i] = vertices->Tangent.ToVector3() * 2.0f - 1.0f;
        LightmapUVs[i] = vertices->LightmapUVs.ToVector2();
        Colors[i] = Color(vertices->Color);

        vertices++;
    }
}

void MeshData::InitFromModelVertices(ModelVertex18* vertices, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    LightmapUVs.Resize(verticesCount, false);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vertices->Position;
        UVs[i] = vertices->TexCoord.ToVector2();
        Normals[i] = vertices->Normal.ToVector3() * 2.0f - 1.0f;
        Tangents[i] = vertices->Tangent.ToVector3() * 2.0f - 1.0f;
        LightmapUVs[i] = vertices->LightmapUVs.ToVector2();

        vertices++;
    }
}

void MeshData::InitFromModelVertices(ModelVertex15* vertices, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    LightmapUVs.Resize(0);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vertices->Position;
        UVs[i] = vertices->TexCoord.ToVector2();
        Normals[i] = vertices->Normal.ToVector3() * 2.0f - 1.0f;
        Tangents[i] = vertices->Tangent.ToVector3() * 2.0f - 1.0f;

        vertices++;
    }
}

void MeshData::InitFromModelVertices(VB0ElementType18* vb0, VB1ElementType18* vb1, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    LightmapUVs.Resize(verticesCount, false);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vb0->Position;
        UVs[i] = vb1->TexCoord.ToVector2();
        Normals[i] = vb1->Normal.ToVector3() * 2.0f - 1.0f;
        Tangents[i] = vb1->Tangent.ToVector3() * 2.0f - 1.0f;
        LightmapUVs[i] = vb1->LightmapUVs.ToVector2();

        vb0++;
        vb1++;
    }
}

void MeshData::InitFromModelVertices(VB0ElementType18* vb0, VB1ElementType18* vb1, VB2ElementType18* vb2, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    LightmapUVs.Resize(verticesCount, false);
    if (vb2)
    {
        Colors.Resize(verticesCount, false);
    }
    else
    {
        Colors.Resize(0);
    }
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vb0->Position;
        UVs[i] = vb1->TexCoord.ToVector2();
        Normals[i] = vb1->Normal.ToVector3() * 2.0f - 1.0f;
        Tangents[i] = vb1->Tangent.ToVector3() * 2.0f - 1.0f;
        LightmapUVs[i] = vb1->LightmapUVs.ToVector2();
        if (vb2)
        {
            Colors[i] = Color(vb2->Color);
            vb2++;
        }

        vb0++;
        vb1++;
    }
}

void MeshData::InitFromModelVertices(VB0ElementType15* vb0, VB1ElementType15* vb1, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    LightmapUVs.Resize(0, false);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vb0->Position;
        UVs[i] = vb1->TexCoord.ToVector2();
        Normals[i] = vb1->Normal.ToVector3() * 2.0f - 1.0f;
        Tangents[i] = vb1->Tangent.ToVector3() * 2.0f - 1.0f;

        vb0++;
        vb1++;
    }
}

void MeshData::SetIndexBuffer(void* data, uint32 indicesCount)
{
    bool use16BitIndexBuffer = indicesCount <= MAX_uint16;

    Indices.Resize(indicesCount, false);

    if (use16BitIndexBuffer)
    {
        auto ib16 = static_cast<uint16*>(data);
        for (uint32 a = 0; a < indicesCount; a++)
            Indices[a] = *ib16++;
    }
    else
    {
        auto ib32 = static_cast<uint32*>(data);
        for (uint32 a = 0; a < indicesCount; a++)
            Indices[a] = *ib32++;
    }
}

bool MeshData::Pack2Model(WriteStream* stream) const
{
    // Validate input
    if (stream == nullptr)
    {
        LOG(Error, "Invalid input.");
        return true;
    }

    // Cache size
    uint32 verticiecCount = Positions.Count();
    uint32 indicesCount = Indices.Count();
    uint32 trianglesCount = indicesCount / 3;
    bool use16Bit = indicesCount <= MAX_uint16;
    if (verticiecCount == 0 || trianglesCount == 0 || indicesCount % 3 != 0)
    {
        LOG(Error, "Empty mesh! Triangles: {0}, Verticies: {1}.", trianglesCount, verticiecCount);
        return true;
    }

    // Validate data structure
    bool hasUVs = UVs.HasItems();
    if (hasUVs && UVs.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("UVs"));
        return true;
    }
    bool hasNormals = Normals.HasItems();
    if (hasNormals && Normals.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("Normals"));
        return true;
    }
    bool hasTangents = Tangents.HasItems();
    if (hasTangents && Tangents.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("Tangents"));
        return true;
    }
    bool hasLightmapUVs = LightmapUVs.HasItems();
    if (hasLightmapUVs && LightmapUVs.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("LightmapUVs"));
        return true;
    }
    bool hasVertexColors = Colors.HasItems();
    if (hasVertexColors && Colors.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("Colors"));
        return true;
    }

    // Vertices
    stream->WriteUint32(verticiecCount);

    // Triangles
    stream->WriteUint32(trianglesCount);

    // Vertex Buffer 0
    stream->WriteBytes(Positions.Get(), sizeof(Vector3) * verticiecCount);

    // Vertex Buffer 1
    VB1ElementType vb1;
    for (uint32 i = 0; i < verticiecCount; i++)
    {
        // Get vertex components
        Vector2 uv = hasUVs ? UVs[i] : Vector2::Zero;
        Vector3 normal = hasNormals ? Normals[i] : Vector3::UnitZ;
        Vector3 tangent = hasTangents ? Tangents[i] : Vector3::UnitX;
        Vector2 lightmapUV = hasLightmapUVs ? LightmapUVs[i] : Vector2::Zero;

        // Calculate bitangent sign
        Vector3 bitangent = Vector3::Normalize(Vector3::Cross(normal, tangent));
        byte sign = static_cast<byte>(Vector3::Dot(Vector3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

        // Write vertex
        vb1.TexCoord = Half2(uv);
        vb1.Normal = Float1010102(normal * 0.5f + 0.5f, 0);
        vb1.Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
        vb1.LightmapUVs = Half2(lightmapUV);
        stream->Write(&vb1);

        // Pack TBN matrix into a quaternion
        /*Quaternion quaternionTBN;
        bool invertedHandednessTBN;
        CalculateQuaternionFromTBN(tangent, bitangent, normal, &quaternionTBN, &invertedHandednessTBN);
        quaternionTBN.Normalize();
        uint32 packedQuaternionTBN = QuantizeNormalizedQuaternionWithHandedness(quaternionTBN, invertedHandednessTBN);

        Vector4 unpackedQuaternionTBN = Vector4(quaternionTBN.X, quaternionTBN.Y, quaternionTBN.Z, ((invertedHandednessTBN ? 0.0f : 128.0f) + (127.0f * (quaternionTBN.W * 0.5f + 0.5f))) / 255.0f);
        
        //lods.WriteUint32(packedQuaternionTBN);
        //lods.WriteVector4(unpackedQuaternionTBN);
        */
    }

    // Vertex Buffer 2
    stream->WriteBool(hasVertexColors);
    if (hasVertexColors)
    {
        VB2ElementType vb2;
        for (uint32 i = 0; i < verticiecCount; i++)
        {
            vb2.Color = Color32(Colors[i]);
            stream->Write(&vb2);
        }
    }

    // Index Buffer
    if (use16Bit)
    {
        for (uint32 i = 0; i < indicesCount; i++)
            stream->WriteUint16(Indices[i]);
    }
    else
    {
        stream->WriteBytes(Indices.Get(), sizeof(uint32) * indicesCount);
    }

    return false;
}

bool MeshData::Pack2SkinnedModel(WriteStream* stream) const
{
    // Validate input
    if (stream == nullptr)
    {
        LOG(Error, "Invalid input.");
        return true;
    }

    // Cache size
    uint32 verticiecCount = Positions.Count();
    uint32 indicesCount = Indices.Count();
    uint32 trianglesCount = indicesCount / 3;
    bool use16Bit = indicesCount <= MAX_uint16;
    if (verticiecCount == 0 || trianglesCount == 0 || indicesCount % 3 != 0)
    {
        LOG(Error, "Empty mesh! Triangles: {0}, Verticies: {1}.", trianglesCount, verticiecCount);
        return true;
    }

    // Validate data structure
    bool hasUVs = UVs.HasItems();
    if (hasUVs && UVs.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT( "UVs"));
        return true;
    }
    bool hasNormals = Normals.HasItems();
    if (hasNormals && Normals.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("Normals"));
        return true;
    }
    bool hasTangents = Tangents.HasItems();
    if (hasTangents && Tangents.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("Tangents"));
        return true;
    }
    if (BlendIndices.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("BlendIndices"));
        return true;
    }
    if (BlendWeights.Count() != verticiecCount)
    {
        LOG(Error, "Invalid size of {0} stream.", TEXT("BlendWeights"));
        return true;
    }

    // Vertices
    stream->WriteUint32(verticiecCount);

    // Triangles
    stream->WriteUint32(trianglesCount);

    // Blend Shapes
    stream->WriteUint16(BlendShapes.Count());
    for (const auto& blendShape : BlendShapes)
    {
        stream->WriteBool(blendShape.UseNormals);
        stream->WriteUint32(blendShape.MinVertexIndex);
        stream->WriteUint32(blendShape.MaxVertexIndex);
        stream->WriteUint32(blendShape.Vertices.Count());
        stream->WriteBytes(blendShape.Vertices.Get(), blendShape.Vertices.Count() * sizeof(BlendShapeVertex));
    }

    // Vertex Buffer
    VB0SkinnedElementType vb;
    for (uint32 i = 0; i < verticiecCount; i++)
    {
        // Get vertex components
        Vector2 uv = hasUVs ? UVs[i] : Vector2::Zero;
        Vector3 normal = hasNormals ? Normals[i] : Vector3::UnitZ;
        Vector3 tangent = hasTangents ? Tangents[i] : Vector3::UnitX;
        Int4 blendIndices = BlendIndices[i];
        Vector4 blendWeights = BlendWeights[i];

        // Calculate bitangent sign
        Vector3 bitangent = Vector3::Normalize(Vector3::Cross(normal, tangent));
        byte sign = static_cast<byte>(Vector3::Dot(Vector3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

        // Write vertex
        vb.Position = Positions[i];
        vb.TexCoord = Half2(uv);
        vb.Normal = Float1010102(normal * 0.5f + 0.5f, 0);
        vb.Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
        vb.BlendIndices = Color32(blendIndices.X, blendIndices.Y, blendIndices.Z, blendIndices.W);
        vb.BlendWeights = Half4(blendWeights);
        stream->Write(&vb);
    }

    // Index Buffer
    if (use16Bit)
    {
        for (uint32 i = 0; i < indicesCount; i++)
            stream->WriteUint16(Indices[i]);
    }
    else
    {
        stream->WriteBytes(Indices.Get(), sizeof(uint32) * indicesCount);
    }

    return false;
}

void MeshData::TransformBuffer(const Matrix& matrix)
{
    // Compute matrix inverse transpose
    Matrix inverseTransposeMatrix;
    Matrix::Invert(matrix, inverseTransposeMatrix);
    Matrix::Transpose(inverseTransposeMatrix, inverseTransposeMatrix);

    // Transform positions
    for (int32 i = 0; i < Positions.Count(); i++)
    {
        Vector3::Transform(Positions[i], matrix, Positions[i]);
    }

    // Transform normals and tangents
    for (int32 i = 0; i < Normals.Count(); i++)
    {
        Vector3::TransformNormal(Normals[i], inverseTransposeMatrix, Normals[i]);
        Normals[i].Normalize();
    }
    for (int32 i = 0; i < Tangents.Count(); i++)
    {
        Vector3::TransformNormal(Tangents[i], inverseTransposeMatrix, Tangents[i]);
        Tangents[i].Normalize();
    }
}

void MeshData::Merge(MeshData& other)
{
    // Merge index buffer (and remap indices)
    const uint32 vertexIndexOffset = Positions.Count();
    const int32 indicesStart = Indices.Count();
    const int32 indicesEnd = indicesStart + other.Indices.Count();
    Indices.Add(other.Indices);
    for (int32 i = indicesStart; i < indicesEnd; i++)
    {
        Indices[i] += vertexIndexOffset;
    }

    // Merge vertex buffer
#define MERGE(item, defautValue) \
    if (item.HasItems() && other.item.HasItems()) \
		item.Add(other.item); \
	else if (item.HasItems() && !other.item.HasItems()) \
		for (int32 i = 0; i < other.Positions.Count(); i++) item.Add(defautValue); \
	else if (!item.HasItems() && other.item.HasItems()) \
		for (int32 i = 0; i < Positions.Count(); i++) item.Add(defautValue)
    MERGE(Positions, Vector3::Zero);
    MERGE(UVs, Vector2::Zero);
    MERGE(Normals, Vector3::Forward);
    MERGE(Tangents, Vector3::Right);
    MERGE(LightmapUVs, Vector2::Zero);
    MERGE(Colors, Color::Black);
    MERGE(BlendIndices, Int4::Zero);
    MERGE(BlendWeights, Vector4::Zero);
#undef MERGE

    // Merge blend shapes
    for (auto& otherBlendShape : other.BlendShapes)
    {
        BlendShape* blendShape = nullptr;
        for (int32 i = 0; i < BlendShapes.Count(); i++)
        {
            if (BlendShapes[i].Name == otherBlendShape.Name)
            {
                blendShape = &BlendShapes[i];
                break;
            }
        }
        if (!blendShape)
        {
            blendShape = &BlendShapes.AddOne();
            blendShape->Name = otherBlendShape.Name;
            blendShape->Weight = otherBlendShape.Weight;
        }
        const int32 startIndex = blendShape->Vertices.Count();
        blendShape->Vertices.Add(otherBlendShape.Vertices);
        for (int32 i = startIndex; i < blendShape->Vertices.Count(); i++)
        {
            blendShape->Vertices[i].VertexIndex += vertexIndexOffset;
        }
    }
}

void ModelData::CalculateLODsScreenSizes()
{
    const float autoComputeLodPowerBase = 0.5f;
    const int32 lodCount = LODs.Count();

    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        auto& lod = LODs[lodIndex];

        if (lodIndex == 0)
        {
            lod.ScreenSize = 1.0f;
        }
        else
        {
            lod.ScreenSize = Math::Pow(autoComputeLodPowerBase, (float)lodIndex);
        }
    }

    MinScreenSize = 0.01f;
}

void ModelData::TransformBuffer(const Matrix& matrix)
{
    for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
    {
        auto& lod = LODs[lodIndex];

        for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
        {
            lod.Meshes[meshIndex]->TransformBuffer(matrix);
        }
    }
}

bool ModelData::Pack2ModelHeader(WriteStream* stream) const
{
    // Validate input
    if (stream == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    const int32 lodCount = GetLODsCount();
    if (lodCount == 0 || lodCount > MODEL_MAX_LODS)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }
    if (Materials.IsEmpty())
    {
        Log::ArgumentOutOfRangeException(TEXT("MaterialSlots"), TEXT("Material slots collection cannot be empty."));
        return true;
    }

    // Min Screen Size
    stream->WriteFloat(MinScreenSize);

    // Amount of material slots
    stream->WriteInt32(Materials.Count());

    // For each material slot
    for (int32 materialSlotIndex = 0; materialSlotIndex < Materials.Count(); materialSlotIndex++)
    {
        auto& slot = Materials[materialSlotIndex];

        stream->Write(&slot.AssetID);
        stream->WriteByte(static_cast<byte>(slot.ShadowsMode));
        stream->WriteString(slot.Name, 11);
    }

    // Amount of LODs
    stream->WriteByte(lodCount);

    // For each LOD
    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        auto& lod = LODs[lodIndex];

        // Screen Size
        stream->WriteFloat(lod.ScreenSize);

        // Amount of meshes
        const int32 meshes = lod.Meshes.Count();
        if (meshes == 0 || meshes > MODEL_MAX_MESHES)
        {
            LOG(Warning, "Too many meshes per LOD.");
            return true;
        }
        stream->WriteUint16(meshes);

        // For each mesh
        for (int32 meshIndex = 0; meshIndex < meshes; meshIndex++)
        {
            auto& mesh = *lod.Meshes[meshIndex];

            // Material Slot
            stream->WriteInt32(mesh.MaterialSlotIndex);

            // Box
            BoundingBox box;
            mesh.CalculateBox(box);
            stream->Write(&box);

            // Sphere
            BoundingSphere sphere;
            mesh.CalculateSphere(sphere);
            stream->Write(&sphere);

            // TODO: calculate Sphere and Box at once - make it faster using SSE

            // Has Lightmap UVs
            stream->WriteBool(mesh.LightmapUVs.HasItems());
        }
    }

    return false;
}

bool ModelData::Pack2SkinnedModelHeader(WriteStream* stream) const
{
    // Validate input
    if (stream == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    const int32 lodCount = GetLODsCount();
    if (lodCount == 0 || lodCount > MODEL_MAX_LODS)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }
    if (Materials.IsEmpty())
    {
        Log::ArgumentOutOfRangeException(TEXT("MaterialSlots"), TEXT("Material slots collection cannot be empty."));
        return true;
    }
    if (!HasSkeleton())
    {
        Log::InvalidOperationException(TEXT("Missing skeleton."));
        return true;
    }

    // Min Screen Size
    stream->WriteFloat(MinScreenSize);

    // Amount of material slots
    stream->WriteInt32(Materials.Count());

    // For each material slot
    for (int32 materialSlotIndex = 0; materialSlotIndex < Materials.Count(); materialSlotIndex++)
    {
        auto& slot = Materials[materialSlotIndex];

        stream->Write(&slot.AssetID);
        stream->WriteByte(static_cast<byte>(slot.ShadowsMode));
        stream->WriteString(slot.Name, 11);
    }

    // Amount of LODs
    stream->WriteByte(lodCount);

    // For each LOD
    for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
    {
        auto& lod = LODs[lodIndex];

        // Screen Size
        stream->WriteFloat(lod.ScreenSize);

        // Amount of meshes
        const int32 meshes = lod.Meshes.Count();
        if (meshes > MODEL_MAX_MESHES)
        {
            LOG(Warning, "Too many meshes per LOD.");
            return true;
        }
        stream->WriteUint16(meshes);

        // For each mesh
        for (int32 meshIndex = 0; meshIndex < meshes; meshIndex++)
        {
            auto& mesh = *lod.Meshes[meshIndex];

            // Material Slot
            stream->WriteInt32(mesh.MaterialSlotIndex);

            // Box
            BoundingBox box;
            mesh.CalculateBox(box);
            stream->Write(&box);

            // Sphere
            BoundingSphere sphere;
            mesh.CalculateSphere(sphere);
            stream->Write(&sphere);

            // TODO: calculate Sphere and Box at once - make it faster using SSE

            // Blend Shapes
            const int32 blendShapes = mesh.BlendShapes.Count();
            stream->WriteUint16(blendShapes);
            for (int32 blendShapeIndex = 0; blendShapeIndex < blendShapes; blendShapeIndex++)
            {
                auto& blendShape = mesh.BlendShapes[blendShapeIndex];
                stream->WriteString(blendShape.Name, 13);
                stream->WriteFloat(blendShape.Weight);
            }
        }
    }

    // Skeleton
    {
        stream->WriteInt32(Skeleton.Nodes.Count());

        // For each node
        for (int32 nodeIndex = 0; nodeIndex < Skeleton.Nodes.Count(); nodeIndex++)
        {
            auto& node = Skeleton.Nodes[nodeIndex];

            stream->Write(&node.ParentIndex);
            stream->Write(&node.LocalTransform);
            stream->WriteString(node.Name, 71);
        }

        stream->WriteInt32(Skeleton.Bones.Count());

        // For each bone
        for (int32 boneIndex = 0; boneIndex < Skeleton.Bones.Count(); boneIndex++)
        {
            auto& bone = Skeleton.Bones[boneIndex];

            stream->Write(&bone.ParentIndex);
            stream->Write(&bone.NodeIndex);
            stream->Write(&bone.LocalTransform);
            stream->Write(&bone.OffsetMatrix);
        }
    }

    return false;
}

bool ModelData::Pack2AnimationHeader(WriteStream* stream) const
{
    // Validate input
    if (stream == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    if (Animation.Duration <= ZeroTolerance || Animation.FramesPerSecond <= ZeroTolerance)
    {
        Log::InvalidOperationException(TEXT("Invalid animation duration."));
        return true;
    }
    if (Animation.Channels.IsEmpty())
    {
        Log::ArgumentOutOfRangeException(TEXT("Channels"), TEXT("Animation channels collection cannot be empty."));
        return true;
    }

    // Info
    stream->WriteInt32(100); // Header version (for fast version upgrades without serialization format change)
    stream->WriteDouble(Animation.Duration);
    stream->WriteDouble(Animation.FramesPerSecond);
    stream->WriteBool(Animation.EnableRootMotion);
    stream->WriteString(Animation.RootNodeName, 13);

    // Animation channels
    stream->WriteInt32(Animation.Channels.Count());
    for (int32 i = 0; i < Animation.Channels.Count(); i++)
    {
        auto& anim = Animation.Channels[i];

        stream->WriteString(anim.NodeName, 172);
        Serialization::Serialize(*stream, anim.Position);
        Serialization::Serialize(*stream, anim.Rotation);
        Serialization::Serialize(*stream, anim.Scale);
    }

    return false;
}
