// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "BlendShape.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Profiler/ProfilerCPU.h"

BlendShapesInstance::MeshInstance::MeshInstance()
    : IsUsed(false)
    , IsDirty(false)
    , DirtyMinVertexIndex(0)
    , DirtyMaxVertexIndex(MAX_uint32)
    , VertexBuffer(0, sizeof(VB0SkinnedElementType), TEXT("Skinned Mesh Blend Shape"))
{
}

BlendShapesInstance::MeshInstance::~MeshInstance()
{
}

BlendShapesInstance::~BlendShapesInstance()
{
    Meshes.ClearDelete();
}

void BlendShapesInstance::Update(SkinnedModel* skinnedModel)
{
    if (!WeightsDirty)
        return;
    WeightsDirty = false;
    ASSERT(skinnedModel);
    if (skinnedModel->IsVirtual())
    {
        // Blend shapes are not supported for virtual skinned models
        return;
    }
    PROFILE_CPU_NAMED("Update Blend Shapes");

    // Collect used meshes
    for (auto& e : Meshes)
    {
        MeshInstance* instance = e.Value;
        instance->IsUsed = false;
        instance->IsDirty = false;
    }
    for (auto& e : Weights)
    {
        for (auto& lod : skinnedModel->LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (auto& blendShape : mesh.BlendShapes)
                {
                    if (blendShape.Name != e.First)
                        continue;

                    // Setup mesh instance and mark it as used for blending
                    MeshInstance* instance;
                    if (!Meshes.TryGet(&mesh, instance))
                    {
                        instance = New<MeshInstance>();
                        Meshes.Add(&mesh, instance);
                    }
                    instance->IsUsed = true;
                    instance->IsDirty = true;

                    // One blend shape of that name per mesh
                    break;
                }
            }
        }
    }

    // Update all used meshes
    for (auto& e : Meshes)
    {
        MeshInstance& instance = *e.Value;
        if (!instance.IsUsed)
            continue;
        const SkinnedMesh* mesh = e.Key;

        // Get skinned mesh vertex buffer data (original, cached on CPU)
        BytesContainer vertexBuffer;
        int32 vertexCount;
        if (mesh->DownloadDataCPU(MeshBufferType::Vertex0, vertexBuffer, vertexCount))
        {
            // Don't use this mesh if failed to get it's vertices data
            instance.IsUsed = false;
            continue;
        }

        // Estimate the range of the vertices to modify by the currently active blend shapes
        uint32 minVertexIndex = MAX_uint32, maxVertexIndex = 0;
        bool useNormals = false;
        Array<Pair<const BlendShape&, const float>, InlinedAllocation<32>> blendShapes;
        for (const BlendShape& blendShape : mesh->BlendShapes)
        {
            for (auto& q : Weights)
            {
                if (q.First == blendShape.Name)
                {
                    const float weight = q.Second * blendShape.Weight;
                    blendShapes.Add(Pair<const BlendShape&, const float>(blendShape, weight));
                    minVertexIndex = Math::Min(minVertexIndex, blendShape.MinVertexIndex);
                    maxVertexIndex = Math::Max(maxVertexIndex, blendShape.MaxVertexIndex);
                    useNormals |= blendShape.UseNormals;
                    break;
                }
            }
        }

        // Initialize the dynamic vertex buffer data (use the dirty range from the previous update to be cleared with initial data)
        instance.VertexBuffer.Data.Resize(vertexBuffer.Length());
        const uint32 dirtyVertexDataStart = instance.DirtyMinVertexIndex * sizeof(VB0SkinnedElementType);
        const uint32 dirtyVertexDataLength = Math::Min<uint32>(instance.DirtyMaxVertexIndex - instance.DirtyMinVertexIndex, vertexCount) * sizeof(VB0SkinnedElementType);
        Platform::MemoryCopy(instance.VertexBuffer.Data.Get() + dirtyVertexDataStart, vertexBuffer.Get() + dirtyVertexDataStart, dirtyVertexDataLength);

        // Blend all blend shapes
        auto data = (VB0SkinnedElementType*)instance.VertexBuffer.Data.Get();
        for (const auto& q : blendShapes)
        {
            // TODO: use SIMD
            if (useNormals)
            {
                for (int32 i = 0; i < q.First.Vertices.Count(); i++)
                {
                    const BlendShapeVertex& blendShapeVertex = q.First.Vertices[i];
                    ASSERT_LOW_LAYER(blendShapeVertex.VertexIndex < (uint32)vertexCount);
                    VB0SkinnedElementType& vertex = *(data + blendShapeVertex.VertexIndex);
                    vertex.Position = vertex.Position + blendShapeVertex.PositionDelta * q.Second;
                    Vector3 normal = (vertex.Normal.ToVector3() * 2.0f - 1.0f) + blendShapeVertex.NormalDelta;
                    vertex.Normal = normal * 0.5f + 0.5f;
                }
            }
            else
            {
                for (int32 i = 0; i < q.First.Vertices.Count(); i++)
                {
                    const BlendShapeVertex& blendShapeVertex = q.First.Vertices[i];
                    ASSERT_LOW_LAYER(blendShapeVertex.VertexIndex < (uint32)vertexCount);
                    VB0SkinnedElementType& vertex = *(data + blendShapeVertex.VertexIndex);
                    vertex.Position = vertex.Position + blendShapeVertex.PositionDelta * q.Second;
                }
            }
        }

        if (useNormals)
        {
            // Normalize normal vectors and rebuild tangent frames (tangent frame is in range [-1;1] but packed to [0;1] range)
            // TODO: use SIMD
            for (uint32 vertexIndex = minVertexIndex; vertexIndex <= maxVertexIndex; vertexIndex++)
            {
                VB0SkinnedElementType& vertex = *(data + vertexIndex);

                Vector3 normal = vertex.Normal.ToVector3() * 2.0f - 1.0f;
                normal.Normalize();
                vertex.Normal = normal * 0.5f + 0.5f;

                Vector3 tangent = vertex.Tangent.ToVector3() * 2.0f - 1.0f;
                tangent = tangent - ((tangent | normal) * normal);
                tangent.Normalize();
                const auto tangentSign = vertex.Tangent.W;
                vertex.Tangent = tangent * 0.5f + 0.5f;
                vertex.Tangent.W = tangentSign;
            }
        }

        // Mark as dirty to be flushed before next rendering
        instance.IsDirty = true;
        instance.DirtyMinVertexIndex = minVertexIndex;
        instance.DirtyMaxVertexIndex = maxVertexIndex;
    }
}

void BlendShapesInstance::Clear()
{
    Meshes.ClearDelete();
    Weights.Clear();
    WeightsDirty = false;
}
