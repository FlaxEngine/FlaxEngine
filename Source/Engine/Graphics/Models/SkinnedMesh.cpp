// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "SkinnedMesh.h"
#include "ModelInstanceEntry.h"
#include "Engine/Content/Assets/Material.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Threading/Task.h"
#include "Engine/Threading/Threading.h"

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

bool SkinnedMesh::Load(uint32 vertices, uint32 triangles, void* vb0, void* ib, bool use16BitIndexBuffer)
{
    // Cache data
    uint32 indicesCount = triangles * 3;
    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);

    GPUBuffer* vertexBuffer = nullptr;
    GPUBuffer* indexBuffer = nullptr;

    // Create vertex buffer
#if GPU_ENABLE_RESOURCE_NAMING
    vertexBuffer = GPUDevice::Instance->CreateBuffer(GetSkinnedModel()->GetPath() + TEXT(".VB"));
#else
	vertexBuffer = GPUDevice::Instance->CreateBuffer(String::Empty);
#endif
    if (vertexBuffer->Init(GPUBufferDescription::Vertex(sizeof(VB0SkinnedElementType), vertices, vb0)))
        goto ERROR_LOAD_END;

    // Create index buffer
#if GPU_ENABLE_RESOURCE_NAMING
    indexBuffer = GPUDevice::Instance->CreateBuffer(GetSkinnedModel()->GetPath() + TEXT(".IB"));
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
    auto model = (SkinnedModel*)_model;

    // Setup GPU resources
    const bool failed = Load(vertexCount, triangleCount, vb, ib, use16BitIndices);
    if (!failed)
    {
        // Calculate mesh bounds
        BoundingBox bounds;
        BoundingBox::FromPoints((Float3*)vb, vertexCount, bounds);
        SetBounds(bounds);

        // Send event (actors using this model can update bounds, etc.)
        model->onLoaded();
    }

    return failed;
}

bool SkinnedMesh::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal) const
{
    // Transform points
    BoundingBox transformedBox;
    Vector3::Transform(_box.Minimum, world, transformedBox.Minimum);
    Vector3::Transform(_box.Maximum, world, transformedBox.Maximum);

    // Test ray on a transformed box
    return transformedBox.Intersects(ray, distance, normal);
}

bool SkinnedMesh::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const
{
    // Transform points
    BoundingBox transformedBox;
    transform.LocalToWorld(_box.Minimum, transformedBox.Minimum);
    transform.LocalToWorld(_box.Maximum, transformedBox.Maximum);

    // Test ray on a transformed box
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
    drawCall.ObjectRadius = info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Lightmap = nullptr;
    drawCall.Surface.LightmapUVsArea = Rectangle::Empty;
    drawCall.Surface.Skinning = info.Skinning;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = Math::FloatSelect(drawCall.World.RotDeterminant(), 1, -1);
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
    drawCall.ObjectRadius = info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
    drawCall.Surface.GeometrySize = _box.GetSize();
    drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
    drawCall.Surface.Lightmap = nullptr;
    drawCall.Surface.LightmapUVsArea = Rectangle::Empty;
    drawCall.Surface.Skinning = info.Skinning;
    drawCall.Surface.LODDitherFactor = lodDitherFactor;
    drawCall.WorldDeterminantSign = Math::FloatSelect(drawCall.World.RotDeterminant(), 1, -1);
    drawCall.PerInstanceRandom = info.PerInstanceRandom;

    // Push draw call to the render lists
    const auto shadowsMode = entry.ShadowsMode & slot.ShadowsMode;
    const auto drawModes = info.DrawModes & material->GetDrawModes();
    if (drawModes != DrawPass::None)
        renderContextBatch.GetMainContext().List->AddDrawCall(renderContextBatch, drawModes, StaticFlags::None, shadowsMode, info.Bounds, drawCall, entry.ReceiveDecals, info.SortOrder);
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

Task* SkinnedMesh::DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const
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

bool SkinnedMesh::DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const
{
    if (_cachedVertexBuffer.IsEmpty())
    {
        PROFILE_CPU();
        auto model = GetSkinnedModel();
        ScopeLock lock(model->Locker);
        if (model->IsVirtual())
        {
            LOG(Error, "Cannot access CPU data of virtual models. Use GPU data download");
            return true;
        }

        // Fetch chunk with data from drive/memory
        const auto chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(_lodIndex);
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
        byte version = stream.ReadByte();
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
                auto blendShapeVerticesData = stream.Move<byte>(blendShapeVertices * sizeof(BlendShapeVertex));
            }
            uint32 indicesCount = triangles * 3;
            bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
            uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
            if (vertices == 0 || triangles == 0)
            {
                LOG(Error, "Invalid mesh data.");
                return true;
            }
            auto vb0 = stream.Move<VB0SkinnedElementType>(vertices);
            auto ib = stream.Move<byte>(indicesCount * ibStride);

            if (i != _index)
                continue;

            // Cache mesh data
            _cachedIndexBufferCount = indicesCount;
            _cachedIndexBuffer.Set(ib, indicesCount * ibStride);
            _cachedVertexBuffer.Set((const byte*)vb0, vertices * sizeof(VB0SkinnedElementType));
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
        result.Link(_cachedVertexBuffer);
        count = _cachedVertexBuffer.Count() / sizeof(VB0SkinnedElementType);
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

#if !COMPILE_WITHOUT_CSHARP

template<typename IndexType>
bool UpdateMesh(SkinnedMesh* mesh, MArray* verticesObj, MArray* trianglesObj, MArray* blendIndicesObj, MArray* blendWeightsObj, MArray* normalsObj, MArray* tangentsObj, MArray* uvObj)
{
    auto model = mesh->GetSkinnedModel();
    ASSERT(model && model->IsVirtual() && verticesObj && trianglesObj && blendIndicesObj && blendWeightsObj);

    // Get buffers data
    const auto vertexCount = (uint32)MCore::Array::GetLength(verticesObj);
    const auto triangleCount = (uint32)MCore::Array::GetLength(trianglesObj) / 3;
    auto vertices = MCore::Array::GetAddress<Float3>(verticesObj);
    auto ib = MCore::Array::GetAddress<IndexType>(trianglesObj);
    auto blendIndices = MCore::Array::GetAddress<Int4>(blendIndicesObj);
    auto blendWeights = MCore::Array::GetAddress<Float4>(blendWeightsObj);
    Array<VB0SkinnedElementType> vb;
    vb.Resize(vertexCount);
    for (uint32 i = 0; i < vertexCount; i++)
    {
        vb[i].Position = vertices[i];
    }
    if (normalsObj)
    {
        const auto normals = MCore::Array::GetAddress<Float3>(normalsObj);
        if (tangentsObj)
        {
            const auto tangents = MCore::Array::GetAddress<Float3>(tangentsObj);
            for (uint32 i = 0; i < vertexCount; i++)
            {
                // Peek normal and tangent
                const Float3 normal = normals[i];
                const Float3 tangent = tangents[i];

                // Calculate bitangent sign
                Float3 bitangent = Float3::Normalize(Float3::Cross(normal, tangent));
                byte sign = static_cast<byte>(Float3::Dot(Float3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

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
                const Float3 normal = normals[i];

                // Calculate tangent
                Float3 c1 = Float3::Cross(normal, Float3::UnitZ);
                Float3 c2 = Float3::Cross(normal, Float3::UnitY);
                Float3 tangent;
                if (c1.LengthSquared() > c2.LengthSquared())
                    tangent = c1;
                else
                    tangent = c2;

                // Calculate bitangent sign
                Float3 bitangent = Float3::Normalize(Float3::Cross(normal, tangent));
                byte sign = static_cast<byte>(Float3::Dot(Float3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

                // Set tangent frame
                vb[i].Tangent = Float1010102(tangent * 0.5f + 0.5f, sign);
                vb[i].Normal = Float1010102(normal * 0.5f + 0.5f, 0);
            }
        }
    }
    else
    {
        const auto n = Float1010102(Float3::UnitZ);
        const auto t = Float1010102(Float3::UnitX);
        for (uint32 i = 0; i < vertexCount; i++)
        {
            vb[i].Normal = n;
            vb[i].Tangent = t;
        }
    }
    if (uvObj)
    {
        const auto uvs = MCore::Array::GetAddress<Float2>(uvObj);
        for (uint32 i = 0; i < vertexCount; i++)
            vb[i].TexCoord = Half2(uvs[i]);
    }
    else
    {
        auto v = Half2::Zero;
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

bool SkinnedMesh::UpdateMeshUInt(MArray* verticesObj, MArray* trianglesObj, MArray* blendIndicesObj, MArray* blendWeightsObj, MArray* normalsObj, MArray* tangentsObj, MArray* uvObj)
{
    return ::UpdateMesh<uint32>(this, verticesObj, trianglesObj, blendIndicesObj, blendWeightsObj, normalsObj, tangentsObj, uvObj);
}

bool SkinnedMesh::UpdateMeshUShort(MArray* verticesObj, MArray* trianglesObj, MArray* blendIndicesObj, MArray* blendWeightsObj, MArray* normalsObj, MArray* tangentsObj, MArray* uvObj)
{
    return ::UpdateMesh<uint16>(this, verticesObj, trianglesObj, blendIndicesObj, blendWeightsObj, normalsObj, tangentsObj, uvObj);
}

enum class InternalBufferType
{
    VB0 = 0,
    IB16 = 3,
    IB32 = 4,
};

MArray* SkinnedMesh::DownloadBuffer(bool forceGpu, MTypeObject* resultType, int32 typeI)
{
    SkinnedMesh* mesh = this;
    InternalBufferType type = (InternalBufferType)typeI;
    auto model = mesh->GetSkinnedModel();
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
            dataCount = data.Length() / sizeof(VB0SkinnedElementType);
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
