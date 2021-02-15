// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Mesh.h"
#include "ModelInstanceEntry.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

namespace
{
    template<typename IndexType>
    bool UpdateMesh(Mesh* mesh, uint32 vertexCount, uint32 triangleCount, Vector3* vertices, IndexType* triangles, Vector3* normals, Vector3* tangents, Vector2* uvs, Color32* colors)
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
                    const Vector3 normal = normals[i];
                    const Vector3 tangent = tangents[i];

                    // Calculate bitangent sign
                    Vector3 bitangent = Vector3::Normalize(Vector3::Cross(normal, tangent));
                    byte sign = static_cast<byte>(Vector3::Dot(Vector3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

                    // Set tangent frame
                    vb1[i].Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
                    vb1[i].Normal = Float1010102(normal * 0.5f + 0.5f, 0);
                }
            }
            else
            {
                for (uint32 i = 0; i < vertexCount; i++)
                {
                    const Vector3 normal = normals[i];

                    // Calculate tangent
                    Vector3 c1 = Vector3::Cross(normal, Vector3::UnitZ);
                    Vector3 c2 = Vector3::Cross(normal, Vector3::UnitY);
                    Vector3 tangent;
                    if (c1.LengthSquared() > c2.LengthSquared())
                        tangent = c1;
                    else
                        tangent = c2;

                    // Calculate bitangent sign
                    Vector3 bitangent = Vector3::Normalize(Vector3::Cross(normal, tangent));
                    byte sign = static_cast<byte>(Vector3::Dot(Vector3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

                    // Set tangent frame
                    vb1[i].Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
                    vb1[i].Normal = Float1010102(normal * 0.5f + 0.5f, 0);
                }
            }
        }
        else
        {
            // Set default tangent frame
            const auto n = Float1010102(Vector3::UnitZ);
            const auto t = Float1010102(Vector3::UnitX);
            for (uint32 i = 0; i < vertexCount; i++)
            {
                vb1[i].Normal = n;
                vb1[i].Tangent = t;
            }
        }
        if (uvs)
        {
            for (uint32 i = 0; i < vertexCount; i++)
                vb1[i].TexCoord = Half2(uvs[i]);
        }
        else
        {
            auto v = Half2(0, 0);
            for (uint32 i = 0; i < vertexCount; i++)
                vb1[i].TexCoord = v;
        }
        {
            auto v = Half2(0, 0);
            for (uint32 i = 0; i < vertexCount; i++)
                vb1[i].LightmapUVs = v;
        }
        if (colors)
        {
            vb2.Resize(vertexCount);
            for (uint32 i = 0; i < vertexCount; i++)
                vb2[i].Color = colors[i];
        }

        return mesh->UpdateMesh(vertexCount, triangleCount, (VB0ElementType*)vertices, vb1.Get(), vb2.HasItems() ? vb2.Get() : nullptr, triangles);
    }

    template<typename IndexType>
    bool UpdateMesh(Mesh* mesh, uint32 vertexCount, uint32 triangleCount, MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj, MonoArray* colorsObj)
    {
        ASSERT((uint32)mono_array_length(verticesObj) >= vertexCount);
        ASSERT((uint32)mono_array_length(trianglesObj) / 3 >= triangleCount);
        auto vertices = (Vector3*)(void*)mono_array_addr_with_size(verticesObj, sizeof(Vector3), 0);
        auto triangles = (IndexType*)(void*)mono_array_addr_with_size(trianglesObj, sizeof(IndexType), 0);
        const auto normals = normalsObj ? (Vector3*)(void*)mono_array_addr_with_size(normalsObj, sizeof(Vector3), 0) : nullptr;
        const auto tangents = tangentsObj ? (Vector3*)(void*)mono_array_addr_with_size(tangentsObj, sizeof(Vector3), 0) : nullptr;
        const auto uvs = uvObj ? (Vector2*)(void*)mono_array_addr_with_size(uvObj, sizeof(Vector2), 0) : nullptr;
        const auto colors = colorsObj ? (Color32*)(void*)mono_array_addr_with_size(colorsObj, sizeof(Color32), 0) : nullptr;
        return UpdateMesh<IndexType>(mesh, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
    }

    template<typename IndexType>
    bool UpdateTriangles(Mesh* mesh, int32 triangleCount, MonoArray* trianglesObj)
    {
        const auto model = mesh->GetModel();
        ASSERT(model && model->IsVirtual() && trianglesObj);

        // Get buffer data
        ASSERT((int32)mono_array_length(trianglesObj) / 3 >= triangleCount);
        auto ib = (IndexType*)(void*)mono_array_addr_with_size(trianglesObj, sizeof(IndexType), 0);

        return mesh->UpdateTriangles(triangleCount, ib);
    }
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0ElementType* vb0, VB1ElementType* vb1, VB2ElementType* vb2, void* ib, bool use16BitIndices)
{
    Unload();

    // Setup GPU resources
    _model->LODs[_lodIndex]._verticesCount -= _vertices;
    const bool failed = Load(vertexCount, triangleCount, vb0, vb1, vb2, ib, use16BitIndices);
    if (!failed)
    {
        _model->LODs[_lodIndex]._verticesCount += _vertices;

        // Calculate mesh bounds
        SetBounds(BoundingBox::FromPoints((Vector3*)vb0, vertexCount));

        // Send event (actors using this model can update bounds, etc.)
        _model->onLoaded();
    }

    return failed;
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, Vector3* vertices, uint16* triangles, Vector3* normals, Vector3* tangents, Vector2* uvs, Color32* colors)
{
    return ::UpdateMesh<uint16>(this, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
}

bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, Vector3* vertices, uint32* triangles, Vector3* normals, Vector3* tangents, Vector2* uvs, Color32* colors)
{
    return ::UpdateMesh<uint32>(this, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
}

bool Mesh::UpdateTriangles(uint32 triangleCount, void* ib, bool use16BitIndices)
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

void Mesh::SetMaterialSlotIndex(int32 value)
{
    if (value < 0 || value >= _model->MaterialSlots.Count())
    {
        LOG(Warning, "Cannot set mesh material slot to {0} while model has {1} slots.", value, _model->MaterialSlots.Count());
        return;
    }

    _materialSlotIndex = value;
}

bool Mesh::Load(uint32 vertices, uint32 triangles, void* vb0, void* vb1, void* vb2, void* ib, bool use16BitIndexBuffer)
{
    // Cache data
    uint32 indicesCount = triangles * 3;
    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);

    GPUBuffer* vertexBuffer0 = nullptr;
    GPUBuffer* vertexBuffer1 = nullptr;
    GPUBuffer* vertexBuffer2 = nullptr;
    GPUBuffer* indexBuffer = nullptr;

    // Create vertex buffer 0
#if GPU_ENABLE_RESOURCE_NAMING
    vertexBuffer0 = GPUDevice::Instance->CreateBuffer(GetModel()->ToString() + TEXT(".VB0"));
#else
	vertexBuffer0 = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
    if (vertexBuffer0->Init(GPUBufferDescription::Vertex(sizeof(VB0ElementType), vertices, vb0)))
        goto ERROR_LOAD_END;

    // Create vertex buffer 1
#if GPU_ENABLE_RESOURCE_NAMING
    vertexBuffer1 = GPUDevice::Instance->CreateBuffer(GetModel()->ToString() + TEXT(".VB1"));
#else
	vertexBuffer1 = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
    if (vertexBuffer1->Init(GPUBufferDescription::Vertex(sizeof(VB1ElementType), vertices, vb1)))
        goto ERROR_LOAD_END;

    // Create vertex buffer 2
    if (vb2)
    {
#if GPU_ENABLE_RESOURCE_NAMING
        vertexBuffer2 = GPUDevice::Instance->CreateBuffer(GetModel()->ToString() + TEXT(".VB2"));
#else
		vertexBuffer2 = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
        if (vertexBuffer2->Init(GPUBufferDescription::Vertex(sizeof(VB2ElementType), vertices, vb2)))
            goto ERROR_LOAD_END;
    }

    // Create index buffer
#if GPU_ENABLE_RESOURCE_NAMING
    indexBuffer = GPUDevice::Instance->CreateBuffer(GetModel()->ToString() + TEXT(".IB"));
#else
	indexBuffer = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
    if (indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
        goto ERROR_LOAD_END;

    // Init collision proxy
#if USE_PRECISE_MESH_INTERSECTS
    if (!_collisionProxy.HasData())
    {
        if (use16BitIndexBuffer)
            _collisionProxy.Init<uint16>(vertices, triangles, (Vector3*)vb0, (uint16*)ib);
        else
            _collisionProxy.Init<uint32>(vertices, triangles, (Vector3*)vb0, (uint32*)ib);
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

    return false;

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
}

bool Mesh::Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal) const
{
    // Get bounding box of the mesh bounds transformed by the instance world matrix
    Vector3 corners[8];
    GetCorners(corners);
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

void Mesh::GetDrawCallGeometry(DrawCall& drawCall)
{
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    drawCall.Geometry.VertexBuffersOffsets[0] = 0;
    drawCall.Geometry.VertexBuffersOffsets[1] = 0;
    drawCall.Geometry.VertexBuffersOffsets[2] = 0;
    drawCall.Geometry.StartIndex = 0;
    drawCall.Geometry.IndicesCount = _triangles * 3;
}

void Mesh::Render(GPUContext* context) const
{
    ASSERT(IsInitialized());

    context->BindVB(ToSpan((GPUBuffer**)_vertexBuffers, 3));
    context->BindIB(_indexBuffer);
    context->DrawIndexedInstanced(_triangles * 3, 1, 0, 0, 0);
}

void Mesh::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, StaticFlags flags, bool receiveDecals) const
{
    if (!material || !material->IsSurface())
        return;

    // Submit draw call
    DrawCall drawCall;
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    drawCall.Geometry.VertexBuffersOffsets[0] = 0;
    drawCall.Geometry.VertexBuffersOffsets[1] = 0;
    drawCall.Geometry.VertexBuffersOffsets[2] = 0;
    drawCall.Geometry.StartIndex = 0;
    drawCall.Geometry.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.IndirectArgsBuffer = nullptr;
    drawCall.IndirectArgsOffset = 0;
    drawCall.Material = material;
    drawCall.World = world;
    drawCall.PrevWorld = world;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.GeometrySize = _box.GetSize();
    drawCall.Lightmap = nullptr;
    drawCall.LightmapUVsArea = Rectangle::Empty;
    drawCall.Skinning = nullptr;
    drawCall.WorldDeterminantSign = Math::FloatSelect(world.RotDeterminant(), 1, -1);
    drawCall.PerInstanceRandom = 0.0f;
    drawCall.LODDitherFactor = 0.0f;
    renderContext.List->AddDrawCall(DrawPass::Default, flags, drawCall, receiveDecals);
}

void Mesh::Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const
{
    // Cache data
    const auto& entry = info.Buffer->At(_materialSlotIndex);
    if (!entry.Visible || !IsInitialized())
        return;
    const MaterialSlot& slot = _model->MaterialSlots[_materialSlotIndex];

    // Check if skip rendering
    const auto shadowsMode = static_cast<ShadowsCastingMode>(entry.ShadowsMode & slot.ShadowsMode);
    const auto drawModes = static_cast<DrawPass>(info.DrawModes & renderContext.View.GetShadowsDrawPassMask(shadowsMode));
    if (drawModes == DrawPass::None)
        return;

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

    // Submit draw call
    DrawCall drawCall;
    drawCall.Geometry.IndexBuffer = _indexBuffer;
    drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
    drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
    drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
    drawCall.Geometry.VertexBuffersOffsets[0] = 0;
    drawCall.Geometry.VertexBuffersOffsets[1] = 0;
    drawCall.Geometry.VertexBuffersOffsets[2] = 0;
    if (info.VertexColors && info.VertexColors[_lodIndex])
    {
        drawCall.Geometry.VertexBuffers[2] = info.VertexColors[_lodIndex];
        // TODO: cache vertexOffset within the model LOD per-mesh
        uint32 vertexOffset = 0;
        for (int32 meshIndex = 0; meshIndex < _index; meshIndex++)
            vertexOffset += _model->LODs[_lodIndex].Meshes[meshIndex].GetVertexCount();
        drawCall.Geometry.VertexBuffers[2] = info.VertexColors[_lodIndex];
        drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(VB2ElementType);
    }
    drawCall.Geometry.StartIndex = 0;
    drawCall.Geometry.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.IndirectArgsBuffer = nullptr;
    drawCall.IndirectArgsOffset = 0;
    drawCall.Material = material;
    drawCall.World = *info.World;
    drawCall.PrevWorld = info.DrawState->PrevWorld;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.GeometrySize = _box.GetSize();
    drawCall.Lightmap = info.Flags & StaticFlags::Lightmap ? info.Lightmap : nullptr;
    drawCall.LightmapUVsArea = info.LightmapUVs ? *info.LightmapUVs : Rectangle::Empty;
    drawCall.Skinning = nullptr;
    drawCall.WorldDeterminantSign = Math::FloatSelect(drawCall.World.RotDeterminant(), 1, -1);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;
    drawCall.LODDitherFactor = lodDitherFactor;
    renderContext.List->AddDrawCall(drawModes, info.Flags, drawCall, entry.ReceiveDecals);
}

bool Mesh::ExtractData(MeshBufferType type, BytesContainer& result) const
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

Task* Mesh::ExtractDataAsync(MeshBufferType type, BytesContainer& result) const
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

ScriptingObject* Mesh::GetParentModel()
{
    return _model;
}

bool Mesh::UpdateMeshInt(int32 vertexCount, int32 triangleCount, MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj, MonoArray* colorsObj)
{
    return ::UpdateMesh<uint32>(this, (uint32)vertexCount, (uint32)triangleCount, verticesObj, trianglesObj, normalsObj, tangentsObj, uvObj, colorsObj);
}

bool Mesh::UpdateMeshUShort(int32 vertexCount, int32 triangleCount, MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj, MonoArray* colorsObj)
{
    return ::UpdateMesh<uint16>(this, (uint32)vertexCount, (uint32)triangleCount, verticesObj, trianglesObj, normalsObj, tangentsObj, uvObj, colorsObj);
}

bool Mesh::UpdateTrianglesInt(int32 triangleCount, MonoArray* trianglesObj)
{
    return ::UpdateTriangles<uint32>(this, triangleCount, trianglesObj);
}

bool Mesh::UpdateTrianglesUShort(int32 triangleCount, MonoArray* trianglesObj)
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

void ConvertMeshData(Mesh* mesh, InternalBufferType type, MonoArray* resultObj, void* srcData)
{
    auto vertices = mesh->GetVertexCount();
    auto triangles = mesh->GetTriangleCount();
    auto indices = triangles * 3;
    auto use16BitIndexBuffer = mesh->Use16BitIndexBuffer();

    void* managedArrayPtr = mono_array_addr_with_size(resultObj, 0, 0);
    switch (type)
    {
    case InternalBufferType::VB0:
    {
        Platform::MemoryCopy(managedArrayPtr, srcData, sizeof(VB0ElementType) * vertices);
        break;
    }
    case InternalBufferType::VB1:
    {
        Platform::MemoryCopy(managedArrayPtr, srcData, sizeof(VB1ElementType) * vertices);
        break;
    }
    case InternalBufferType::VB2:
    {
        Platform::MemoryCopy(managedArrayPtr, srcData, sizeof(VB2ElementType) * vertices);
        break;
    }
    case InternalBufferType::IB16:
    {
        if (use16BitIndexBuffer)
        {
            Platform::MemoryCopy(managedArrayPtr, srcData, sizeof(uint16) * indices);
        }
        else
        {
            auto dst = (uint16*)managedArrayPtr;
            auto src = (uint32*)srcData;
            for (int32 i = 0; i < indices; i++)
            {
                dst[i] = src[i];
            }
        }
        break;
    }
    case InternalBufferType::IB32:
    {
        if (use16BitIndexBuffer)
        {
            auto dst = (uint32*)managedArrayPtr;
            auto src = (uint16*)srcData;
            for (int32 i = 0; i < indices; i++)
            {
                dst[i] = src[i];
            }
        }
        else
        {
            Platform::MemoryCopy(managedArrayPtr, srcData, sizeof(uint32) * indices);
        }
        break;
    }
    }
}

bool Mesh::DownloadBuffer(bool forceGpu, MonoArray* resultObj, int32 typeI)
{
    Mesh* mesh = this;
    InternalBufferType type = (InternalBufferType)typeI;
    auto model = mesh->GetModel();
    ASSERT(model && resultObj);

    ScopeLock lock(model->Locker);

    // Virtual assets always fetch from GPU memory
    forceGpu |= model->IsVirtual();

    if (!mesh->IsInitialized() && forceGpu)
    {
        LOG(Error, "Cannot load mesh data from GPU if it's not loaded.");
        return true;
    }
    if (type == InternalBufferType::IB16 || type == InternalBufferType::IB32)
    {
        if (mono_array_length(resultObj) != mesh->GetTriangleCount() * 3)
        {
            LOG(Error, "Invalid buffer size.");
            return true;
        }
    }
    else
    {
        if (mono_array_length(resultObj) != mesh->GetVertexCount())
        {
            LOG(Error, "Invalid buffer size.");
            return true;
        }
    }

    // Check if load data from GPU
    if (forceGpu)
    {
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
        default: CRASH;
            return true;
        }

        // TODO: support reusing the input memory buffer to perform a single copy from staging buffer to the input CPU buffer
        BytesContainer data;
        auto task = mesh->ExtractDataAsync(bufferType, data);
        if (task == nullptr)
            return true;

        model->Locker.Unlock();

        task->Start();
        if (task->Wait())
        {
            LOG(Error, "Task failed.");
            return true;
        }

        ConvertMeshData(mesh, type, resultObj, data.Get());

        model->Locker.Lock();

        return false;
    }

    // Get data from drive/memory
    {
        // Fetch chunk with data
        const auto chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(mesh->GetLODIndex());
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
        for (int32 i = 0; i < mesh->GetIndex(); i++)
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
            auto vb0 = stream.Read<VB0ElementType>(vertices);
            auto vb1 = stream.Read<VB1ElementType>(vertices);
            bool hasColors = stream.ReadBool();
            VB2ElementType18* vb2 = nullptr;
            if (hasColors)
            {
                vb2 = stream.Read<VB2ElementType18>(vertices);
            }
            auto ib = stream.Read<byte>(indicesCount * ibStride);
        }

        // Get mesh data
        void* data = nullptr;
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
            auto vb0 = stream.Read<VB0ElementType>(vertices);
            auto vb1 = stream.Read<VB1ElementType>(vertices);
            bool hasColors = stream.ReadBool();
            VB2ElementType18* vb2 = nullptr;
            if (hasColors)
            {
                vb2 = stream.Read<VB2ElementType18>(vertices);
            }
            auto ib = stream.Read<byte>(indicesCount * ibStride);

            if (mesh->HasVertexColors() != hasColors || mesh->Use16BitIndexBuffer() != use16BitIndexBuffer)
            {
                LOG(Error, "Invalid mesh data loaded from chunk.");
                return true;
            }

            switch (type)
            {
            case InternalBufferType::VB0:
                data = vb0;
                break;
            case InternalBufferType::VB1:
                data = vb1;
                break;
            case InternalBufferType::VB2:
                data = vb2;
                break;
            case InternalBufferType::IB16:
            case InternalBufferType::IB32:
                data = ib;
                break;
            }
        }

        ConvertMeshData(mesh, type, resultObj, data);
    }

    return false;
}
