// Copyright (c) Wojciech Figat. All rights reserved.

#include "MeshBase.h"
#include "MeshAccessor.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Assets/ModelBase.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/GPUContext.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Threading/Task.h"

static_assert(MODEL_MAX_VB == 3, "Update code in mesh to match amount of vertex buffers.");

namespace
{
#if !COMPILE_WITHOUT_CSHARP
    template<typename IndexType>
    bool UpdateTriangles(MeshBase* mesh, int32 triangleCount, const MArray* trianglesObj)
    {
        const auto model = mesh->GetModelBase();
        ASSERT(model && model->IsVirtual() && trianglesObj);

        // Get buffer data
        ASSERT(MCore::Array::GetLength(trianglesObj) / 3 >= triangleCount);
        auto ib = MCore::Array::GetAddress<IndexType>(trianglesObj);

        return mesh->UpdateTriangles(triangleCount, ib);
    }
#endif
}

MeshAccessor::Stream::Stream(Span<byte> data, PixelFormat format, int32 stride)
    : _data(data)
    , _format(PixelFormat::Unknown)
    , _stride(stride)
{
    auto sampler = PixelFormatSampler::Get(format);
    if (sampler)
    {
        _format = format;
        _sampler = *sampler;
    }
    else if (format != PixelFormat::Unknown)
    {
        LOG(Error, "Unsupported pixel format '{}' to sample vertex attribute.", ScriptingEnum::ToString(format));
    }
}

Span<byte> MeshAccessor::Stream::GetData() const
{
    return _data;
}

PixelFormat MeshAccessor::Stream::GetFormat() const
{
    return _format;
}

int32 MeshAccessor::Stream::GetStride() const
{
    return _stride;
}

int32 MeshAccessor::Stream::GetCount() const
{
    return _data.Length() / _stride;
}

bool MeshAccessor::Stream::IsValid() const
{
    return _format != PixelFormat::Unknown;
}

bool MeshAccessor::Stream::IsLinear(PixelFormat expectedFormat) const
{
    return _format == expectedFormat && _stride == PixelFormatExtensions::SizeInBytes(_format);
}

void MeshAccessor::Stream::SetLinear(const void* data)
{
    Platform::MemoryCopy(_data.Get(), data, _data.Length());
}

void MeshAccessor::Stream::Set(Span<Float2> src)
{
    const int32 count = GetCount();
    ASSERT(src.Length() >= count);
    if (IsLinear(PixelFormat::R32G32_Float))
    {
        Platform::MemoryCopy(_data.Get(), src.Get(), _data.Length());
    }
    else
    {
        for (int32 i = 0; i < count; i++)
            _sampler.Write(_data.Get() + i * _stride, Float4(src.Get()[i], 0, 0));
    }
}

void MeshAccessor::Stream::Set(Span<Float3> src)
{
    const int32 count = GetCount();
    ASSERT(src.Length() >= count);
    if (IsLinear(PixelFormat::R32G32B32_Float))
    {
        Platform::MemoryCopy(_data.Get(), src.Get(), _data.Length());
    }
    else
    {
        for (int32 i = 0; i < count; i++)
            _sampler.Write(_data.Get() + i * _stride, Float4(src.Get()[i], 0));
    }
}

inline void MeshAccessor::Stream::Set(Span<::Color> src)
{
    const int32 count = GetCount();
    ASSERT(src.Length() >= count);
    if (IsLinear(PixelFormat::R32G32B32A32_Float))
    {
        Platform::MemoryCopy(_data.Get(), src.Get(), _data.Length());
    }
    else
    {
        for (int32 i = 0; i < count; i++)
            _sampler.Write(_data.Get() + i * _stride, Float4(src.Get()[i]));
    }
}

void MeshAccessor::Stream::CopyTo(Span<Float2> dst) const
{
    const int32 count = GetCount();
    ASSERT(dst.Length() >= count);
    if (IsLinear(PixelFormat::R32G32_Float))
    {
        Platform::MemoryCopy(dst.Get(), _data.Get(), _data.Length());
    }
    else
    {
        for (int32 i = 0; i < count; i++)
            dst.Get()[i] = Float2(_sampler.Read(_data.Get() + i * _stride));
    }
}

void MeshAccessor::Stream::CopyTo(Span<Float3> dst) const
{
    const int32 count = GetCount();
    ASSERT(dst.Length() >= count);
    if (IsLinear(PixelFormat::R32G32B32_Float))
    {
        Platform::MemoryCopy(dst.Get(), _data.Get(), _data.Length());
    }
    else
    {
        for (int32 i = 0; i < count; i++)
            dst.Get()[i] = Float3(_sampler.Read(_data.Get() + i * _stride));
    }
}

void MeshAccessor::Stream::CopyTo(Span<::Color> dst) const
{
    const int32 count = GetCount();
    ASSERT(dst.Length() >= count);
    if (IsLinear(PixelFormat::R32G32B32A32_Float))
    {
        Platform::MemoryCopy(dst.Get(), _data.Get(), _data.Length());
    }
    else
    {
        for (int32 i = 0; i < count; i++)
            dst.Get()[i] = ::Color(_sampler.Read(_data.Get() + i * _stride));
    }
}

bool MeshAccessor::LoadMesh(const MeshBase* mesh, bool forceGpu, Span<MeshBufferType> buffers)
{
    CHECK_RETURN(mesh, true);
    constexpr MeshBufferType allBuffers[(int32)MeshBufferType::MAX] = { MeshBufferType::Index, MeshBufferType::Vertex0, MeshBufferType::Vertex1, MeshBufferType::Vertex2 };
    if (buffers.IsInvalid())
        buffers = Span<MeshBufferType>(allBuffers, ARRAY_COUNT(allBuffers));
    Array<BytesContainer, FixedAllocation<4>> meshBuffers;
    Array<GPUVertexLayout*, FixedAllocation<4>> meshLayouts;
    if (mesh->DownloadData(buffers, meshBuffers, meshLayouts, forceGpu))
        return true;
    for (int32 i = 0; i < buffers.Length(); i++)
    {
        _data[(int32)buffers[i]] = MoveTemp(meshBuffers[i]);
        _layouts[(int32)buffers[i]] = meshLayouts[i];
    }
    _formats[(int32)MeshBufferType::Index] = mesh->Use16BitIndexBuffer() ? PixelFormat::R16_UInt : PixelFormat::R32_UInt;
    return false;
}

bool MeshAccessor::LoadBuffer(MeshBufferType bufferType, Span<byte> bufferData, GPUVertexLayout* layout)
{
    CHECK_RETURN(layout, true);
    CHECK_RETURN(bufferData.IsValid(), true);
    const int32 idx = (int32)bufferType;
    _data[idx].Link(bufferData);
    _layouts[idx] = layout;
    return false;
}

bool MeshAccessor::LoadFromMeshData(const void* meshDataPtr)
{
    if (!meshDataPtr)
        return true;
    const auto& meshData = *(const ModelBase::MeshData*)meshDataPtr;
    if (meshData.VBData.Count() != meshData.VBLayout.Count())
        return true;
    _data[(int32)MeshBufferType::Index].Link((const byte*)meshData.IBData, meshData.IBStride * meshData.Triangles * 3);
    if (meshData.VBData.Count() > 0 && meshData.VBLayout[0])
    {
        constexpr int32 idx = (int32)MeshBufferType::Vertex0;
        _data[idx].Link((const byte*)meshData.VBData[0], meshData.VBLayout[0]->GetStride() * meshData.Vertices);
        _layouts[idx] = meshData.VBLayout[0];
    }
    if (meshData.VBData.Count() > 1 && meshData.VBLayout[1])
    {
        constexpr int32 idx = (int32)MeshBufferType::Vertex1;
        _data[idx].Link((const byte*)meshData.VBData[1], meshData.VBLayout[1]->GetStride() * meshData.Vertices);
        _layouts[idx] = meshData.VBLayout[1];
    }
    if (meshData.VBData.Count() > 2 && meshData.VBLayout[2])
    {
        constexpr  int32 idx = (int32)MeshBufferType::Vertex2;
        _data[idx].Link((const byte*)meshData.VBData[2], meshData.VBLayout[2]->GetStride() * meshData.Vertices);
        _layouts[idx] = meshData.VBLayout[2];
    }
    return false;
}

bool MeshAccessor::AllocateBuffer(MeshBufferType bufferType, int32 count, GPUVertexLayout* layout)
{
    CHECK_RETURN(count, true);
    CHECK_RETURN(layout, true);
    const int32 idx = (int32)bufferType;
    _data[idx].Allocate(count * layout->GetStride());
    _layouts[idx] = layout;
    return false;
}

bool MeshAccessor::AllocateBuffer(MeshBufferType bufferType, int32 count, PixelFormat format)
{
    CHECK_RETURN(count, true);
    const int32 stride = PixelFormatExtensions::SizeInBytes(format);
    CHECK_RETURN(stride, true);
    const int32 idx = (int32)bufferType;
    _data[idx].Allocate(count * stride);
    _formats[idx] = format;
    return false;
}

bool MeshAccessor::UpdateMesh(MeshBase* mesh, bool calculateBounds)
{
    CHECK_RETURN(mesh, true);
    constexpr int32 IB = (int32)MeshBufferType::Index;
    constexpr int32 VB0 = (int32)MeshBufferType::Vertex0;
    constexpr int32 VB1 = (int32)MeshBufferType::Vertex1;
    constexpr int32 VB2 = (int32)MeshBufferType::Vertex2;

    uint32 vertices = 0, triangles = 0;
    const void* ibData = nullptr;
    bool use16BitIndexBuffer = false;
    Array<const void*, FixedAllocation<3>> vbData;
    Array<GPUVertexLayout*, FixedAllocation<3>> vbLayout;
    vbData.Resize(3);
    vbLayout.Resize(3);
    vbData.SetAll(nullptr);
    vbLayout.SetAll(nullptr);
    if (_data[VB0].IsValid())
    {
        vbData[0] = _data[VB0].Get();
        vbLayout[0] = _layouts[VB0];
        vertices = _data[VB0].Length() / _layouts[VB0]->GetStride();
    }
    if (_data[VB1].IsValid())
    {
        vbData[1] = _data[VB1].Get();
        vbLayout[1] = _layouts[VB1];
        vertices = _data[VB1].Length() / _layouts[VB1]->GetStride();
    }
    if (_data[VB2].IsValid())
    {
        vbData[2] = _data[VB2].Get();
        vbLayout[2] = _layouts[VB2];
        vertices = _data[VB2].Length() / _layouts[VB2]->GetStride();
    }
    if (_data[IB].IsValid() && _formats[IB] != PixelFormat::Unknown)
    {
        ibData = _data[IB].Get();
        use16BitIndexBuffer = _formats[IB] == PixelFormat::R16_UInt;
        triangles = _data[IB].Length() / PixelFormatExtensions::SizeInBytes(_formats[IB]) / 3;
    }

    if (mesh->Init(vertices, triangles, vbData, ibData, use16BitIndexBuffer, vbLayout))
        return true;

    if (calculateBounds)
    {
        // Calculate mesh bounds
        BoundingBox bounds;
        auto positionStream = Position();
        CHECK_RETURN(positionStream.IsValid(), true);
        if (positionStream.IsLinear(PixelFormat::R32G32B32_Float))
        {
            Span<byte> positionData = positionStream.GetData();
            BoundingBox::FromPoints((const Float3*)positionData.Get(), positionData.Length() / sizeof(Float3), bounds);
        }
        else
        {
            Float3 min = Float3::Maximum, max = Float3::Minimum;
            for (uint32 i = 0; i < vertices; i++)
            {
                Float3 pos = positionStream.GetFloat3(i);
                Float3::Min(min, pos, min);
                Float3::Max(max, pos, max);
            }
            bounds = BoundingBox(min, max);
        }
        mesh->SetBounds(bounds);
    }

    return false;
}

MeshAccessor::Stream MeshAccessor::Index()
{
    Span<byte> data;
    PixelFormat format = PixelFormat::Unknown;
    int32 stride = 0;
    auto& ib = _data[(int32)MeshBufferType::Index];
    if (ib.IsValid())
    {
        data = ib;
        format = _formats[(int32)MeshBufferType::Index];
        stride = PixelFormatExtensions::SizeInBytes(format);
    }
    return Stream(data, format, stride);
}

MeshAccessor::Stream MeshAccessor::Attribute(VertexElement::Types attribute)
{
    Span<byte> data;
    PixelFormat format = PixelFormat::Unknown;
    int32 stride = 0;
    for (int32 vbIndex = 0; vbIndex < 3 && format == PixelFormat::Unknown; vbIndex++)
    {
        static_assert((int32)MeshBufferType::Vertex0 == 1, "Update code.");
        const int32 idx = vbIndex + 1;
        auto layout = _layouts[idx];
        if (!layout)
            continue;
        for (const VertexElement& e : layout->GetElements())
        {
            auto& vb = _data[idx];
            if (e.Type == attribute && vb.IsValid())
            {
                data = vb.Slice(e.Offset);
                format = e.Format;
                stride = layout->GetStride();
                break;
            }
        }
    }
    return Stream(data, format, stride);
}

MeshBase::~MeshBase()
{
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[0]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[1]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[2]);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
}

bool MeshBase::HasVertexColors() const
{
    return _vertexBuffers[2] != nullptr && _vertexBuffers[2]->IsAllocated();
}

void MeshBase::SetMaterialSlotIndex(int32 value)
{
    if (value < 0 || value >= _model->MaterialSlots.Count())
    {
        LOG(Warning, "Cannot set mesh material slot to {0} while model has {1} slots.", value, _model->MaterialSlots.Count());
        return;
    }
    _materialSlotIndex = value;
}

void MeshBase::SetBounds(const BoundingBox& box)
{
    _box = box;
    BoundingSphere::FromBox(box, _sphere);
    _hasBounds = true;
    if (_model && _model->IsLoaded())
    {
        // Send event (actors using this model can update bounds, etc.)
        _model->onLoaded();
    }
}

void MeshBase::SetBounds(const BoundingBox& box, const BoundingSphere& sphere)
{
    _box = box;
    _sphere = sphere;
    _hasBounds = true;
    if (_model && _model->IsLoaded())
    {
        // Send event (actors using this model can update bounds, etc.)
        _model->onLoaded();
    }
}

GPUVertexLayout* MeshBase::GetVertexLayout() const
{
    return GPUVertexLayout::Get(Span<GPUBuffer*>(_vertexBuffers, MODEL_MAX_VB));
}

bool MeshBase::Init(uint32 vertices, uint32 triangles, const Array<const void*, FixedAllocation<MODEL_MAX_VB>>& vbData, const void* ibData, bool use16BitIndexBuffer, Array<GPUVertexLayout*, FixedAllocation<MODEL_MAX_VB>> vbLayout)
{
    CHECK_RETURN(vbData.HasItems() && vertices, true);
    CHECK_RETURN(ibData, true);
    CHECK_RETURN(vbLayout.Count() >= vbData.Count(), true);
    const uint32 indices = triangles * 3;
    if (use16BitIndexBuffer)
    {
        CHECK_RETURN(indices <= MAX_uint16, true);
    }
    ASSERT(_model);
    GPUBuffer* vertexBuffer0 = nullptr;
    GPUBuffer* vertexBuffer1 = nullptr;
    GPUBuffer* vertexBuffer2 = nullptr;
    GPUBuffer* indexBuffer = nullptr;

    // Create GPU buffers
#if GPU_ENABLE_RESOURCE_NAMING
    const String& modelPath = _model->GetPath();
#define MESH_BUFFER_NAME(postfix) modelPath + TEXT(postfix)
#else
#define MESH_BUFFER_NAME(postfix) String::Empty
#endif
    vertexBuffer0 = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".VB0"));
    if (vertexBuffer0->Init(GPUBufferDescription::Vertex(vbLayout[0], vertices, vbData[0])))
        goto ERROR_LOAD_END;
    if (vbData.Count() >= 2 && vbData[1])
    {
        vertexBuffer1 = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".VB1"));
        if (vertexBuffer1->Init(GPUBufferDescription::Vertex(vbLayout[1], vertices, vbData[1])))
            goto ERROR_LOAD_END;
    }
    if (vbData.Count() >= 3 && vbData[2])
    {
        vertexBuffer2 = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".VB2"));
        if (vertexBuffer2->Init(GPUBufferDescription::Vertex(vbLayout[2], vertices, vbData[2])))
            goto ERROR_LOAD_END;
    }
    indexBuffer = GPUDevice::Instance->CreateBuffer(MESH_BUFFER_NAME(".IB"));
    if (indexBuffer->Init(GPUBufferDescription::Index(use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32), indices, ibData)))
        goto ERROR_LOAD_END;

    // Init collision proxy
#if MODEL_USE_PRECISE_MESH_INTERSECTS
    if (use16BitIndexBuffer)
        _collisionProxy.Init<uint16>(vertices, triangles, (const Float3*)vbData[0], (const uint16*)ibData);
    else
        _collisionProxy.Init<uint32>(vertices, triangles, (const Float3*)vbData[0], (const uint32*)ibData);
#endif

    // Free old buffers
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[0]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[1]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[2]);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);

    // Initialize
    _vertexBuffers[0] = vertexBuffer0;
    _vertexBuffers[1] = vertexBuffer1;
    _vertexBuffers[2] = vertexBuffer2;
    _indexBuffer = indexBuffer;
    _triangles = triangles;
    _vertices = vertices;
    _use16BitIndexBuffer = use16BitIndexBuffer;
    _cachedVertexBuffers[0].Release();
    _cachedVertexBuffers[1].Release();
    _cachedVertexBuffers[2].Release();
    Platform::MemoryClear(_cachedVertexLayouts, sizeof(_cachedVertexLayouts));

    return false;

#undef MESH_BUFFER_NAME
ERROR_LOAD_END:

    SAFE_DELETE_GPU_RESOURCE(vertexBuffer0);
    SAFE_DELETE_GPU_RESOURCE(vertexBuffer1);
    SAFE_DELETE_GPU_RESOURCE(vertexBuffer2);
    SAFE_DELETE_GPU_RESOURCE(indexBuffer);
    return true;
}

void MeshBase::Release()
{
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[0]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[1]);
    SAFE_DELETE_GPU_RESOURCE(_vertexBuffers[2]);
    SAFE_DELETE_GPU_RESOURCE(_indexBuffer);
#if MODEL_USE_PRECISE_MESH_INTERSECTS
    _collisionProxy.Clear();
#endif
    _triangles = 0;
    _vertices = 0;
    _use16BitIndexBuffer = false;
    _cachedIndexBuffer.Release();
    _cachedVertexBuffers[0].Release();
    _cachedVertexBuffers[1].Release();
    _cachedVertexBuffers[2].Release();
    Platform::MemoryClear(_cachedVertexLayouts, sizeof(_cachedVertexLayouts));
}

bool MeshBase::UpdateTriangles(uint32 triangleCount, const void* ib, bool use16BitIndices)
{
    uint32 indicesCount = triangleCount * 3;
    uint32 ibStride = use16BitIndices ? sizeof(uint16) : sizeof(uint32);
    if (!_indexBuffer)
        _indexBuffer = GPUDevice::Instance->CreateBuffer(TEXT("DynamicMesh.IB"));
    if (_indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
    {
        _triangles = 0;
        return true;
    }

    // TODO: update collision proxy

    _triangles = triangleCount;
    _use16BitIndexBuffer = use16BitIndices;
    return false;
}

bool MeshBase::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal) const
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
#if MODEL_USE_PRECISE_MESH_INTERSECTS
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

bool MeshBase::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal) const
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
#if MODEL_USE_PRECISE_MESH_INTERSECTS
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

bool MeshBase::DownloadDataGPU(MeshBufferType type, BytesContainer& result, GPUVertexLayout** layout) const
{
    GPUBuffer* buffer = nullptr;
    switch (type)
    {
    case MeshBufferType::Index:
        buffer = _indexBuffer;
        break;
    case MeshBufferType::Vertex0:
        buffer = _vertexBuffers[0];
        if (layout && buffer)
            *layout = buffer->GetVertexLayout();
        break;
    case MeshBufferType::Vertex1:
        buffer = _vertexBuffers[1];
        if (layout && buffer)
            *layout = buffer->GetVertexLayout();
        break;
    case MeshBufferType::Vertex2:
        buffer = _vertexBuffers[2];
        if (layout && buffer)
            *layout = buffer->GetVertexLayout();
        break;
    }
    return buffer && buffer->DownloadData(result);
}

Task* MeshBase::DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result, GPUVertexLayout** layout) const
{
    GPUBuffer* buffer = nullptr;
    switch (type)
    {
    case MeshBufferType::Index:
        buffer = _indexBuffer;
        break;
    case MeshBufferType::Vertex0:
        buffer = _vertexBuffers[0];
        if (layout && buffer)
            *layout = buffer->GetVertexLayout();
        break;
    case MeshBufferType::Vertex1:
        buffer = _vertexBuffers[1];
        if (layout && buffer)
            *layout = buffer->GetVertexLayout();
        break;
    case MeshBufferType::Vertex2:
        buffer = _vertexBuffers[2];
        if (layout && buffer)
            *layout = buffer->GetVertexLayout();
        break;
    }
    return buffer ? buffer->DownloadDataAsync(result) : nullptr;
}

bool MeshBase::DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count, GPUVertexLayout** layout) const
{
    if (_cachedVertexBuffers[0].IsInvalid())
    {
        PROFILE_CPU();
        auto model = GetModelBase();
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
        ModelBase::MeshData meshData;

        // Seek to find mesh location
        byte meshVersion = stream.ReadByte();
        for (int32 meshIndex = 0; meshIndex <= _index; meshIndex++)
        {
            if (model->LoadMesh(stream, meshVersion, model->GetMesh(meshIndex, _lodIndex), &meshData))
                return true;
            if (meshIndex != _index)
                continue;

            // Cache mesh data
            _cachedVertexBufferCount = meshData.Vertices;
            _cachedIndexBufferCount = (int32)meshData.Triangles * 3;
            _cachedIndexBuffer.Copy((const byte*)meshData.IBData, _cachedIndexBufferCount * (int32)meshData.IBStride);
            for (int32 vb = 0; vb < meshData.VBData.Count(); vb++)
            {
                _cachedVertexBuffers[vb].Copy((const byte*)meshData.VBData[vb], (int32)(meshData.VBLayout[vb]->GetStride() * meshData.Vertices));
                _cachedVertexLayouts[vb] = meshData.VBLayout[vb];
            }
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
        result.Link(_cachedVertexBuffers[0]);
        count = _cachedVertexBufferCount;
        if (layout)
            *layout = _cachedVertexLayouts[0];
        break;
    case MeshBufferType::Vertex1:
        result.Link(_cachedVertexBuffers[1]);
        count = _cachedVertexBufferCount;
        if (layout)
            *layout = _cachedVertexLayouts[1];
        break;
    case MeshBufferType::Vertex2:
        result.Link(_cachedVertexBuffers[2]);
        count = _cachedVertexBufferCount;
        if (layout)
            *layout = _cachedVertexLayouts[2];
        break;
    default:
        return true;
    }
    return false;
}

bool MeshBase::DownloadData(Span<MeshBufferType> types, Array<BytesContainer, FixedAllocation<4>>& buffers, Array<GPUVertexLayout*, FixedAllocation<4>>& layouts, bool forceGpu) const
{
    PROFILE_CPU();
    buffers.Resize(types.Length());
    layouts.Resize(types.Length());
    layouts.SetAll(nullptr);
    auto model = _model;
    model->Locker.Lock();
    
    // Virtual assets always fetch from GPU memory
    forceGpu |= model->IsVirtual();

    if (forceGpu)
    {
        if (!IsInitialized())
        {
            model->Locker.Unlock();
            LOG(Error, "Cannot load mesh data from GPU if it's not loaded.");
            return true;
        }

        // Get data from GPU (start of series of async tasks that copy GPU-read data into staging buffers)
        Array<Task*, FixedAllocation<(int32)MeshBufferType::MAX>> tasks;
        for (int32 i = 0; i < types.Length(); i++)
        {
            auto task = DownloadDataGPUAsync(types[i], buffers[i], &layouts[i]);
            if (!task)
            {
                model->Locker.Unlock();
                return true;
            }
            task->Start();
            tasks.Add(task);
        }
        
        // Wait for async tasks
        model->Locker.Unlock();
        if (Task::WaitAll(tasks))
        {
            LOG(Error, "Failed to download mesh data from GPU.");
            return true;
        }
        model->Locker.Lock();
    }
    else
    {
        // Get data from CPU
        for (int32 i = 0; i < types.Length(); i++)
        {
            int32 count = 0;
            if (DownloadDataCPU(types[i], buffers[i], count, &layouts[i]))
            {
                model->Locker.Unlock();
                return true;
            }
        }
    }

    model->Locker.Unlock();
    return false;
}

void MeshBase::GetDrawCallGeometry(DrawCall& drawCall) const
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

void MeshBase::Render(GPUContext* context) const
{
    if (!IsInitialized())
        return;
    context->BindVB(ToSpan(_vertexBuffers, 3));
    context->BindIB(_indexBuffer);
    context->DrawIndexed(_triangles * 3);
}

ScriptingObject* MeshBase::GetParentModel() const
{
    return _model;
}

#if !COMPILE_WITHOUT_CSHARP

bool MeshBase::UpdateTrianglesUInt(int32 triangleCount, const MArray* trianglesObj)
{
    return ::UpdateTriangles<uint32>(this, triangleCount, trianglesObj);
}

bool MeshBase::UpdateTrianglesUShort(int32 triangleCount, const MArray* trianglesObj)
{
    return ::UpdateTriangles<uint16>(this, triangleCount, trianglesObj);
}

MArray* MeshBase::DownloadIndexBuffer(bool forceGpu, MTypeObject* resultType, bool use16Bit)
{
    ScopeLock lock(GetModelBase()->Locker);

    // Get index buffer data from the mesh (CPU or GPU)
    MeshAccessor accessor;
    MeshBufferType bufferTypes[1] = { MeshBufferType::Index };
    if (accessor.LoadMesh(this, forceGpu, ToSpan(bufferTypes, 1)))
        return nullptr;
    auto indexStream = accessor.Index();
    if (!indexStream.IsValid())
        return nullptr;
    auto indexData = indexStream.GetData();
    auto indexCount = indexStream.GetCount();
    auto indexStride = indexStream.GetStride();

    // Convert into managed array
    MArray* result = MCore::Array::New(MCore::Type::GetClass(INTERNAL_TYPE_OBJECT_GET(resultType)), indexCount);
    void* managedArrayPtr = MCore::Array::GetAddress(result);
    if (use16Bit)
    {
        if (indexStride == sizeof(uint16))
        {
            Platform::MemoryCopy(managedArrayPtr, indexData.Get(), indexData.Length());
        }
        else
        {
            auto dst = (uint16*)managedArrayPtr;
            auto src = (const uint32*)indexData.Get();
            for (int32 i = 0; i < indexCount; i++)
                dst[i] = src[i];
        }
    }
    else
    {
        if (indexStride == sizeof(uint16))
        {
            auto dst = (uint32*)managedArrayPtr;
            auto src = (const uint16*)indexData.Get();
            for (int32 i = 0; i < indexCount; i++)
                dst[i] = src[i];
        }
        else
        {
            Platform::MemoryCopy(managedArrayPtr, indexData.Get(), indexData.Length());
        }
    }

    return result;
}

bool MeshBase::DownloadData(int32 count, MeshBufferType* types, BytesContainer& buffer0, BytesContainer& buffer1, BytesContainer& buffer2, BytesContainer& buffer3, GPUVertexLayout*& layout0, GPUVertexLayout*& layout1, GPUVertexLayout*& layout2, GPUVertexLayout*& layout3, bool forceGpu) const
{
    layout0 = layout1 = layout2 = layout3 = nullptr;
    Array<BytesContainer, FixedAllocation<4>> meshBuffers;
    Array<GPUVertexLayout*, FixedAllocation<4>> meshLayouts;
    if (DownloadData(Span<MeshBufferType>(types, count), meshBuffers, meshLayouts, forceGpu))
        return true;
    if (count > 0)
    {
        buffer0 = meshBuffers[0];
        layout0 = meshLayouts[0];
        if (count > 1)
        {
            buffer1 = meshBuffers[1];
            layout1 = meshLayouts[1];
            if (count > 2)
            {
                buffer2 = meshBuffers[2];
                layout2 = meshLayouts[2];
                if (count > 3)
                {
                    buffer3 = meshBuffers[3];
                    layout3 = meshLayouts[3];
                }
            }
        }
    }
    return false;
}

#endif
