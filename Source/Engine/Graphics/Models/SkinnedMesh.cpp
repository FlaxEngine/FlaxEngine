// Copyright (c) Wojciech Figat. All rights reserved.

#include "SkinnedMesh.h"
#include "MeshAccessor.h"
#include "MeshDeformation.h"
#include "ModelInstanceEntry.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Threading/Threading.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

GPUVertexLayout* VB0SkinnedElementType2::GetLayout()
{
    return GPUVertexLayout::Get({
        { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32B32_Float },
        { VertexElement::Types::TexCoord, 0, 0, 0, PixelFormat::R16G16_Float },
        { VertexElement::Types::Normal, 0, 0, 0, PixelFormat::R10G10B10A2_UNorm },
        { VertexElement::Types::Tangent, 0, 0, 0, PixelFormat::R10G10B10A2_UNorm },
        { VertexElement::Types::BlendIndices, 0, 0, 0, PixelFormat::R8G8B8A8_UInt },
        { VertexElement::Types::BlendWeights, 0, 0, 0, PixelFormat::R16G16B16A16_Float },
    });
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

namespace
{
    bool UpdateMesh(MeshBase* mesh, uint32 vertexCount, uint32 triangleCount, PixelFormat indexFormat, const Float3* vertices, const void* triangles, const Int4* blendIndices, const Float4* blendWeights, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
    {
        auto model = mesh->GetModelBase();
        CHECK_RETURN(model && model->IsVirtual(), true);
        CHECK_RETURN(triangles && vertices, true);
        MeshAccessor accessor;

        // Index Buffer
        {
            if (accessor.AllocateBuffer(MeshBufferType::Index, triangleCount, indexFormat))
                return true;
            auto indexStream = accessor.Index();
            ASSERT(indexStream.IsLinear(indexFormat));
            indexStream.SetLinear(triangles);
        }

        // Vertex Buffer
        {
            GPUVertexLayout::Elements vb0elements;
            vb0elements.Add({ VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32B32_Float });
            if (normals)
            {
                vb0elements.Add({ VertexElement::Types::Normal, 0, 0, 0, PixelFormat::R10G10B10A2_UNorm });
                if (tangents)
                    vb0elements.Add({ VertexElement::Types::Tangent, 0, 0, 0, PixelFormat::R10G10B10A2_UNorm });
            }
            vb0elements.Add({ VertexElement::Types::BlendIndices, 0, 0, 0, PixelFormat::R8G8B8A8_UInt });
            vb0elements.Add({ VertexElement::Types::BlendWeights, 0, 0, 0, PixelFormat::R8G8B8A8_UNorm });
            if (uvs)
                vb0elements.Add({ VertexElement::Types::TexCoord, 0, 0, 0, PixelFormat::R16G16_Float });
            if (colors)
                vb0elements.Add({ VertexElement::Types::Color, 0, 0, 0, PixelFormat::R8G8B8A8_UNorm });

            GPUVertexLayout* vb0layout = GPUVertexLayout::Get(vb0elements);
            if (accessor.AllocateBuffer(MeshBufferType::Vertex0, vertexCount, vb0layout))
                return true;

            auto positionStream = accessor.Position();
            ASSERT(positionStream.IsLinear(PixelFormat::R32G32B32_Float));
            positionStream.SetLinear(vertices);
            if (normals)
            {
                auto normalStream = accessor.Normal();
                if (tangents)
                {
                    auto tangentStream = accessor.Tangent();
                    for (uint32 i = 0; i < vertexCount; i++)
                    {
                        const Float3 normal = normals[i];
                        const Float3 tangent = tangents[i];
                        Float3 n;
                        Float4 t;
                        RenderTools::CalculateTangentFrame(n, t, normal, tangent);
                        normalStream.SetFloat3(i, n);
                        tangentStream.SetFloat4(i, t);
                    }
                }
                else
                {
                    for (uint32 i = 0; i < vertexCount; i++)
                    {
                        const Float3 normal = normals[i];
                        Float3 n;
                        Float4 t;
                        RenderTools::CalculateTangentFrame(n, t, normal);
                        normalStream.SetFloat3(i, n);
                    }
                }
            }
            {
                auto blendIndicesStream = accessor.BlendIndices();
                auto blendWeightsStream = accessor.BlendWeights();
                for (uint32 i = 0; i < vertexCount; i++)
                {
                    blendIndicesStream.SetFloat4(i, blendIndices[i]);
                    blendWeightsStream.SetFloat4(i, blendWeights[i]);
                }
            }
            if (uvs)
            {
                auto uvsStream = accessor.TexCoord();
                for (uint32 i = 0; i < vertexCount; i++)
                    uvsStream.SetFloat2(i, uvs[i]);
            }
            if (colors)
            {
                auto colorStream = accessor.Color();
                for (uint32 i = 0; i < vertexCount; i++)
                    colorStream.SetFloat4(i, Float4(Color(colors[i]))); // TODO: optimize with direct memory copy
            }
        }

        return accessor.UpdateMesh(mesh);
    }

#if !COMPILE_WITHOUT_CSHARP
    template<typename IndexType>
    bool UpdateMesh(SkinnedMesh* mesh, uint32 vertexCount, uint32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
    {
        ASSERT((uint32)MCore::Array::GetLength(verticesObj) >= vertexCount);
        ASSERT((uint32)MCore::Array::GetLength(trianglesObj) / 3 >= triangleCount);
        auto vertices = MCore::Array::GetAddress<Float3>(verticesObj);
        auto triangles = MCore::Array::GetAddress<IndexType>(trianglesObj);
        const PixelFormat indexFormat = sizeof(IndexType) == 4 ? PixelFormat::R32_UInt : PixelFormat::R16_UInt;
        const auto blendIndices = MCore::Array::GetAddress<Int4>(blendIndicesObj);
        const auto blendWeights = MCore::Array::GetAddress<Float4>(blendWeightsObj);
        const auto normals = normalsObj ? MCore::Array::GetAddress<Float3>(normalsObj) : nullptr;
        const auto tangents = tangentsObj ? MCore::Array::GetAddress<Float3>(tangentsObj) : nullptr;
        const auto uvs = uvObj ? MCore::Array::GetAddress<Float2>(uvObj) : nullptr;
        const auto colors = colorsObj ? MCore::Array::GetAddress<Color32>(colorsObj) : nullptr;
        return UpdateMesh(mesh, vertexCount, triangleCount, indexFormat, vertices, triangles, blendIndices, blendWeights, normals, tangents, uvs, colors);
    }
#endif
}

void SkeletonData::Swap(SkeletonData& other)
{
    Nodes.Swap(other.Nodes);
    Bones.Swap(other.Bones);
}

Transform SkeletonData::GetNodeTransform(int32 nodeIndex) const
{
    const int32 parentIndex = Nodes[nodeIndex].ParentIndex;
    if (parentIndex == -1)
    {
        return Nodes[nodeIndex].LocalTransform;
    }
    const Transform parentTransform = GetNodeTransform(parentIndex);
    return parentTransform.LocalToWorld(Nodes[nodeIndex].LocalTransform);
}

void SkeletonData::SetNodeTransform(int32 nodeIndex, const Transform& value)
{
    const int32 parentIndex = Nodes[nodeIndex].ParentIndex;
    if (parentIndex == -1)
    {
        Nodes[nodeIndex].LocalTransform = value;
        return;
    }
    const Transform parentTransform = GetNodeTransform(parentIndex);
    parentTransform.WorldToLocal(value, Nodes[nodeIndex].LocalTransform);
}

int32 SkeletonData::FindNode(const StringView& name) const
{
    for (int32 i = 0; i < Nodes.Count(); i++)
    {
        if (Nodes[i].Name == name)
            return i;
    }
    return -1;
}

int32 SkeletonData::FindBone(int32 nodeIndex) const
{
    for (int32 i = 0; i < Bones.Count(); i++)
    {
        if (Bones[i].NodeIndex == nodeIndex)
            return i;
    }
    return -1;
}

uint64 SkeletonData::GetMemoryUsage() const
{
    uint64 result = Nodes.Capacity() * sizeof(SkeletonNode) + Bones.Capacity() * sizeof(SkeletonBone);
    for (const auto& e : Nodes)
        result += (e.Name.Length() + 1) * sizeof(Char);
    return result;
}

void SkeletonData::Dispose()
{
    Nodes.Resize(0);
    Bones.Resize(0);
}

bool SkinnedMesh::Load(uint32 vertices, uint32 triangles, const void* vb0, const void* ib, bool use16BitIndexBuffer)
{
    Array<const void*, FixedAllocation<3>> vbData;
    vbData.Add(vb0);
    Array<GPUVertexLayout*, FixedAllocation<3>> vbLayout;
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    vbLayout.Add(VB0SkinnedElementType::GetLayout());
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    return Init(vertices, triangles, vbData, ib, use16BitIndexBuffer, vbLayout);
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const int32* ib)
{
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    return UpdateMesh(vertexCount, triangleCount, vb, ib, false);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const uint32* ib)
{
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    return UpdateMesh(vertexCount, triangleCount, vb, ib, false);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const uint16* ib)
{
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    return UpdateMesh(vertexCount, triangleCount, vb, ib, true);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0SkinnedElementType* vb, const void* ib, bool use16BitIndices)
{
    // Setup GPU resources
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    const bool failed = Load(vertexCount, triangleCount, vb, ib, use16BitIndices);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    if (!failed)
    {
        // Calculate mesh bounds
        BoundingBox bounds;
        BoundingBox::FromPoints((const Float3*)vb, vertexCount, bounds);
        SetBounds(bounds);
    }
    return failed;
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint16* triangles, const Int4* blendIndices, const Float4* blendWeights, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
{
    return ::UpdateMesh(this, vertexCount, triangleCount, PixelFormat::R16_UInt, vertices, triangles, blendIndices, blendWeights, normals, tangents, uvs, colors);
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint32* triangles, const Int4* blendIndices, const Float4* blendWeights, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
{
    return ::UpdateMesh(this, vertexCount, triangleCount, PixelFormat::R16_UInt, vertices, triangles, blendIndices, blendWeights, normals, tangents, uvs, colors);
}

void SkinnedMesh::Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const
{
    const auto& entry = info.Buffer->At(_materialSlotIndex);
    if (!entry.Visible || !IsInitialized())
        return;
    const MaterialSlot& slot = _model->MaterialSlots[_materialSlotIndex];

    // Select material
    MaterialBase* material;
    if (entry.Material && entry.Material->IsLoaded())
        material = entry.Material;
    else if (slot.Material && slot.Material->IsLoaded())
        material = slot.Material;
    else
        material = GPUDevice::Instance->GetDefaultMaterial();
    if (!material || !material->IsSurface())
        return;

    // Check if skip rendering
    const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
    const auto drawModes = info.DrawModes & renderContext.View.Pass & renderContext.View.GetShadowsDrawPassMask(shadowsMode) & material->GetDrawModes();
    if (drawModes == DrawPass::None)
        return;

    // Setup draw call
    DrawCall drawCall;
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    if (info.Deformation)
        info.Deformation->RunDeformers(this, MeshBufferType::Vertex0, drawCall.Geometry.VertexBuffers[0]);
    drawCall.Draw.StartIndex = 0;
    drawCall.Draw.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    drawCall.World = *info.World;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Skinning = info.Skinning;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;

    // Push draw call to the render list
    renderContext.List->AddDrawCall(renderContext, drawModes, StaticFlags::None, drawCall, entry.ReceiveDecals, info.SortOrder);
}

void SkinnedMesh::Draw(const RenderContextBatch& renderContextBatch, const DrawInfo& info, float lodDitherFactor) const
{
    const auto& entry = info.Buffer->At(_materialSlotIndex);
    if (!entry.Visible || !IsInitialized())
        return;
    const MaterialSlot& slot = _model->MaterialSlots[_materialSlotIndex];

    // Select material
    MaterialBase* material;
    if (entry.Material && entry.Material->IsLoaded())
        material = entry.Material;
    else if (slot.Material && slot.Material->IsLoaded())
        material = slot.Material;
    else
        material = GPUDevice::Instance->GetDefaultMaterial();
    if (!material || !material->IsSurface())
        return;

    // Setup draw call
    DrawCall drawCall;
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    if (info.Deformation)
        info.Deformation->RunDeformers(this, MeshBufferType::Vertex0, drawCall.Geometry.VertexBuffers[0]);
    drawCall.Draw.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    drawCall.World = *info.World;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Skinning = info.Skinning;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;

    // Push draw call to the render lists
    const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
    const auto drawModes = info.DrawModes & material->GetDrawModes();
    if (drawModes != DrawPass::None)
        renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, drawModes, StaticFlags::None, shadowsMode, info.Bounds, drawCall, entry.ReceiveDecals, info.SortOrder);
}

void SkinnedMesh::Release()
{
    MeshBase::Release();

    BlendShapes.Clear();
}

#if !COMPILE_WITHOUT_CSHARP

bool SkinnedMesh::UpdateMeshUInt(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
{
    return ::UpdateMesh<uint32>(this, (uint32)vertexCount, (uint32)triangleCount, verticesObj, trianglesObj, blendIndicesObj, blendWeightsObj, normalsObj, tangentsObj, uvObj, colorsObj);
}

bool SkinnedMesh::UpdateMeshUShort(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* blendIndicesObj, const MArray* blendWeightsObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
{
    return ::UpdateMesh<uint16>(this, (uint32)vertexCount, (uint32)triangleCount, verticesObj, trianglesObj, blendIndicesObj, blendWeightsObj, normalsObj, tangentsObj, uvObj, colorsObj);
}

// [Deprecated in v1.10]
enum class InternalBufferType
{
    VB0 = 0,
};

MArray* SkinnedMesh::DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI)
{
    // [Deprecated in v1.10]
    ScopeLock lock(GetModelBase()->Locker);

    // Get vertex buffers data from the mesh (CPU or GPU)
    MeshAccessor accessor;
    MeshBufferType bufferTypes[1] = { MeshBufferType::Vertex0 };
    if (accessor.LoadMesh(this, forceGpu, ToSpan(bufferTypes, 1)))
        return nullptr;
    auto positionStream = accessor.Position();
    auto texCoordStream = accessor.TexCoord();
    auto normalStream = accessor.Normal();
    auto tangentStream = accessor.Tangent();
    auto blendIndicesStream = accessor.BlendIndices();
    auto BlendWeightsStream = accessor.BlendWeights();
    auto count = GetVertexCount();

    // Convert into managed array
    MArray* result = MCore::Array::New(MCore::Type::GetClass(INTERNAL_TYPE_OBJECT_GET(resultType)), count);
    void* managedArrayPtr = MCore::Array::GetAddress(result);
    switch ((InternalBufferType)typeI)
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
    case InternalBufferType::VB0:
        for (int32 i = 0; i < count; i++)
        {
            auto& dst = ((VB0SkinnedElementType*)managedArrayPtr)[i];
            dst.Position = positionStream.GetFloat3(i);
            if (texCoordStream.IsValid())
                dst.TexCoord = texCoordStream.GetFloat2(i);
            if (normalStream.IsValid())
                dst.Normal = normalStream.GetFloat3(i);
            if (tangentStream.IsValid())
                dst.Tangent = tangentStream.GetFloat4(i);
            if (blendIndicesStream.IsValid())
                dst.BlendIndices = Color32(blendIndicesStream.GetFloat4(i));
            if (BlendWeightsStream.IsValid())
                dst.BlendWeights = Half4(BlendWeightsStream.GetFloat4(i));
        }
        break;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    return result;
}

#endif
