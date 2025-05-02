// Copyright (c) Wojciech Figat. All rights reserved.

#include "Mesh.h"
#include "MeshAccessor.h"
#include "MeshDeformation.h"
#include "ModelInstanceEntry.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Renderer/GBufferPass.h"
#endif

PRAGMA_DISABLE_DEPRECATION_WARNINGS
GPUVertexLayout* VB0ElementType18::GetLayout()
{
    return GPUVertexLayout::Get({
        { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32B32_Float },
    });
}

GPUVertexLayout* VB1ElementType18::GetLayout()
{
    return GPUVertexLayout::Get({
        { VertexElement::Types::TexCoord, 1, 0, 0, PixelFormat::R16G16_Float },
        { VertexElement::Types::Normal, 1, 0, 0, PixelFormat::R10G10B10A2_UNorm },
        { VertexElement::Types::Tangent, 1, 0, 0, PixelFormat::R10G10B10A2_UNorm },
        { VertexElement::Types::TexCoord1, 1, 0, 0, PixelFormat::R16G16_Float },
    });
}

GPUVertexLayout* VB2ElementType18::GetLayout()
{
    return GPUVertexLayout::Get({
        { VertexElement::Types::Color, 2, 0, 0, PixelFormat::R8G8B8A8_UNorm },
    });
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

namespace
{
    bool UpdateMesh(MeshBase* mesh, uint32 vertexCount, uint32 triangleCount, PixelFormat indexFormat, const Float3* vertices, const void* triangles, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
    {
        auto model = mesh->GetModelBase();
        CHECK_RETURN(model && model->IsVirtual(), true);
        CHECK_RETURN(triangles && vertices, true);
        MeshAccessor accessor;
        
        // Index Buffer
        {
            if (accessor.AllocateBuffer(MeshBufferType::Index, triangleCount * 3, indexFormat))
                return true;
            auto indexStream = accessor.Index();
            ASSERT(indexStream.IsLinear(indexFormat));
            indexStream.SetLinear(triangles);
        }

        // Vertex Buffer 0 (position-only)
        {
            GPUVertexLayout* vb0layout = GPUVertexLayout::Get({ { VertexElement::Types::Position, 0, 0, 0, PixelFormat::R32G32B32_Float } });
            if (accessor.AllocateBuffer(MeshBufferType::Vertex0, vertexCount, vb0layout))
                return true;
            auto positionStream = accessor.Position();
            ASSERT(positionStream.IsLinear(PixelFormat::R32G32B32_Float));
            positionStream.SetLinear(vertices);
        }

        // Vertex Buffer 1 (general purpose components)
        GPUVertexLayout::Elements vb1elements;
        if (normals)
        {
            vb1elements.Add({ VertexElement::Types::Normal, 1, 0, 0, PixelFormat::R10G10B10A2_UNorm });
            if (tangents)
                vb1elements.Add({ VertexElement::Types::Tangent, 1, 0, 0, PixelFormat::R10G10B10A2_UNorm });
        }
        if (uvs)
            vb1elements.Add({ VertexElement::Types::TexCoord, 1, 0, 0, PixelFormat::R16G16_Float });
        if (vb1elements.HasItems())
        {
            GPUVertexLayout* vb1layout = GPUVertexLayout::Get(vb1elements);
            if (accessor.AllocateBuffer(MeshBufferType::Vertex1, vertexCount, vb1layout))
                return true;
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
            if (uvs)
            {
                auto uvsStream = accessor.TexCoord();
                for (uint32 i = 0; i < vertexCount; i++)
                    uvsStream.SetFloat2(i, uvs[i]);
            }
        }

        // Vertex Buffer 2 (color-only)
        if (colors)
        {
            GPUVertexLayout* vb2layout = GPUVertexLayout::Get({{ VertexElement::Types::Color, 2, 0, 0, PixelFormat::R8G8B8A8_UNorm }});
            if (accessor.AllocateBuffer(MeshBufferType::Vertex2, vertexCount, vb2layout))
                return true;
            auto colorStream = accessor.Color();
            ASSERT(colorStream.IsLinear(PixelFormat::R8G8B8A8_UNorm));
            colorStream.SetLinear(colors);
        }

        return accessor.UpdateMesh(mesh);
    }

#if !COMPILE_WITHOUT_CSHARP
    template<typename IndexType>
    bool UpdateMesh(Mesh* mesh, uint32 vertexCount, uint32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
    {
        ASSERT((uint32)MCore::Array::GetLength(verticesObj) >= vertexCount);
        ASSERT((uint32)MCore::Array::GetLength(trianglesObj) / 3 >= triangleCount);
        auto vertices = MCore::Array::GetAddress<Float3>(verticesObj);
        auto triangles = MCore::Array::GetAddress<IndexType>(trianglesObj);
        const PixelFormat indexFormat = sizeof(IndexType) == 4 ? PixelFormat::R32_UInt : PixelFormat::R16_UInt;
        const auto normals = normalsObj ? MCore::Array::GetAddress<Float3>(normalsObj) : nullptr;
        const auto tangents = tangentsObj ? MCore::Array::GetAddress<Float3>(tangentsObj) : nullptr;
        const auto uvs = uvObj ? MCore::Array::GetAddress<Float2>(uvObj) : nullptr;
        const auto colors = colorsObj ? MCore::Array::GetAddress<Color32>(colorsObj) : nullptr;
        return UpdateMesh(mesh, vertexCount, triangleCount, indexFormat, vertices, triangles, normals, tangents, uvs, colors);
    }
#endif
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const uint32* ib)
{
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    return UpdateMesh(vertexCount, triangleCount, vb0, vb1, vb2, ib, false);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const uint16* ib)
{
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    return UpdateMesh(vertexCount, triangleCount, vb0, vb1, vb2, ib, true);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const void* ib, bool use16BitIndices)
{
    Release();

    // Setup GPU resources
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    const bool failed = Load(vertexCount, triangleCount, vb0, vb1, vb2, ib, use16BitIndices);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    if (!failed)
    {
        // Calculate mesh bounds
        BoundingBox bounds;
        BoundingBox::FromPoints((const Float3*)vb0, vertexCount, bounds);
        SetBounds(bounds);
    }
    return failed;
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint16* triangles, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
{
    return ::UpdateMesh(this, vertexCount, triangleCount, PixelFormat::R16_UInt, vertices, triangles, normals, tangents, uvs, colors);
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint32* triangles, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
{
    return ::UpdateMesh(this, vertexCount, triangleCount, PixelFormat::R16_UInt, vertices, triangles, normals, tangents, uvs, colors);
}

bool Mesh::Load(uint32 vertices, uint32 triangles, const void* vb0, const void* vb1, const void* vb2, const void* ib, bool use16BitIndexBuffer)
{
    Array<const void*, FixedAllocation<3>> vbData;
    vbData.Add(vb0);
    if (vb1)
        vbData.Add(vb1);
    if (vb2)
        vbData.Add(vb2);
    Array<GPUVertexLayout*, FixedAllocation<3>> vbLayout;
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    vbLayout.Add(VB0ElementType::GetLayout());
    vbLayout.Add(VB1ElementType::GetLayout());
    vbLayout.Add(VB2ElementType::GetLayout());
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    return Init(vertices, triangles, vbData, ib, use16BitIndexBuffer, vbLayout);
}

void Mesh::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, StaticFlags flags, bool receiveDecals, DrawPass drawModes, float perInstanceRandom, int8 sortOrder) const
{
    if (!material || !material->IsSurface() || !IsInitialized())
        return;
    drawModes &= material->GetDrawModes();
    if (drawModes == DrawPass::None)
        return;

    // Setup draw call
    DrawCall drawCall;
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    drawCall.Draw.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    drawCall.World = world;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)_sphere.Radius * drawCall.World.GetScaleVector().GetAbsolute().MaxValue();
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = world;
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = perInstanceRandom;
#if USE_EDITOR
    const ViewMode viewMode = renderContext.View.Mode;
    if (viewMode == ViewMode::LightmapUVsDensity || viewMode == ViewMode::LODPreview)
        GBufferPass::AddIndexBufferToModelLOD(_indexBuffer, &((Model*)_model)->LODs[_lodIndex]);
#endif

    // Push draw call to the render list
    renderContext.List->AddDrawCall(renderContext, drawModes, flags, drawCall, receiveDecals, sortOrder);
}

void Mesh::Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const
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
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    if (info.Deformation)
    {
        info.Deformation->RunDeformers(this, MeshBufferType::Vertex0, drawCall.Geometry.VertexBuffers[0]);
        info.Deformation->RunDeformers(this, MeshBufferType::Vertex1, drawCall.Geometry.VertexBuffers[1]);
    }
    if (info.VertexColors && info.VertexColors[_lodIndex])
    {
        // TODO: cache vertexOffset within the model LOD per-mesh
        uint32 vertexOffset = 0;
        for (int32 meshIndex = 0; meshIndex < _index; meshIndex++)
            vertexOffset += ((Model*)_model)->LODs[_lodIndex].Meshes[meshIndex].GetVertexCount();
        drawCall.Geometry.VertexBuffers[2] = info.VertexColors[_lodIndex];
        drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(Color32);
    }
    drawCall.Draw.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    drawCall.World = *info.World;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Lightmap = (info.Flags & StaticFlags::Lightmap) != StaticFlags::None ? info.Lightmap : nullptr;
    drawCall.Surface.LightmapUVsArea = info.LightmapUVs ? *info.LightmapUVs : Rectangle::Empty;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;
#if USE_EDITOR
    const ViewMode viewMode = renderContext.View.Mode;
    if (viewMode == ViewMode::LightmapUVsDensity || viewMode == ViewMode::LODPreview)
        GBufferPass::AddIndexBufferToModelLOD(_indexBuffer, &((Model*)_model)->LODs[_lodIndex]);
    if (viewMode == ViewMode::LightmapUVsDensity)
        drawCall.Surface.LODDitherFactor = info.LightmapScale; // See LightmapUVsDensityMaterialShader
#endif

    // Push draw call to the render list
    renderContext.List->AddDrawCall(renderContext, drawModes, info.Flags, drawCall, entry.ReceiveDecals, info.SortOrder);
}

void Mesh::Draw(const RenderContextBatch& renderContextBatch, const DrawInfo& info, float lodDitherFactor) const
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
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    if (info.Deformation)
    {
        info.Deformation->RunDeformers(this, MeshBufferType::Vertex0, drawCall.Geometry.VertexBuffers[0]);
        info.Deformation->RunDeformers(this, MeshBufferType::Vertex1, drawCall.Geometry.VertexBuffers[1]);
    }
    if (info.VertexColors && info.VertexColors[_lodIndex])
    {
        // TODO: cache vertexOffset within the model LOD per-mesh
        uint32 vertexOffset = 0;
        for (int32 meshIndex = 0; meshIndex < _index; meshIndex++)
            vertexOffset += ((Model*)_model)->LODs[_lodIndex].Meshes[meshIndex].GetVertexCount();
        drawCall.Geometry.VertexBuffers[2] = info.VertexColors[_lodIndex];
        drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(Color32);
    }
    drawCall.Draw.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    drawCall.World = *info.World;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.ObjectRadius = (float)info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Lightmap = (info.Flags & StaticFlags::Lightmap) != StaticFlags::None ? info.Lightmap : nullptr;
    drawCall.Surface.LightmapUVsArea = info.LightmapUVs ? *info.LightmapUVs : Rectangle::Empty;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = RenderTools::GetWorldDeterminantSign(drawCall.World);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;
#if USE_EDITOR
    const ViewMode viewMode = renderContextBatch.GetMainContext().View.Mode;
    if (viewMode == ViewMode::LightmapUVsDensity || viewMode == ViewMode::LODPreview)
        GBufferPass::AddIndexBufferToModelLOD(_indexBuffer, &((Model*)_model)->LODs[_lodIndex]);
    if (viewMode == ViewMode::LightmapUVsDensity)
        drawCall.Surface.LODDitherFactor = info.LightmapScale; // See LightmapUVsDensityMaterialShader
#endif

    // Push draw call to the render lists
    const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
    const auto drawModes = info.DrawModes & material->GetDrawModes();
    if (drawModes != DrawPass::None)
        renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, drawModes, info.Flags, shadowsMode, info.Bounds, drawCall, entry.ReceiveDecals, info.SortOrder);
}

bool Mesh::Init(uint32 vertices, uint32 triangles, const Array<const void*, FixedAllocation<3>>& vbData, const void* ibData, bool use16BitIndexBuffer, Array<GPUVertexLayout*, FixedAllocation<3>> vbLayout)
{
    // Inject lightmap uv coordinate index into the vertex layout of one of the buffers
    if (LightmapUVsIndex != -1)
    {
        const auto vertexElementType = (VertexElement::Types)((int32)VertexElement::Types::TexCoord0 + LightmapUVsIndex);
        for (int32 vbIndex = 0; vbIndex < vbLayout.Count(); vbIndex++)
        {
            // Check if layout contains lightmap uvs texcoords channel
            GPUVertexLayout* layout = vbLayout[vbIndex];
            VertexElement element = layout->FindElement(vertexElementType);
            if (element.Type == vertexElementType)
            {
                // Ensure element doesn't exist in this layout
                if (layout->FindElement(VertexElement::Types::Lightmap).Format == PixelFormat::Unknown)
                {
                    GPUVertexLayout::Elements elements = layout->GetElements();
                    element.Type = VertexElement::Types::Lightmap;
                    elements.Add(element);
                    layout = GPUVertexLayout::Get(elements, true);
                    vbLayout[vbIndex] = layout;
                }
                break;
            }
        }
    }

    if (MeshBase::Init(vertices, triangles, vbData, ibData, use16BitIndexBuffer, vbLayout))
        return true;

    auto model = (Model*)_model;
    if (model)
        model->LODs[_lodIndex]._verticesCount += _vertices;
    return false;
}

void Mesh::Release()
{
    auto model = (Model*)_model;
    if (model)
        model->LODs[_lodIndex]._verticesCount -= _vertices;

    MeshBase::Release();
}

#if !COMPILE_WITHOUT_CSHARP

bool Mesh::UpdateMeshUInt(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
{
    return ::UpdateMesh<uint32>(this, (uint32)vertexCount, (uint32)triangleCount, verticesObj, trianglesObj, normalsObj, tangentsObj, uvObj, colorsObj);
}

bool Mesh::UpdateMeshUShort(int32 vertexCount, int32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
{
    return ::UpdateMesh<uint16>(this, (uint32)vertexCount, (uint32)triangleCount, verticesObj, trianglesObj, normalsObj, tangentsObj, uvObj, colorsObj);
}

// [Deprecated in v1.10]
enum class InternalBufferType
{
    VB0 = 0,
    VB1 = 1,
    VB2 = 2,
};

MArray* Mesh::DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI)
{
    // [Deprecated in v1.10]
    ScopeLock lock(GetModelBase()->Locker);

    // Get vertex buffers data from the mesh (CPU or GPU)
    MeshAccessor accessor;
    MeshBufferType bufferTypes[1] = { MeshBufferType::Vertex0 };
    switch ((InternalBufferType)typeI)
    {
    case InternalBufferType::VB1:
        bufferTypes[0] = MeshBufferType::Vertex1;
        break;
    case InternalBufferType::VB2:
        bufferTypes[0] = MeshBufferType::Vertex2;
        break;
    }
    if (accessor.LoadMesh(this, forceGpu, ToSpan(bufferTypes, 1)))
        return nullptr;
    auto positionStream = accessor.Position();
    auto texCoordStream = accessor.TexCoord();
    auto lightmapUVsStream = accessor.TexCoord(1);
    auto normalStream = accessor.Normal();
    auto tangentStream = accessor.Tangent();
    auto colorStream = accessor.Color();
    auto count = GetVertexCount();

    // Convert into managed array
    MArray* result = MCore::Array::New(MCore::Type::GetClass(INTERNAL_TYPE_OBJECT_GET(resultType)), count);
    void* managedArrayPtr = MCore::Array::GetAddress(result);
    switch ((InternalBufferType)typeI)
    {
PRAGMA_DISABLE_DEPRECATION_WARNINGS
    case InternalBufferType::VB0:
        CHECK_RETURN(positionStream.IsValid(), nullptr);
        for (int32 i = 0; i < count; i++)
        {
            auto& dst = ((VB0ElementType*)managedArrayPtr)[i];
            dst.Position = positionStream.GetFloat3(i);
        }
        break;
    case InternalBufferType::VB1:
        for (int32 i = 0; i < count; i++)
        {
            auto& dst = ((VB1ElementType*)managedArrayPtr)[i];
            if (texCoordStream.IsValid())
                dst.TexCoord = texCoordStream.GetFloat2(i);
            if (normalStream.IsValid())
                dst.Normal = normalStream.GetFloat3(i);
            if (tangentStream.IsValid())
                dst.Tangent = tangentStream.GetFloat4(i);
            if (lightmapUVsStream.IsValid())
                dst.LightmapUVs = lightmapUVsStream.GetFloat2(i);
        }
        break;
    case InternalBufferType::VB2:
        if (colorStream.IsValid())
        {
            for (int32 i = 0; i < count; i++)
            {
                auto& dst = ((VB2ElementType*)managedArrayPtr)[i];
                    dst.Color = Color32(colorStream.GetFloat4(i));
            }
        }
        break;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    return result;
}

#if USE_EDITOR

Array<Vector3> Mesh::GetCollisionProxyPoints() const
{
    PROFILE_CPU();
    Array<Vector3> result;
#if USE_PRECISE_MESH_INTERSECTS
    for (int32 i = 0; i < _collisionProxy.Triangles.Count(); i++)
    {
        auto triangle = _collisionProxy.Triangles.Get()[i];
        result.Add(triangle.V0);
        result.Add(triangle.V1);
        result.Add(triangle.V2);
    }
#endif
    return result;
}

#endif

#endif
