// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SkinnedMesh.h"
#include "ModelInstanceEntry.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

void SkinnedMesh::Init(SkinnedModel* model, int32 lodIndex, int32 index, int32 materialSlotIndex, const BoundingBox& box, const BoundingSphere& sphere)
{
    _model = model;
    _index = index;
    _lodIndex = lodIndex;
    _materialSlotIndex = materialSlotIndex;
    _use16BitIndexBuffer = false;
    _box = box;
    _sphere = sphere;
    _vertices = 0;
    _triangles = 0;
    _vertexBuffer = nullptr;
    _indexBuffer = nullptr;
    _cachedIndexBuffer.Clear();
    _cachedVertexBuffer.Clear();
    BlendShapes.Clear();
}

SkinnedMesh::~SkinnedMesh()
{
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffer);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
}

void SkinnedMesh::SetMaterialSlotIndex(int32 value)
{
    if (value < 0 || value >= _model->MaterialSlots.Count())
    {
        LOG(Warning, "Cannot set mesh material slot to {0} while model has {1} slots.", value, _model->MaterialSlots.Count());
        return;
    }

    _materialSlotIndex = value;
}

bool SkinnedMesh::Load(uint32 vertices, uint32 triangles, void* vb0, void* ib, bool use16BitIndexBuffer)
{
    // Cache data
    uint32 indicesCount = triangles * 3;
    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);

    GPUBuffer* vertexBuffer = nullptr;
    GPUBuffer* indexBuffer = nullptr;

    // Create vertex buffer
#if GPU_ENABLE_RESOURCE_NAMING
    vertexBuffer = GPUDevice::Instance->CreateBuffer(GetSkinnedModel()->ToString() + TEXT(".VB"));
#else
	vertexBuffer = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
    if (vertexBuffer->Init(GPUBufferDescription::Vertex(sizeof(VB0SkinnedElementType), vertices, vb0)))
        goto ERROR_LOAD_END;

    // Create index buffer
#if GPU_ENABLE_RESOURCE_NAMING
    indexBuffer = GPUDevice::Instance->CreateBuffer(GetSkinnedModel()->ToString() + TEXT(".IB"));
#else
	indexBuffer = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
    if (indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
        goto ERROR_LOAD_END;

    // Initialize
    _vertexBuffer = vertexBuffer;
    _indexBuffer = indexBuffer;
    _triangles = triangles;
    _vertices = vertices;
    _use16BitIndexBuffer = use16BitIndexBuffer;

    return false;

ERROR_LOAD_END:

    SAFE_DELETE_GPU_RESOURCE(vertexBuffer);
    SAFE_DELETE_GPU_RESOURCE(indexBuffer);
    return true;
}

void SkinnedMesh::Unload()
{
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffer);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
    _cachedIndexBuffer.Clear();
    _cachedVertexBuffer.Clear();
    _triangles = 0;
    _vertices = 0;
    _use16BitIndexBuffer = false;
}

bool SkinnedMesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, VB0SkinnedElementType* vb, void* ib, bool use16BitIndices)
{
    // Setup GPU resources
    const bool failed = Load(vertexCount, triangleCount, vb, ib, use16BitIndices);
    if (!failed)
    {
        // Calculate mesh bounds
        SetBounds(BoundingBox::FromPoints((Vector3*)vb, vertexCount));

        // Send event (actors using this model can update bounds, etc.)
        _model->onLoaded();
    }

    return failed;
}

bool SkinnedMesh::Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal) const
{
    // Transform points
    Vector3 min, max;
    Vector3::Transform(_box.Minimum, world, min);
    Vector3::Transform(_box.Maximum, world, max);

    // Get transformed box
    BoundingBox transformedBox(min, max);

    // Test ray on a box
    return transformedBox.Intersects(ray, distance, normal);
}

void SkinnedMesh::Render(GPUContext* context) const
{
    ASSERT(IsInitialized());

    context->BindVB(ToSpan(&_vertexBuffer, 1));
    context->BindIB(_indexBuffer);
    context->DrawIndexed(_triangles * 3);
}

void SkinnedMesh::Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const
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
    BlendShapesInstance::MeshInstance* blendShapeMeshInstance;
    if (info.BlendShapes && info.BlendShapes->Meshes.TryGet(this, blendShapeMeshInstance) && blendShapeMeshInstance->IsUsed)
    {
        // Use modified vertex buffer from the blend shapes
        if (blendShapeMeshInstance->IsDirty)
        {
            blendShapeMeshInstance->VertexBuffer.Flush();
            blendShapeMeshInstance->IsDirty = false;
        }
        drawCall.Geometry.VertexBuffers[0] = blendShapeMeshInstance->VertexBuffer.GetBuffer();
    }
    else
    {
        drawCall.Geometry.VertexBuffers[0] = _vertexBuffer;
    }
    drawCall.Geometry.VertexBuffers[1] = nullptr;
    drawCall.Geometry.VertexBuffers[2] = nullptr;
    drawCall.Geometry.VertexBuffersOffsets[0] = 0;
    drawCall.Geometry.VertexBuffersOffsets[1] = 0;
    drawCall.Geometry.VertexBuffersOffsets[2] = 0;
    drawCall.Draw.StartIndex = 0;
    drawCall.Draw.IndicesCount = _triangles * 3;
    drawCall.InstanceCount = 1;
    drawCall.Material = material;
    drawCall.World = *info.World;
    drawCall.ObjectPosition = drawCall.World.GetTranslation();
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Lightmap = nullptr;
    drawCall.Surface.LightmapUVsArea = Rectangle::Empty;
    drawCall.Surface.Skinning = info.Skinning;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = Math::FloatSelect(drawCall.World.RotDeterminant(), 1, -1);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;
    renderContext.List->AddDrawCall(drawModes, StaticFlags::None, drawCall, entry.ReceiveDecals);
}

bool SkinnedMesh::DownloadDataGPU(MeshBufferType type, BytesContainer& result) const
{
    GPUBuffer* buffer = nullptr;
    switch (type)
    {
    case MeshBufferType::Index:
        buffer = _indexBuffer;
        break;
    case MeshBufferType::Vertex0:
        buffer = _vertexBuffer;
        break;
    }
    return buffer && buffer->DownloadData(result);
}

Task* SkinnedMesh::DownloadDataAsyncGPU(MeshBufferType type, BytesContainer& result) const
{
    GPUBuffer* buffer = nullptr;
    switch (type)
    {
    case MeshBufferType::Index:
        buffer = _indexBuffer;
        break;
    case MeshBufferType::Vertex0:
        buffer = _vertexBuffer;
        break;
    }
    return buffer ? buffer->DownloadDataAsync(result) : nullptr;
}

bool SkinnedMesh::DownloadDataCPU(MeshBufferType type, BytesContainer& result) const
{
    if (_cachedVertexBuffer.IsEmpty())
    {
        PROFILE_CPU();
        auto model = GetSkinnedModel();
        ScopeLock lock(model->Locker);

        // Fetch chunk with data from drive/memory
        const auto chunkIndex = _lodIndex + 1;
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
            uint16 blendShapesCount;
            stream.ReadUint16(&blendShapesCount);
            for (int32 blendShapeIndex = 0; blendShapeIndex < blendShapesCount; blendShapeIndex++)
            {
                uint32 minVertexIndex, maxVertexIndex;
                bool useNormals = stream.ReadBool();
                stream.ReadUint32(&minVertexIndex);
                stream.ReadUint32(&maxVertexIndex);
                uint32 blendShapeVertices;
                stream.ReadUint32(&blendShapeVertices);
                auto blendShapeVerticesData = stream.Read<byte>(blendShapeVertices * sizeof(BlendShapeVertex));
            }
            uint32 indicesCount = triangles * 3;
            bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
            uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
            if (vertices == 0 || triangles == 0)
            {
                LOG(Error, "Invalid mesh data.");
                return true;
            }
            auto vb0 = stream.Read<VB0SkinnedElementType>(vertices);
            auto ib = stream.Read<byte>(indicesCount * ibStride);

            if (i != _index)
                continue;

            // Cache mesh data
            _cachedIndexBuffer.Set(ib, indicesCount * ibStride);
            _cachedVertexBuffer.Set((const byte*)vb0, vertices * sizeof(VB0SkinnedElementType));
            break;
        }
    }

    switch (type)
    {
    case MeshBufferType::Index:
        result.Link(_cachedIndexBuffer);
        break;
    case MeshBufferType::Vertex0:
        result.Link(_cachedVertexBuffer);
        break;
    default:
        return true;
    }
    return false;
}

ScriptingObject* SkinnedMesh::GetParentModel()
{
    return _model;
}

template<typename IndexType>
bool UpdateMesh(SkinnedMesh* mesh, MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* blendIndicesObj, MonoArray* blendWeightsObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj)
{
    auto model = mesh->GetSkinnedModel();
    ASSERT(model && model->IsVirtual() && verticesObj && trianglesObj && blendIndicesObj && blendWeightsObj);

    // Get buffers data
    const auto vertexCount = (uint32)mono_array_length(verticesObj);
    const auto triangleCount = (uint32)mono_array_length(trianglesObj) / 3;
    auto vertices = (Vector3*)(void*)mono_array_addr_with_size(verticesObj, sizeof(Vector3), 0);
    auto ib = (IndexType*)(void*)mono_array_addr_with_size(trianglesObj, sizeof(int32), 0);
    auto blendIndices = (Int4*)(void*)mono_array_addr_with_size(blendIndicesObj, sizeof(Int4), 0);
    auto blendWeights = (Vector4*)(void*)mono_array_addr_with_size(blendWeightsObj, sizeof(Vector4), 0);
    Array<VB0SkinnedElementType> vb;
    vb.Resize(vertexCount);
    for (uint32 i = 0; i < vertexCount; i++)
    {
        vb[i].Position = vertices[i];
    }
    if (normalsObj)
    {
        const auto normals = (Vector3*)(void*)mono_array_addr_with_size(normalsObj, sizeof(Vector3), 0);
        if (tangentsObj)
        {
            const auto tangents = (Vector3*)(void*)mono_array_addr_with_size(tangentsObj, sizeof(Vector3), 0);
            for (uint32 i = 0; i < vertexCount; i++)
            {
                // Peek normal and tangent
                const Vector3 normal = normals[i];
                const Vector3 tangent = tangents[i];

                // Calculate bitangent sign
                Vector3 bitangent = Vector3::Normalize(Vector3::Cross(normal, tangent));
                byte sign = static_cast<byte>(Vector3::Dot(Vector3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

                // Set tangent frame
                vb[i].Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
                vb[i].Normal = Float1010102(normal * 0.5f + 0.5f, 0);
            }
        }
        else
        {
            for (uint32 i = 0; i < vertexCount; i++)
            {
                // Peek normal
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
                vb[i].Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
                vb[i].Normal = Float1010102(normal * 0.5f + 0.5f, 0);
            }
        }
    }
    else
    {
        const auto n = Float1010102(Vector3::UnitZ);
        const auto t = Float1010102(Vector3::UnitX);
        for (uint32 i = 0; i < vertexCount; i++)
        {
            vb[i].Normal = n;
            vb[i].Tangent = t;
        }
    }
    if (uvObj)
    {
        const auto uvs = (Vector2*)(void*)mono_array_addr_with_size(uvObj, sizeof(Vector2), 0);
        for (uint32 i = 0; i < vertexCount; i++)
            vb[i].TexCoord = Half2(uvs[i]);
    }
    else
    {
        auto v = Half2(0, 0);
        for (uint32 i = 0; i < vertexCount; i++)
            vb[i].TexCoord = v;
    }
    for (uint32 i = 0; i < vertexCount; i++)
    {
        auto v = blendIndices[i];
        vb[i].BlendIndices = Color32(v.X, v.Y, v.Z, v.W);
    }
    for (uint32 i = 0; i < vertexCount; i++)
    {
        auto v = blendWeights[i];
        vb[i].BlendWeights = Half4(v);
    }

    return mesh->UpdateMesh(vertexCount, triangleCount, vb.Get(), ib);
}

bool SkinnedMesh::UpdateMeshInt(MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* blendIndicesObj, MonoArray* blendWeightsObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj)
{
    return ::UpdateMesh<int32>(this, verticesObj, trianglesObj, blendIndicesObj, blendWeightsObj, normalsObj, tangentsObj, uvObj);
}

bool SkinnedMesh::UpdateMeshUShort(MonoArray* verticesObj, MonoArray* trianglesObj, MonoArray* blendIndicesObj, MonoArray* blendWeightsObj, MonoArray* normalsObj, MonoArray* tangentsObj, MonoArray* uvObj)
{
    return ::UpdateMesh<uint16>(this, verticesObj, trianglesObj, blendIndicesObj, blendWeightsObj, normalsObj, tangentsObj, uvObj);
}

enum class InternalBufferType
{
    VB0 = 0,
    IB16 = 3,
    IB32 = 4,
};

void ConvertMeshData(SkinnedMesh* mesh, InternalBufferType type, MonoArray* resultObj, void* srcData)
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
        Platform::MemoryCopy(managedArrayPtr, srcData, sizeof(VB0SkinnedElementType) * vertices);
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

bool SkinnedMesh::DownloadBuffer(bool forceGpu, MonoArray* resultObj, int32 typeI)
{
    SkinnedMesh* mesh = this;
    InternalBufferType type = (InternalBufferType)typeI;
    auto model = mesh->GetSkinnedModel();
    ASSERT(model && resultObj);

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

    MeshBufferType bufferType;
    switch (type)
    {
    case InternalBufferType::VB0:
        bufferType = MeshBufferType::Vertex0;
        break;
    case InternalBufferType::IB16:
    case InternalBufferType::IB32:
        bufferType = MeshBufferType::Index;
        break;
    default: CRASH;
        return true;
    }
    BytesContainer data;
    if (forceGpu)
    {
        // Get data from GPU
        // TODO: support reusing the input memory buffer to perform a single copy from staging buffer to the input CPU buffer
        auto task = mesh->DownloadDataAsyncGPU(bufferType, data);
        if (task == nullptr)
            return true;
        task->Start();
        if (task->Wait())
        {
            LOG(Error, "Task failed.");
            return true;
        }
    }
    else
    {
        // Get data from CPU
        if (DownloadDataCPU(bufferType, data))
            return true;
    }

    // Convert into managed memory
    ConvertMeshData(mesh, type, resultObj, data.Get());
    return false;
}
