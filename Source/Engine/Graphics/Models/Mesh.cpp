// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Mesh.h"
#include "MeshDeformation.h"
#include "ModelInstanceEntry.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Task.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Renderer/GBufferPass.h"
#endif

namespace
{
    template<typename IndexType>
    bool UpdateMesh(Mesh* mesh, uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const IndexType* triangles, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
    {
        auto model = mesh->GetModel();
        CHECK_RETURN(model && model->IsVirtual(), true);
        CHECK_RETURN(triangles && vertices, true);

        // Pack mesh data into vertex buffers
        Array<VB1ElementType> vb1;
        Array<VB2ElementType> vb2;
        vb1.Resize(vertexCount);
        if (normals)
        {
            if (tangents)
            {
                for (uint32 i = 0; i < vertexCount; i++)
                {
                    const Float3 normal = normals[i];
                    const Float3 tangent = tangents[i];
                    auto& v = vb1.Get()[i];
                    RenderTools::CalculateTangentFrame(v.Normal, v.Tangent, normal, tangent);
                }
            }
            else
            {
                for (uint32 i = 0; i < vertexCount; i++)
                {
                    const Float3 normal = normals[i];
                    auto& v = vb1.Get()[i];
                    RenderTools::CalculateTangentFrame(v.Normal, v.Tangent, normal);
                }
            }
        }
        else
        {
            // Set default tangent frame
            const auto n = Float1010102(Float3::UnitZ);
            const auto t = Float1010102(Float3::UnitX);
            for (uint32 i = 0; i < vertexCount; i++)
            {
                vb1.Get()[i].Normal = n;
                vb1.Get()[i].Tangent = t;
            }
        }
        if (uvs)
        {
            for (uint32 i = 0; i < vertexCount; i++)
                vb1.Get()[i].TexCoord = Half2(uvs[i]);
        }
        else
        {
            auto v = Half2::Zero;
            for (uint32 i = 0; i < vertexCount; i++)
                vb1.Get()[i].TexCoord = v;
        }
        {
            auto v = Half2::Zero;
            for (uint32 i = 0; i < vertexCount; i++)
                vb1.Get()[i].LightmapUVs = v;
        }
        if (colors)
        {
            vb2.Resize(vertexCount);
            for (uint32 i = 0; i < vertexCount; i++)
                vb2.Get()[i].Color = colors[i];
        }

        return mesh->UpdateMesh(vertexCount, triangleCount, (VB0ElementType*)vertices, vb1.Get(), vb2.HasItems() ? vb2.Get() : nullptr, triangles);
    }

#if !COMPILE_WITHOUT_CSHARP
    template<typename IndexType>
    bool UpdateMesh(Mesh* mesh, uint32 vertexCount, uint32 triangleCount, const MArray* verticesObj, const MArray* trianglesObj, const MArray* normalsObj, const MArray* tangentsObj, const MArray* uvObj, const MArray* colorsObj)
    {
        ASSERT((uint32)MCore::Array::GetLength(verticesObj) >= vertexCount);
        ASSERT((uint32)MCore::Array::GetLength(trianglesObj) / 3 >= triangleCount);
        auto vertices = MCore::Array::GetAddress<Float3>(verticesObj);
        auto triangles = MCore::Array::GetAddress<IndexType>(trianglesObj);
        const auto normals = normalsObj ? MCore::Array::GetAddress<Float3>(normalsObj) : nullptr;
        const auto tangents = tangentsObj ? MCore::Array::GetAddress<Float3>(tangentsObj) : nullptr;
        const auto uvs = uvObj ? MCore::Array::GetAddress<Float2>(uvObj) : nullptr;
        const auto colors = colorsObj ? MCore::Array::GetAddress<Color32>(colorsObj) : nullptr;
        return UpdateMesh<IndexType>(mesh, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
    }

    template<typename IndexType>
    bool UpdateTriangles(Mesh* mesh, int32 triangleCount, const MArray* trianglesObj)
    {
        const auto model = mesh->GetModel();
        ASSERT(model && model->IsVirtual() && trianglesObj);

        // Get buffer data
        ASSERT(MCore::Array::GetLength(trianglesObj) / 3 >= triangleCount);
        auto ib = MCore::Array::GetAddress<IndexType>(trianglesObj);

        return mesh->UpdateTriangles(triangleCount, ib);
    }
#endif
}

bool Mesh::HasVertexColors() const
{
    return _vertexBuffers[2] != nullptr && _vertexBuffers[2]->IsAllocated();
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const void* ib, bool use16BitIndices)
{
    auto model = (Model*)_model;

    Unload();

    // Setup GPU resources
    model->LODs[_lodIndex]._verticesCount -= _vertices;
    const bool failed = Load(vertexCount, triangleCount, vb0, vb1, vb2, ib, use16BitIndices);
    if (!failed)
    {
        model->LODs[_lodIndex]._verticesCount += _vertices;

        // Calculate mesh bounds
        BoundingBox bounds;
        BoundingBox::FromPoints((const Float3*)vb0, vertexCount, bounds);
        SetBounds(bounds);

        // Send event (actors using this model can update bounds, etc.)
        model->onLoaded();
    }

    return failed;
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint16* triangles, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
{
    return ::UpdateMesh<uint16>(this, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint32* triangles, const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
{
    return ::UpdateMesh<uint32>(this, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
}

bool Mesh::UpdateTriangles(uint32 triangleCount, const void* ib, bool use16BitIndices)
{
    // Cache data
    uint32 indicesCount = triangleCount * 3;
    uint32 ibStride = use16BitIndices ? sizeof(uint16) : sizeof(uint32);

    // Create index buffer
    GPUBuffer* indexBuffer = GPUDevice::Instance->CreateBuffer(String::Empty);
    if (indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
    {
        Delete(indexBuffer);
        return true;
    }

    // TODO: update collision proxy

    // Initialize
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
    _indexBuffer = indexBuffer;
    _triangles = triangleCount;
    _use16BitIndexBuffer = use16BitIndices;

    return false;
}

void Mesh::Init(Model* model, int32 lodIndex, int32 index, int32 materialSlotIndex, const BoundingBox& box, const BoundingSphere& sphere, bool hasLightmapUVs)
{
    _model = model;
    _lodIndex = lodIndex;
    _index = index;
    _materialSlotIndex = materialSlotIndex;
    _use16BitIndexBuffer = false;
    _hasLightmapUVs = hasLightmapUVs;
    _box = box;
    _sphere = sphere;
    _vertices = 0;
    _triangles = 0;
    _vertexBuffers[0] = nullptr;
    _vertexBuffers[1] = nullptr;
    _vertexBuffers[2] = nullptr;
    _indexBuffer = nullptr;
}

Mesh::~Mesh()
{
    // Release buffers
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[0]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[1]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[2]);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
}

bool Mesh::Load(uint32 vertices, uint32 triangles, const void* vb0, const void* vb1, const void* vb2, const void* ib, bool use16BitIndexBuffer)
{
    // Cache data
    uint32 indicesCount = triangles * 3;
    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);

    GPUBuffer* vertexBuffer0 = nullptr;
    GPUBuffer* vertexBuffer1 = nullptr;
    GPUBuffer* vertexBuffer2 = nullptr;
    GPUBuffer* indexBuffer = nullptr;

    // Create GPU buffers
#if GPU_ENABLE_RESOURCE_NAMING
#define MESH_BUFFER_NAME(postfix) GetModel()->GetPath() + TEXT(postfix)
#else
#define MESH_BUFFER_NAME(postfix) String::Empty
#endif
    vertexBuffer0 = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".VB0"));
    if (vertexBuffer0->Init(GPUBufferDescription::Vertex(sizeof(VB0ElementType), vertices, vb0)))
        goto ERROR_LOAD_END;
    vertexBuffer1 = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".VB1"));
    if (vertexBuffer1->Init(GPUBufferDescription::Vertex(sizeof(VB1ElementType), vertices, vb1)))
        goto ERROR_LOAD_END;
    if (vb2)
    {
        vertexBuffer2 = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".VB2"));
        if (vertexBuffer2->Init(GPUBufferDescription::Vertex(sizeof(VB2ElementType), vertices, vb2)))
            goto ERROR_LOAD_END;
    }
    indexBuffer = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".IB"));
    if (indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
        goto ERROR_LOAD_END;

    // Init collision proxy
#if USE_PRECISE_MESH_INTERSECTS
    if (!_collisionProxy.HasData())
    {
        if (use16BitIndexBuffer)
            _collisionProxy.Init<uint16>(vertices, triangles, (Float3*)vb0, (uint16*)ib);
        else
            _collisionProxy.Init<uint32>(vertices, triangles, (Float3*)vb0, (uint32*)ib);
    }
#endif

    // Initialize
    _vertexBuffers[0] = vertexBuffer0;
    _vertexBuffers[1] = vertexBuffer1;
    _vertexBuffers[2] = vertexBuffer2;
    _indexBuffer = indexBuffer;
    _triangles = triangles;
    _vertices = vertices;
    _use16BitIndexBuffer = use16BitIndexBuffer;
    _cachedVertexBuffer[0].Clear();
    _cachedVertexBuffer[1].Clear();
    _cachedVertexBuffer[2].Clear();

    return false;

#undef MESH_BUFFER_NAME
ERROR_LOAD_END:

    SAFE_DELETE_GPU_RESOURCE(vertexBuffer0);
    SAFE_DELETE_GPU_RESOURCE(vertexBuffer1);
    SAFE_DELETE_GPU_RESOURCE(vertexBuffer2);
    SAFE_DELETE_GPU_RESOURCE(indexBuffer);
    return true;
}

void Mesh::Unload()
{
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[0]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[1]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[2]);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
    _triangles = 0;
    _vertices = 0;
    _use16BitIndexBuffer = false;
    _cachedIndexBuffer.Resize(0);
    _cachedVertexBuffer[0].Clear();
    _cachedVertexBuffer[1].Clear();
    _cachedVertexBuffer[2].Clear();
}

bool Mesh::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal) const
{
    // Get bounding box of the mesh bounds transformed by the instance world matrix
    Vector3 corners[8];
    _box.GetCorners(corners);
    Vector3 tmp;
    Vector3::Transform(corners[0], world, tmp);
    Vector3 min = tmp;
    Vector3 max = tmp;
    for (int32 i = 1; i < 8; i++)
    {
        Vector3::Transform(corners[i], world, tmp);
        min = Vector3::Min(min, tmp);
        max = Vector3::Max(max, tmp);
    }
    const BoundingBox transformedBox(min, max);

    // Test ray on box
#if USE_PRECISE_MESH_INTERSECTS
    if (transformedBox.Intersects(ray, distance))
    {
        // Use exact test on raw geometry
        return _collisionProxy.Intersects(ray, world, distance, normal);
    }
    distance = 0;
    normal = Vector3::Up;
    return false;
#else
	return transformedBox.Intersects(ray, distance, normal);
#endif
}

bool Mesh::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const
{
    // Get bounding box of the mesh bounds transformed by the instance world matrix
    Vector3 corners[8];
    _box.GetCorners(corners);
    Vector3 tmp;
    transform.LocalToWorld(corners[0], tmp);
    Vector3 min = tmp;
    Vector3 max = tmp;
    for (int32 i = 1; i < 8; i++)
    {
        transform.LocalToWorld(corners[i], tmp);
        min = Vector3::Min(min, tmp);
        max = Vector3::Max(max, tmp);
    }
    const BoundingBox transformedBox(min, max);

    // Test ray on box
#if USE_PRECISE_MESH_INTERSECTS
    if (transformedBox.Intersects(ray, distance))
    {
        // Use exact test on raw geometry
        return _collisionProxy.Intersects(ray, transform, distance, normal);
    }
    distance = 0;
    normal = Vector3::Up;
    return false;
#else
	return transformedBox.Intersects(ray, distance, normal);
#endif
}

void Mesh::GetDrawCallGeometry(DrawCall& drawCall) const
{
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    drawCall.Geometry.VertexBuffersOffsets[0] = 0;
    drawCall.Geometry.VertexBuffersOffsets[1] = 0;
    drawCall.Geometry.VertexBuffersOffsets[2] = 0;
    drawCall.Draw.StartIndex = 0;
    drawCall.Draw.IndicesCount = _triangles * 3;
}

void Mesh::Render(GPUContext* context) const
{
    if (!IsInitialized())
        return;

    context->BindVB(ToSpan((GPUBuffer**)_vertexBuffers, 3));
    context->BindIB(_indexBuffer);
    context->DrawIndexed(_triangles * 3);
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
        drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(VB2ElementType);
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
        drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(VB2ElementType);
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
#endif

    // Push draw call to the render lists
    const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
    const auto drawModes = info.DrawModes & material->GetDrawModes();
    if (drawModes != DrawPass::None)
        renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, drawModes, info.Flags, shadowsMode, info.Bounds, drawCall, entry.ReceiveDecals, info.SortOrder);
}

bool Mesh::DownloadDataGPU(MeshBufferType type, BytesContainer& result) const
{
    GPUBuffer* buffer = nullptr;
    switch (type)
    {
    case MeshBufferType::Index:
        buffer = _indexBuffer;
        break;
    case MeshBufferType::Vertex0:
        buffer = _vertexBuffers[0];
        break;
    case MeshBufferType::Vertex1:
        buffer = _vertexBuffers[1];
        break;
    case MeshBufferType::Vertex2:
        buffer = _vertexBuffers[2];
        break;
    }
    return buffer && buffer->DownloadData(result);
}

Task* Mesh::DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const
{
    GPUBuffer* buffer = nullptr;
    switch (type)
    {
    case MeshBufferType::Index:
        buffer = _indexBuffer;
        break;
    case MeshBufferType::Vertex0:
        buffer = _vertexBuffers[0];
        break;
    case MeshBufferType::Vertex1:
        buffer = _vertexBuffers[1];
        break;
    case MeshBufferType::Vertex2:
        buffer = _vertexBuffers[2];
        break;
    }
    return buffer ? buffer->DownloadDataAsync(result) : nullptr;
}

bool Mesh::DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const
{
    if (_cachedVertexBuffer[0].IsEmpty())
    {
        PROFILE_CPU();
        auto model = GetModel();
        ScopeLock lock(model->Locker);
        if (model->IsVirtual())
        {
            LOG(Error, "Cannot access CPU data of virtual models. Use GPU data download.");
            return true;
        }

        // Fetch chunk with data from drive/memory
        const auto chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(GetLODIndex());
        if (model->LoadChunk(chunkIndex))
            return true;
        const auto chunk = model->GetChunk(chunkIndex);
        if (!chunk)
        {
            LOG(Error, "Missing chunk.");
            return true;
        }

        MemoryReadStream stream(chunk->Get(), chunk->Size());

        // Seek to find mesh location
        for (int32 i = 0; i <= _index; i++)
        {
            // #MODEL_DATA_FORMAT_USAGE
            uint32 vertices;
            stream.ReadUint32(&vertices);
            uint32 triangles;
            stream.ReadUint32(&triangles);
            uint32 indicesCount = triangles * 3;
            bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
            uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
            if (vertices == 0 || triangles == 0)
            {
                LOG(Error, "Invalid mesh data.");
                return true;
            }
            auto vb0 = stream.Move<VB0ElementType>(vertices);
            auto vb1 = stream.Move<VB1ElementType>(vertices);
            bool hasColors = stream.ReadBool();
            VB2ElementType18* vb2 = nullptr;
            if (hasColors)
            {
                vb2 = stream.Move<VB2ElementType18>(vertices);
            }
            auto ib = stream.Move<byte>(indicesCount * ibStride);

            if (i != _index)
                continue;

            // Cache mesh data
            _cachedIndexBufferCount = indicesCount;
            _cachedIndexBuffer.Set(ib, indicesCount * ibStride);
            _cachedVertexBuffer[0].Set((const byte*)vb0, vertices * sizeof(VB0ElementType));
            _cachedVertexBuffer[1].Set((const byte*)vb1, vertices * sizeof(VB1ElementType));
            if (hasColors)
                _cachedVertexBuffer[2].Set((const byte*)vb2, vertices * sizeof(VB2ElementType));
            break;
        }
    }

    switch (type)
    {
    case MeshBufferType::Index:
        result.Link(_cachedIndexBuffer);
        count = _cachedIndexBufferCount;
        break;
    case MeshBufferType::Vertex0:
        result.Link(_cachedVertexBuffer[0]);
        count = _cachedVertexBuffer[0].Count() / sizeof(VB0ElementType);
        break;
    case MeshBufferType::Vertex1:
        result.Link(_cachedVertexBuffer[1]);
        count = _cachedVertexBuffer[1].Count() / sizeof(VB1ElementType);
        break;
    case MeshBufferType::Vertex2:
        result.Link(_cachedVertexBuffer[2]);
        count = _cachedVertexBuffer[2].Count() / sizeof(VB2ElementType);
        break;
    default:
        return true;
    }
    return false;
}

ScriptingObject* Mesh::GetParentModel()
{
    return _model;
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

bool Mesh::UpdateTrianglesUInt(int32 triangleCount, const MArray* trianglesObj)
{
    return ::UpdateTriangles<uint32>(this, triangleCount, trianglesObj);
}

bool Mesh::UpdateTrianglesUShort(int32 triangleCount, const MArray* trianglesObj)
{
    return ::UpdateTriangles<uint16>(this, triangleCount, trianglesObj);
}

enum class InternalBufferType
{
    VB0 = 0,
    VB1 = 1,
    VB2 = 2,
    IB16 = 3,
    IB32 = 4,
};

MArray* Mesh::DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI)
{
    auto mesh = this;
    auto type = (InternalBufferType)typeI;
    auto model = mesh->GetModel();
    ScopeLock lock(model->Locker);

    // Virtual assets always fetch from GPU memory
    forceGpu |= model->IsVirtual();

    if (!mesh->IsInitialized() && forceGpu)
    {
        LOG(Error, "Cannot load mesh data from GPU if it's not loaded.");
        return nullptr;
    }

    MeshBufferType bufferType;
    switch (type)
    {
    case InternalBufferType::VB0:
        bufferType = MeshBufferType::Vertex0;
        break;
    case InternalBufferType::VB1:
        bufferType = MeshBufferType::Vertex1;
        break;
    case InternalBufferType::VB2:
        bufferType = MeshBufferType::Vertex2;
        break;
    case InternalBufferType::IB16:
    case InternalBufferType::IB32:
        bufferType = MeshBufferType::Index;
        break;
    default:
        return nullptr;
    }
    BytesContainer data;
    int32 dataCount;
    if (forceGpu)
    {
        // Get data from GPU
        // TODO: support reusing the input memory buffer to perform a single copy from staging buffer to the input CPU buffer
        auto task = mesh->DownloadDataGPUAsync(bufferType, data);
        if (task == nullptr)
            return nullptr;
        task->Start();
        model->Locker.Unlock();
        if (task->Wait())
        {
            LOG(Error, "Task failed.");
            return nullptr;
        }
        model->Locker.Lock();

        // Extract elements count from result data
        switch (bufferType)
        {
        case MeshBufferType::Index:
            dataCount = data.Length() / (Use16BitIndexBuffer() ? sizeof(uint16) : sizeof(uint32));
            break;
        case MeshBufferType::Vertex0:
            dataCount = data.Length() / sizeof(VB0ElementType);
            break;
        case MeshBufferType::Vertex1:
            dataCount = data.Length() / sizeof(VB1ElementType);
            break;
        case MeshBufferType::Vertex2:
            dataCount = data.Length() / sizeof(VB2ElementType);
            break;
        }
    }
    else
    {
        // Get data from CPU
        if (DownloadDataCPU(bufferType, data, dataCount))
            return nullptr;
    }

    // Convert into managed array
    MArray* result = MCore::Array::New(MCore::Type::GetClass(INTERNAL_TYPE_OBJECT_GET(resultType)), dataCount);
    void* managedArrayPtr = MCore::Array::GetAddress(result);
    const int32 elementSize = data.Length() / dataCount;
    switch (type)
    {
    case InternalBufferType::VB0:
    case InternalBufferType::VB1:
    case InternalBufferType::VB2:
    {
        Platform::MemoryCopy(managedArrayPtr, data.Get(), data.Length());
        break;
    }
    case InternalBufferType::IB16:
    {
        if (elementSize == sizeof(uint16))
        {
            Platform::MemoryCopy(managedArrayPtr, data.Get(), data.Length());
        }
        else
        {
            auto dst = (uint16*)managedArrayPtr;
            auto src = (uint32*)data.Get();
            for (int32 i = 0; i < dataCount; i++)
                dst[i] = src[i];
        }
        break;
    }
    case InternalBufferType::IB32:
    {
        if (elementSize == sizeof(uint16))
        {
            auto dst = (uint32*)managedArrayPtr;
            auto src = (uint16*)data.Get();
            for (int32 i = 0; i < dataCount; i++)
                dst[i] = src[i];
        }
        else
        {
            Platform::MemoryCopy(managedArrayPtr, data.Get(), data.Length());
        }
        break;
    }
    }

    return result;
}

#endif
