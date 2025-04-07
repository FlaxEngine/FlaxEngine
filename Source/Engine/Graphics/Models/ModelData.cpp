// Copyright (c) Wojciech Figat. All rights reserved.

#include "ModelData.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Animations/CurveSerialization.h"
#include "Engine/Serialization/WriteStream.h"

void MeshData::Clear()
{
    MaterialSlotIndex = 0;
    NodeIndex = 0;
    Positions.Clear();
    Indices.Clear();
    UVs.Clear();
    Normals.Clear();
    Tangents.Clear();
    BitangentSigns.Clear();
    Colors.Clear();
    BlendIndices.Clear();
    BlendWeights.Clear();
    BlendShapes.Clear();
}

void MeshData::EnsureCapacity(int32 vertices, int32 indices, bool preserveContents, bool withColors, bool withSkin, int32 texcoords)
{
    Positions.EnsureCapacity(vertices, preserveContents);
    Indices.EnsureCapacity(indices, preserveContents);
    UVs.Resize(texcoords);
    for (auto& channel : UVs)
        channel.EnsureCapacity(vertices, preserveContents);
    Normals.EnsureCapacity(vertices, preserveContents);
    Tangents.EnsureCapacity(vertices, preserveContents);
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
    BitangentSigns.Swap(other.BitangentSigns);
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
    BitangentSigns.Resize(0);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
void MeshData::InitFromModelVertices(ModelVertex19* vertices, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(2);
    UVs[0].Resize(verticesCount, false);
    UVs[1].Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    BitangentSigns.Resize(0);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vertices->Position;
        UVs[0][i] = vertices->TexCoord.ToFloat2();
        UVs[1][i] = vertices->LightmapUVs.ToFloat2();
        Normals[i] = vertices->Normal.ToFloat3() * 2.0f - 1.0f;
        Tangents[i] = vertices->Tangent.ToFloat3() * 2.0f - 1.0f;
        Colors[i] = Color(vertices->Color);

        vertices++;
    }
}

void MeshData::InitFromModelVertices(VB0ElementType18* vb0, VB1ElementType18* vb1, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(2);
    UVs[0].Resize(verticesCount, false);
    UVs[1].Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    BitangentSigns.Resize(0);
    Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vb0->Position;
        UVs[0][i] = vb1->TexCoord.ToFloat2();
        UVs[1][i] = vb1->LightmapUVs.ToFloat2();
        Normals[i] = vb1->Normal.ToFloat3() * 2.0f - 1.0f;
        Tangents[i] = vb1->Tangent.ToFloat3() * 2.0f - 1.0f;

        vb0++;
        vb1++;
    }
}

void MeshData::InitFromModelVertices(VB0ElementType18* vb0, VB1ElementType18* vb1, VB2ElementType18* vb2, uint32 verticesCount)
{
    Positions.Resize(verticesCount, false);
    UVs.Resize(2);
    UVs[0].Resize(verticesCount, false);
    UVs[1].Resize(verticesCount, false);
    Normals.Resize(verticesCount, false);
    Tangents.Resize(verticesCount, false);
    BitangentSigns.Resize(0);
    if (vb2)
        Colors.Resize(verticesCount, false);
    else
        Colors.Resize(0);
    BlendIndices.Resize(0);
    BlendWeights.Resize(0);
    BlendShapes.Resize(0);

    for (uint32 i = 0; i < verticesCount; i++)
    {
        Positions[i] = vb0->Position;
        UVs[0][i] = vb1->TexCoord.ToFloat2();
        UVs[1][i] = vb1->LightmapUVs.ToFloat2();
        Normals[i] = vb1->Normal.ToFloat3() * 2.0f - 1.0f;
        Tangents[i] = vb1->Tangent.ToFloat3() * 2.0f - 1.0f;
        if (vb2)
        {
            Colors[i] = Color(vb2->Color);
            vb2++;
        }

        vb0++;
        vb1++;
    }
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

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

void MeshData::CalculateBox(BoundingBox& result) const
{
    if (Positions.HasItems())
        BoundingBox::FromPoints(Positions.Get(), Positions.Count(), result);
    else
        result = BoundingBox::Zero;
}

void MeshData::CalculateSphere(BoundingSphere& result) const
{
    if (Positions.HasItems())
        BoundingSphere::FromPoints(Positions.Get(), Positions.Count(), result);
    else
        result = BoundingSphere::Empty;
}

void MeshData::CalculateBounds(BoundingBox& box, BoundingSphere& sphere) const
{
    if (Positions.HasItems())
    {
        // Merged code of BoundingBox::FromPoints and BoundingSphere::FromPoints within a single loop
        const Float3* points = Positions.Get();
        const int32 pointsCount = Positions.Count();
        Float3 min = points[0];
        Float3 max = min;
        Float3 center = min;
        for (int32 i = 1; i < pointsCount; i++)
            Float3::Add(points[i], center, center);
        center /= (float)pointsCount;
        float radiusSq = Float3::DistanceSquared(center, min);
        for (int32 i = 1; i < pointsCount; i++)
        {
            Float3::Min(min, points[i], min);
            Float3::Max(max, points[i], max);
            const float distance = Float3::DistanceSquared(center, points[i]);
            if (distance > radiusSq)
                radiusSq = distance;
        }
        box = BoundingBox(min, max);
        sphere = BoundingSphere(center, Math::Sqrt(radiusSq));
    }
    else
    {
        box = BoundingBox::Zero;
        sphere = BoundingSphere::Empty;
    }
}

void MeshData::TransformBuffer(const Matrix& matrix)
{
    // Compute matrix inverse transpose
    Matrix inverseTransposeMatrix;
    Matrix::Invert(matrix, inverseTransposeMatrix);
    Matrix::Transpose(inverseTransposeMatrix, inverseTransposeMatrix);

    // Transform blend shapes
    for (auto& blendShape : BlendShapes)
    {
        const auto vv = blendShape.Vertices.Get();
        for (int32 i = 0; i < blendShape.Vertices.Count(); i++)
        {
            auto& v = vv[i];

            Float3 p = Positions[v.VertexIndex];
            Float3 vp = p + v.PositionDelta;
            Float3::Transform(vp, matrix, vp);
            Float3::Transform(p, matrix, p);
            v.PositionDelta = vp - p;

            Float3 n = Normals[v.VertexIndex];
            Float3 vn = n + v.NormalDelta;
            vn.Normalize();
            Float3::TransformNormal(vn, inverseTransposeMatrix, vn);
            vn.Normalize();
            Float3::TransformNormal(n, inverseTransposeMatrix, n);
            n.Normalize();
            v.NormalDelta = vn - n;
        }
    }

    // Transform positions
    const auto pp = Positions.Get();
    for (int32 i = 0; i < Positions.Count(); i++)
    {
        auto& p = pp[i];
        Float3::Transform(p, matrix, p);
    }

    // Transform normals and tangents
    const auto nn = Normals.Get();
    for (int32 i = 0; i < Normals.Count(); i++)
    {
        auto& n = nn[i];
        Float3::TransformNormal(n, inverseTransposeMatrix, n);
        n.Normalize();
    }
    const auto tt = Tangents.Get();
    for (int32 i = 0; i < Tangents.Count(); i++)
    {
        auto& t = tt[i];
        Float3::TransformNormal(t, inverseTransposeMatrix, t);
        t.Normalize();
    }
}

void MeshData::NormalizeBlendWeights()
{
    ASSERT(Positions.Count() == BlendWeights.Count());
    for (int32 i = 0; i < Positions.Count(); i++)
    {
        Float4& weights = BlendWeights.Get()[i];
        const float sum = weights.SumValues();
        const float invSum = sum > ZeroTolerance ? 1.0f / sum : 0.0f;
        weights *= invSum;
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
#define MERGE(item, defaultValue) \
    if (item.HasItems() && other.item.HasItems()) \
		item.Add(other.item); \
	else if (item.HasItems() && !other.item.HasItems()) \
		for (int32 i = 0; i < other.Positions.Count(); i++) item.Add(defaultValue); \
	else if (!item.HasItems() && other.item.HasItems()) \
		for (int32 i = 0; i < Positions.Count(); i++) item.Add(defaultValue)
    MERGE(Positions, Float3::Zero);
    MERGE(Normals, Float3::Forward);
    MERGE(Tangents, Float3::Right);
    MERGE(BitangentSigns, 1.0f);
    MERGE(Colors, Color::Black);
    MERGE(BlendIndices, Int4::Zero);
    MERGE(BlendWeights, Float4::Zero);
    if (other.UVs.Count() > UVs.Count())
        UVs.Resize(other.UVs.Count());
    for (int32 channelIdx = 0; channelIdx < UVs.Count(); channelIdx++)
    {
        if (other.UVs.Count() <= channelIdx)
        {
            for (int32 i = 0; i < other.Positions.Count(); i++)
                UVs[channelIdx].Add(Float2::Zero);
            continue;
        }
        MERGE(UVs[channelIdx], Float2::Zero);
    }
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

bool MaterialSlotEntry::UsesProperties() const
{
    return Diffuse.Color != Color::White ||
            Diffuse.TextureIndex != -1 ||
            Emissive.Color != Color::Transparent ||
            Emissive.TextureIndex != -1 ||
            !Math::IsOne(Opacity.Value) ||
            Opacity.TextureIndex != -1 ||
            Math::NotNearEqual(Roughness.Value, 0.5f) ||
            Roughness.TextureIndex != -1 ||
            Normals.TextureIndex != -1;
}

float MaterialSlotEntry::ShininessToRoughness(float shininess)
{
    // https://github.com/assimp/assimp/issues/4573
    const float a = -1.0f;
    const float b = 2.0f;
    const float c = (shininess / 100) - 1;
    const float d = b * b - (4 * a * c);
	return (-b + Math::Sqrt(d)) / (2 * a);
}

ModelLodData::~ModelLodData()
{
    Meshes.ClearDelete();
}

BoundingBox ModelLodData::GetBox() const
{
    if (Meshes.IsEmpty())
        return BoundingBox::Empty;
    BoundingBox bounds;
    Meshes[0]->CalculateBox(bounds);
    for (int32 i = 1; i < Meshes.Count(); i++)
    {
        BoundingBox b;
        Meshes[i]->CalculateBox(b);
        BoundingBox::Merge(bounds, b, bounds);
    }
    return bounds;
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
