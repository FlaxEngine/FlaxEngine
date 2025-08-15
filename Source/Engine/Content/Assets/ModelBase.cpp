// Copyright (c) Wojciech Figat. All rights reserved.

#include "ModelBase.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/Models/MeshBase.h"
#include "Engine/Graphics/Models/MeshDeformation.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"
#if GPU_ENABLE_ASYNC_RESOURCES_CREATION
#include "Engine/Threading/ThreadPoolTask.h"
#define STREAM_TASK_BASE ThreadPoolTask
#else
#include "Engine/Threading/MainThreadTask.h"
#define STREAM_TASK_BASE MainThreadTask
#endif
#if USE_EDITOR
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#endif

/// <summary>
/// Model LOD streaming task.
/// </summary>
class StreamModelLODTask : public STREAM_TASK_BASE
{
private:
    WeakAssetReference<ModelBase> _model;
    int32 _lodIndex;
    FlaxStorage::LockData _dataLock;

public:
    StreamModelLODTask(ModelBase* model, int32 lodIndex)
        : _model(model)
        , _lodIndex(lodIndex)
        , _dataLock(model->Storage->Lock())
    {
    }

    bool HasReference(Object* resource) const override
    {
        return _model == resource;
    }

    bool Run() override
    {
        AssetReference<ModelBase> model = _model.Get();
        if (model == nullptr)
            return true;

        // Get data
        BytesContainer data;
        model->GetLODData(_lodIndex, data);
        if (data.IsInvalid())
        {
            LOG(Warning, "Missing data chunk");
            return true;
        }
        MemoryReadStream stream(data.Get(), data.Length());

        // Load meshes data and pass data to the GPU buffers
        Array<MeshBase*> meshes;
        model->GetMeshes(meshes, _lodIndex);
        byte meshVersion = stream.ReadByte();
        if (meshVersion < 2 || meshVersion > MODEL_MESH_VERSION)
        {
            LOG(Warning, "Unsupported mesh version {}", meshVersion);
            return true;
        }
        for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
        {
            if (model->LoadMesh(stream, meshVersion, meshes[meshIndex]))
            {
                LOG(Warning, "Cannot initialize mesh {} in LOD{} for model \'{}\'", meshIndex, _lodIndex, model->ToString());
                return true;
            }
        }

        // Update residency level
        // Note: this is running on thread pool task so we must be sure that updated LOD is not used at all (for rendering)
        model->_loadedLODs++;
        model->ResidencyChanged();

        return false;
    }

    void OnEnd() override
    {
        // Unlink
        ModelBase* model = _model.Get();
        if (model)
        {
            ASSERT(model->_streamingTask == this);
            model->_streamingTask = nullptr;
            _model = nullptr;
        }
        _dataLock.Release();

        STREAM_TASK_BASE::OnEnd();
    }
};

bool ModelLODBase::HasAnyMeshInitialized() const
{
    // Note: we initialize all meshes at once so the last one can be used to check it.
    const int32 meshCount = GetMeshesCount();
    return meshCount != 0 && GetMesh(meshCount - 1)->IsInitialized();
}

BoundingBox ModelLODBase::GetBox() const
{
    Vector3 min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    const int32 meshCount = GetMeshesCount();
    for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
    {
        GetMesh(meshIndex)->GetBox().GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            min = Vector3::Min(min, corners[i]);
            max = Vector3::Max(max, corners[i]);
        }
    }
    return BoundingBox(min, max);
}

BoundingBox ModelLODBase::GetBox(const Matrix& world) const
{
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    const int32 meshCount = GetMeshesCount();
    for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
    {
        GetMesh(meshIndex)->GetBox().GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            Vector3::Transform(corners[i], world, tmp);
            min = Vector3::Min(min, tmp);
            max = Vector3::Max(max, tmp);
        }
    }
    return BoundingBox(min, max);
}

BoundingBox ModelLODBase::GetBox(const Transform& transform, const MeshDeformation* deformation) const
{
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    const int32 meshCount = GetMeshesCount();
    for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
    {
        BoundingBox box = GetMesh(meshIndex)->GetBox();
        if (deformation)
            deformation->GetBounds(_lodIndex, meshIndex, box);
        box.GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            transform.LocalToWorld(corners[i], tmp);
            min = Vector3::Min(min, tmp);
            max = Vector3::Max(max, tmp);
        }
    }
    return BoundingBox(min, max);
}

ModelBase::~ModelBase()
{
    ASSERT(_streamingTask == nullptr);
}

void ModelBase::SetupMaterialSlots(int32 slotsCount)
{
    CHECK(slotsCount >= 0 && slotsCount < 4096);
    if (!IsVirtual() && WaitForLoaded())
        return;
    ScopeLock lock(Locker);

    const int32 prevCount = MaterialSlots.Count();
    MaterialSlots.Resize(slotsCount, false);

    // Initialize slot names
    for (int32 i = prevCount; i < slotsCount; i++)
        MaterialSlots[i].Name = String::Format(TEXT("Material {0}"), i + 1);
}

MaterialSlot* ModelBase::GetSlot(const StringView& name)
{
    MaterialSlot* result = nullptr;
    for (auto& slot : MaterialSlots)
    {
        if (slot.Name == name)
        {
            result = &slot;
            break;
        }
    }
    return result;
}

ContentLoadTask* ModelBase::RequestLODDataAsync(int32 lodIndex)
{
    const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
    return RequestChunkDataAsync(chunkIndex);
}

void ModelBase::GetLODData(int32 lodIndex, BytesContainer& data) const
{
    const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
    GetChunkData(chunkIndex, data);
}

#if USE_EDITOR

bool ModelBase::Save(bool withMeshDataFromGpu, const StringView& path)
{
    // Validate state
    if (OnCheckSave(path))
        return true;
    if (withMeshDataFromGpu && IsInMainThread())
    {
        LOG(Error, "To save model with GPU mesh buffers it needs to be called from the other thread (not the main thread).");
        return true;
    }
    if (IsVirtual() && !withMeshDataFromGpu)
    {
        LOG(Error, "To save virtual model asset you need to specify 'withMeshDataFromGpu' (it has no other storage container to get data).");
        return true;
    }
    ScopeLock lock(Locker);

    // Use a temporary chunks for data storage for virtual assets
    FlaxChunk* tmpChunks[ASSET_FILE_DATA_CHUNKS];
    Platform::MemoryClear(tmpChunks, sizeof(tmpChunks));
    Array<FlaxChunk> chunks;
    if (IsVirtual())
        chunks.Resize(ASSET_FILE_DATA_CHUNKS);
    Function<FlaxChunk*(int32)> getChunk = [&](int32 index) -> FlaxChunk* {
        return IsVirtual() ? tmpChunks[index] = &chunks[index] : GetOrCreateChunk(index);
    };

    // Save LODs data
    const int32 lodsCount = GetLODsCount();
    if (withMeshDataFromGpu)
    {
        // Fetch runtime mesh data (from GPU)
        MemoryWriteStream meshesStream;
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            meshesStream.SetPosition(0);
            if (SaveLOD(meshesStream, lodIndex))
                return true;
            auto lodChunk = getChunk(MODEL_LOD_TO_CHUNK_INDEX(lodIndex));
            if (lodChunk == nullptr)
                return true;
            lodChunk->Data.Copy(ToSpan(meshesStream));
        }
    }
    else if (!IsVirtual())
    {
        // Load all chunks with a mesh data
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            if (LoadChunk(MODEL_LOD_TO_CHUNK_INDEX(lodIndex)))
                return true;
        }
    }

    // Save custom data
    if (Save(withMeshDataFromGpu, getChunk))
        return true;

    // Save header data
    {
        MemoryWriteStream headerStream(1024);
        if (SaveHeader(headerStream))
            return true;
        auto headerChunk = getChunk(0);
        ASSERT(headerChunk != nullptr);
        headerChunk->Data.Copy(ToSpan(headerStream));
    }

    // Save file
    AssetInitData data;
    data.SerializedVersion = GetSerializedVersion();
    if (IsVirtual())
        Platform::MemoryCopy(_header.Chunks, tmpChunks, sizeof(_header.Chunks));
    const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
    if (IsVirtual())
        Platform::MemoryClear(_header.Chunks, sizeof(_header.Chunks));
    if (saveResult)
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif

bool ModelBase::LoadHeader(ReadStream& stream, byte& headerVersion)
{
    // Basic info
    stream.Read(headerVersion);
    if (headerVersion < 2 || headerVersion > MODEL_HEADER_VERSION)
    {
        LOG(Warning, "Unsupported model asset header version {}", headerVersion);
        return true;
    }
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");
    stream.Read(MinScreenSize);

    // Materials
    int32 materialsCount;
    stream.Read(materialsCount);
    if (materialsCount < 0 || materialsCount > 4096)
        return true;
    MaterialSlots.Resize(materialsCount, false);
    Guid materialId;
    for (auto& slot : MaterialSlots)
    {
        stream.Read(materialId);
        slot.Material = materialId;
        slot.ShadowsMode = (ShadowsCastingMode)stream.ReadByte();
        stream.Read(slot.Name, 11);
    }

    return stream.HasError();
}

bool ModelBase::LoadMesh(MemoryReadStream& stream, byte meshVersion, MeshBase* mesh, MeshData* dataIfReadOnly)
{
    // Load descriptor
    static_assert(MODEL_MESH_VERSION == 2, "Update code");
    uint32 vertices, triangles;
    stream.Read(vertices);
    stream.Read(triangles);
    const uint32 indicesCount = triangles * 3;
    const bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
    const uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
    if (vertices == 0 || triangles == 0)
        return true;
    Array<const void*, FixedAllocation<MODEL_MAX_VB>> vbData;
    Array<GPUVertexLayout*, FixedAllocation<MODEL_MAX_VB>> vbLayout;
    byte vbCount;
    stream.Read(vbCount);
    if (vbCount > MODEL_MAX_VB)
        return true;
    vbData.Resize(vbCount);
    vbLayout.Resize(vbCount);
    for (int32 i = 0; i < vbCount; i++)
    {
        GPUVertexLayout::Elements elements;
        stream.Read(elements);
        vbLayout[i] = GPUVertexLayout::Get(elements);
    }

    // Move over actual mesh data
    for (int32 i = 0; i < vbCount; i++)
    {
        auto layout = vbLayout[i];
        if (!layout)
        {
            LOG(Warning, "Failed to get vertex layout for buffer {}", i);
            return true;
        }
        vbData[i] = stream.Move(vertices * layout->GetStride());
    }
    const auto ibData = stream.Move(indicesCount * ibStride);

    // Pass results if read-only
    if (dataIfReadOnly)
    {
        dataIfReadOnly->Vertices = vertices;
        dataIfReadOnly->Triangles = triangles;
        dataIfReadOnly->IBStride = ibStride;
        dataIfReadOnly->VBData = vbData;
        dataIfReadOnly->VBLayout = vbLayout;
        dataIfReadOnly->IBData = ibData;
        return false;
    }

    // Setup GPU resources
    return mesh->Init(vertices, triangles, vbData, ibData, use16BitIndexBuffer, vbLayout);
}

#if USE_EDITOR

bool ModelBase::SaveHeader(WriteStream& stream) const
{
    // Basic info
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");
    stream.Write(MODEL_HEADER_VERSION);
    stream.Write(MinScreenSize);

    // Materials
    stream.Write(MaterialSlots.Count());
    for (const auto& slot : MaterialSlots)
    {
        stream.Write(slot.Material.GetID());
        stream.Write((byte)slot.ShadowsMode);
        stream.Write(slot.Name, 11);
    }

    return false;
}

bool ModelBase::SaveHeader(WriteStream& stream, const ModelData& modelData)
{
    // Validate data
    if (modelData.LODs.Count() > MODEL_MAX_LODS)
    {
        Log::ArgumentOutOfRangeException(TEXT("LODs"), TEXT("Too many LODs."));
        return true;
    }
    for (int32 lodIndex = 0; lodIndex < modelData.LODs.Count(); lodIndex++)
    {
        auto& lod = modelData.LODs[lodIndex];
        if (lod.Meshes.Count() > MODEL_MAX_MESHES)
        {
            Log::ArgumentOutOfRangeException(TEXT("LOD.Meshes"), TEXT("Too many meshes."));
            return true;
        }
        for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
        {
            auto& mesh = *lod.Meshes[meshIndex];
            if (mesh.MaterialSlotIndex < 0 || mesh.MaterialSlotIndex >= modelData.Materials.Count())
            {
                Log::ArgumentOutOfRangeException(TEXT("MaterialSlotIndex"), TEXT("Incorrect material index used by the mesh."));
                return true;
            }
        }
    }

    // Basic info
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");
    stream.Write(MODEL_HEADER_VERSION);
    stream.Write(modelData.MinScreenSize);

    // Materials
    stream.WriteInt32(modelData.Materials.Count());
    for (const auto& slot : modelData.Materials)
    {
        stream.Write(slot.AssetID);
        stream.Write((byte)slot.ShadowsMode);
        stream.Write(slot.Name, 11);
    }

    return false;
}

bool ModelBase::SaveLOD(WriteStream& stream, int32 lodIndex) const
{
    // Download all meshes buffers from the GPU
    Array<Task*> tasks;
    Array<const MeshBase*> meshes;
    GetMeshes(meshes, lodIndex);
    const int32 meshesCount = meshes.Count();
    struct MeshData
    {
        BytesContainer VB[3];
        BytesContainer IB;
    };
    Array<MeshData> meshesData;
    meshesData.Resize(meshesCount);
    tasks.EnsureCapacity(meshesCount * 4);
    for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
    {
        const auto& mesh = *meshes[meshIndex];
        auto& meshData = meshesData[meshIndex];

        // Vertex Buffer 0 (required)
        auto task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex0, meshData.VB[0]);
        if (task == nullptr)
            return true;
        task->Start();
        tasks.Add(task);

        // Vertex Buffer 1 (optional)
        task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex1, meshData.VB[1]);
        if (task)
        {
            task->Start();
            tasks.Add(task);
        }

        // Vertex Buffer 2 (optional)
        task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex2, meshData.VB[2]);
        if (task)
        {
            task->Start();
            tasks.Add(task);
        }

        // Index Buffer (required)
        task = mesh.DownloadDataGPUAsync(MeshBufferType::Index, meshData.IB);
        if (task == nullptr)
            return true;
        task->Start();
        tasks.Add(task);
    }

    // Wait for async tasks
    if (Task::WaitAll(tasks))
        return true;

    // Create meshes data
    static_assert(MODEL_MESH_VERSION == 2, "Update code");
    stream.Write(MODEL_MESH_VERSION);
    for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
    {
        const auto& mesh = *meshes[meshIndex];
        const auto& meshData = meshesData[meshIndex];
        uint32 vertices = mesh.GetVertexCount();
        uint32 triangles = mesh.GetTriangleCount();
        uint32 indicesCount = triangles * 3;
        bool shouldUse16BitIndexBuffer = indicesCount <= MAX_uint16;
        bool use16BitIndexBuffer = mesh.Use16BitIndexBuffer();
        uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
        uint32 ibSize = indicesCount * ibStride;

        // Validate data
        if (vertices == 0 || triangles == 0)
        {
            LOG(Warning, "Cannot save model with empty meshes.");
            return true;
        }
        Array<const GPUVertexLayout*, FixedAllocation<MODEL_MAX_VB>> vbLayout;
        for (int32 vbIndex = 0; vbIndex < MODEL_MAX_VB; vbIndex++)
        {
            if (vbIndex != 0 && meshData.VB[vbIndex].IsInvalid()) // VB0 is always required
                continue;
            const GPUBuffer* vb = mesh.GetVertexBuffer(vbIndex);
            if (!vb)
                break;
            const GPUVertexLayout* layout = vb->GetVertexLayout();
            if (!layout)
            {
                LOG(Warning, "Invalid vertex buffer {}. Missing vertex layout.", vbIndex);
                return true;
            }
            const uint32 vbSize = vb->GetSize();
            vbLayout.Add(layout);
            if ((uint32)meshData.VB[vbIndex].Length() != vbSize)
            {
                LOG(Warning, "Invalid vertex buffer {} size. Got {} bytes but expected {} bytes. Stride: {}. Layout:\n{}", vbIndex, meshData.VB[vbIndex].Length(), vbSize, vb->GetStride(), layout->GetElementsString());
                return true;
            }
        }
        if ((uint32)meshData.IB.Length() < ibSize)
        {
            LOG(Warning, "Invalid index buffer size. Got {} bytes bytes expected {} bytes. Stride: {}", meshData.IB.Length(), ibSize, ibStride);
            return true;
        }

        // Write descriptor
        stream.Write(vertices);
        stream.Write(triangles);
        byte vbCount = (byte)vbLayout.Count();
        stream.Write(vbCount);
        for (const GPUVertexLayout* layout : vbLayout)
            stream.Write(layout->GetElements());

        // Write actual mesh data
        for (int32 vbIndex = 0; vbIndex < vbCount; vbIndex++)
        {
            const auto& vb = meshData.VB[vbIndex];
            stream.WriteBytes(vb.Get(), vb.Length());
        }
        if (shouldUse16BitIndexBuffer == use16BitIndexBuffer)
        {
            stream.WriteBytes(meshData.IB.Get(), ibSize);
        }
        else if (shouldUse16BitIndexBuffer)
        {
            // Convert 32-bit indices to 16-bit
            auto ib = (const int32*)meshData.IB.Get();
            for (uint32 i = 0; i < indicesCount; i++)
            {
                stream.WriteUint16(ib[i]);
            }
        }
        else
        {
            CRASH;
        }

        // Write custom data
        if (SaveMesh(stream, &mesh))
            return true;
    }

    return false;
}

bool ModelBase::SaveLOD(WriteStream& stream, const ModelData& modelData, int32 lodIndex, bool saveMesh(WriteStream& stream, const ModelData& modelData, int32 lodIndex, int32 meshIndex))
{
    // Create meshes data
    static_assert(MODEL_MESH_VERSION == 2, "Update code");
    stream.Write(MODEL_MESH_VERSION);
    const auto& lod = modelData.LODs[lodIndex];
    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
    {
        const auto& mesh = *lod.Meshes[meshIndex];
        uint32 vertices = (uint32)mesh.Positions.Count();
        uint32 indicesCount = (uint32)mesh.Indices.Count();
        uint32 triangles = indicesCount / 3;
        bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
        const bool isSkinned = mesh.BlendIndices.HasItems() && mesh.BlendWeights.HasItems();

        // Validate data
        if (vertices == 0 || triangles == 0 || indicesCount % 3 != 0)
        {
            LOG(Warning, "Cannot save model with empty meshes.");
            return true;
        }
        for (auto& channel : mesh.UVs)
        {
            if ((uint32)channel.Count() != vertices)
            {
                LOG(Error, "Invalid size of {0} stream.", TEXT("UVs"));
                return true;
            }
        }
        bool hasNormals = mesh.Normals.HasItems();
        if (hasNormals && (uint32)mesh.Normals.Count() != vertices)
        {
            LOG(Error, "Invalid size of {0} stream.", TEXT("Normals"));
            return true;
        }
        bool hasTangents = mesh.Tangents.HasItems();
        if (hasTangents && (uint32)mesh.Tangents.Count() != vertices)
        {
            LOG(Error, "Invalid size of {0} stream.", TEXT("Tangents"));
            return true;
        }
        bool hasBitangentSigns = mesh.BitangentSigns.HasItems();
        if (hasBitangentSigns && (uint32)mesh.BitangentSigns.Count() != vertices)
        {
            LOG(Error, "Invalid size of {0} stream.", TEXT("BitangentSigns"));
            return true;
        }
        bool hasColors = mesh.Colors.HasItems();
        if (hasColors && (uint32)mesh.Colors.Count() != vertices)
        {
            LOG(Error, "Invalid size of {0} stream.", TEXT("Colors"));
            return true;
        }
        if (isSkinned && (uint32)mesh.BlendIndices.Count() != vertices)
        {
            LOG(Error, "Invalid size of {0} stream.", TEXT("BlendIndices"));
            return true;
        }
        if (isSkinned && (uint32)mesh.BlendWeights.Count() != vertices)
        {
            LOG(Error, "Invalid size of {0} stream.", TEXT("BlendWeights"));
            return true;
        }

        // Define vertex buffers layout and packing
        Array<GPUVertexLayout::Elements, FixedAllocation<MODEL_MAX_VB>> vbElements;
        const bool useSeparatePositions = !isSkinned;
        const bool useSeparateColors = !isSkinned;
        PixelFormat positionsFormat = modelData.PositionFormat == ModelData::PositionFormats::Float32 ? PixelFormat::R32G32B32_Float : PixelFormat::R16G16B16A16_Float;
        PixelFormat texCoordsFormat = modelData.TexCoordFormat == ModelData::TexCoordFormats::Float16 ? PixelFormat::R16G16_Float : PixelFormat::R8G8_UNorm;
        PixelFormat blendIndicesFormat = PixelFormat::R8G8B8A8_UInt;
        PixelFormat blendWeightsFormat = PixelFormat::R8G8B8A8_UNorm;
        for (const Int4& indices : mesh.BlendIndices)
        {
            if (indices.MaxValue() > MAX_uint8)
            {
                blendIndicesFormat = PixelFormat::R16G16B16A16_UInt;
                break;
            }
        }
        {
            byte vbIndex = 0;
            // TODO: add option to quantize vertex attributes (eg. 8-bit blend weights, 8-bit texcoords)

            // Position
            if (useSeparatePositions)
            {
                auto& vb0 = vbElements.AddOne();
                vb0.Add({ VertexElement::Types::Position, vbIndex, 0, 0, positionsFormat });
                vbIndex++;
            }

            // General purpose components
            {
                auto& vb = vbElements.AddOne();
                if (!useSeparatePositions)
                    vb.Add({ VertexElement::Types::Position, vbIndex, 0, 0, positionsFormat });
                for (int32 channelIdx = 0; channelIdx < mesh.UVs.Count(); channelIdx++)
                {
                    auto& channel = mesh.UVs.Get()[channelIdx];
                    if (channel.HasItems())
                    {
                        vb.Add({ (VertexElement::Types)((int32)VertexElement::Types::TexCoord0 + channelIdx), vbIndex, 0, 0, texCoordsFormat });
                    }
                }
                vb.Add({ VertexElement::Types::Normal, vbIndex, 0, 0, PixelFormat::R10G10B10A2_UNorm });
                vb.Add({ VertexElement::Types::Tangent, vbIndex, 0, 0, PixelFormat::R10G10B10A2_UNorm });
                if (isSkinned)
                {
                    vb.Add({ VertexElement::Types::BlendIndices, vbIndex, 0, 0, blendIndicesFormat });
                    vb.Add({ VertexElement::Types::BlendWeights, vbIndex, 0, 0, blendWeightsFormat });
                }
                if (!useSeparateColors && hasColors)
                    vb.Add({ VertexElement::Types::Color, vbIndex, 0, 0, PixelFormat::R8G8B8A8_UNorm });
                vbIndex++;
            }

            // Colors
            if (useSeparateColors && hasColors)
            {
                auto& vb = vbElements.AddOne();
                vb.Add({ VertexElement::Types::Color, vbIndex, 0, 0, PixelFormat::R8G8B8A8_UNorm });
                vbIndex++;
            }
        }

        // Write descriptor
        stream.Write(vertices);
        stream.Write(triangles);
        byte vbCount = (byte)vbElements.Count();
        stream.Write(vbCount);
        for (const auto& elements : vbElements)
            stream.Write(elements);

        // Write vertex buffers
        for (int32 vbIndex = 0; vbIndex < vbCount; vbIndex++)
        {
            if (useSeparatePositions && vbIndex == 0 && positionsFormat == PixelFormat::R32G32B32_Float)
            {
                // Fast path for vertex positions of static models using the first buffer
                stream.WriteBytes(mesh.Positions.Get(), sizeof(Float3) * vertices);
                continue;
            }

            // Write vertex components interleaved
            const GPUVertexLayout::Elements& layout = vbElements[vbIndex];
            for (uint32 vertex = 0; vertex < vertices; vertex++)
            {
                for (const VertexElement& element : layout)
                {
                    switch (element.Type)
                    {
                    case VertexElement::Types::Position:
                    {
                        const Float3 position = mesh.Positions.Get()[vertex];
                        if (positionsFormat == PixelFormat::R16G16B16A16_Float)
                        {
                            const Half4 positionEnc(Float4(position, 0.0f));
                            stream.Write(positionEnc);
                        }
                        else //if (positionsFormat == PixelFormat::R32G32B32_Float)
                        {
                            stream.Write(position);
                        }
                        break;
                    }
                    case VertexElement::Types::Color:
                    {
                        const Color32 color(mesh.Colors.Get()[vertex]);
                        stream.Write(color);
                        break;
                    }
                    case VertexElement::Types::Normal:
                    {
                        const Float3 normal = hasNormals ? mesh.Normals.Get()[vertex] : Float3::UnitZ;
                        const FloatR10G10B10A2 normalEnc(normal * 0.5f + 0.5f, 0);
                        stream.Write(normalEnc.Value);
                        break;
                    }
                    case VertexElement::Types::Tangent:
                    {
                        const Float3 normal = hasNormals ? mesh.Normals.Get()[vertex] : Float3::UnitZ;
                        const Float3 tangent = hasTangents ? mesh.Tangents.Get()[vertex] : Float3::UnitX;
                        const float bitangentSign = hasBitangentSigns ? mesh.BitangentSigns.Get()[vertex] : Float3::Dot(Float3::Cross(Float3::Normalize(Float3::Cross(normal, tangent)), normal), tangent);
                        const FloatR10G10B10A2 tangentEnc(tangent * 0.5f + 0.5f, (byte)(bitangentSign < 0 ? 1 : 0));
                        stream.Write(tangentEnc.Value);
                        break;
                    }
                    case VertexElement::Types::BlendIndices:
                    {
                        const Int4 blendIndices = mesh.BlendIndices.Get()[vertex];
                        if (blendIndicesFormat == PixelFormat::R8G8B8A8_UInt)
                        {
                            // 8-bit indices
                            const Color32 blendIndicesEnc(blendIndices.X, blendIndices.Y, blendIndices.Z, blendIndices.W);
                            stream.Write(blendIndicesEnc);
                        }
                        else //if (blendWeightsFormat == PixelFormat::R16G16B16A16_UInt)
                        {
                            // 16-bit indices
                            const uint16 blendIndicesEnc[4] = { (uint16)blendIndices.X, (uint16)blendIndices.Y, (uint16)blendIndices.Z, (uint16)blendIndices.W };
                            stream.Write(blendIndicesEnc);
                        }
                        break;
                    }
                    case VertexElement::Types::BlendWeights:
                    {
                        const Float4 blendWeights = mesh.BlendWeights.Get()[vertex];
                        if (blendWeightsFormat == PixelFormat::R8G8B8A8_UNorm)
                        {
                            // 8-bit weights
                            const Color32 blendWeightsEnc(blendWeights);
                            stream.Write(blendWeightsEnc);
                        }
                        else //if (blendWeightsFormat == PixelFormat::R16G16B16A16_Float)
                        {
                            // 16-bit weights
                            const Half4 blendWeightsEnc(blendWeights);
                            stream.Write(blendWeightsEnc);
                        }
                        break;
                    }
                    case VertexElement::Types::TexCoord0:
                    case VertexElement::Types::TexCoord1:
                    case VertexElement::Types::TexCoord2:
                    case VertexElement::Types::TexCoord3:
                    {
                        const int32 channelIdx = (int32)element.Type - (int32)VertexElement::Types::TexCoord0;
                        const Float2 uv = mesh.UVs.Get()[channelIdx].Get()[vertex];
                        if (texCoordsFormat == PixelFormat::R8G8_UNorm)
                        {
                            stream.Write((uint8)Math::Clamp<int32>((int32)(uv.X * 255), 0, 255));
                            stream.Write((uint8)Math::Clamp<int32>((int32)(uv.Y * 255), 0, 255));
                        }
                        else //if (texCoordsFormat == PixelFormat::R16G16_Float)
                        {
                            const Half2 uvEnc(uv);
                            stream.Write(uvEnc);
                        }
                        break;
                    }
                    default:
                        LOG(Error, "Unsupported vertex element: {}", element.ToString());
                        return true;
                    }
                }
            }
        }

        // Write index buffer
        const uint32* meshIndices = mesh.Indices.Get();
        if (use16BitIndexBuffer)
        {
            for (uint32 i = 0; i < indicesCount; i++)
                stream.Write((uint16)meshIndices[i]);
        }
        else
        {
            stream.WriteBytes(meshIndices, sizeof(uint32) * indicesCount);
        }

        // Write custom data
        if (saveMesh && saveMesh(stream, modelData, lodIndex, meshIndex))
            return true;
    }

    return false;
}

bool ModelBase::SaveMesh(WriteStream& stream, const MeshBase* mesh) const
{
    return false;
}

bool ModelBase::Save(bool withMeshDataFromGpu, Function<FlaxChunk*(int32)>& getChunk) const
{
    return false;
}

#endif

void ModelBase::CancelStreaming()
{
    BinaryAsset::CancelStreaming();

    CancelStreamingTasks();
}

#if USE_EDITOR

void ModelBase::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    BinaryAsset::GetReferences(assets, files);

    for (auto& slot : MaterialSlots)
        assets.Add(slot.Material.GetID());
}

bool ModelBase::Save(const StringView& path)
{
    return Save(false, path);
}

#endif

int32 ModelBase::GetCurrentResidency() const
{
    return _loadedLODs;
}

bool ModelBase::CanBeUpdated() const
{
    // Check if is ready and has no streaming tasks running
    return IsInitialized() && _streamingTask == nullptr;
}

Task* ModelBase::UpdateAllocation(int32 residency)
{
    // Models are not using dynamic allocation feature
    return nullptr;
}

Task* ModelBase::CreateStreamingTask(int32 residency)
{
    ScopeLock lock(Locker);

    const int32 lodMax = GetLODsCount();
    ASSERT(IsInitialized() && Math::IsInRange(residency, 0, lodMax) && _streamingTask == nullptr);
    Task* result = nullptr;
    const int32 lodCount = residency - GetCurrentResidency();

    // Switch if go up or down with residency
    if (lodCount > 0)
    {
        // Allow only to change LODs count by 1
        ASSERT(Math::Abs(lodCount) == 1);

        int32 lodIndex = HighestResidentLODIndex() - 1;

        // Request LOD data
        result = (Task*)RequestLODDataAsync(lodIndex);

        // Add upload data task
        _streamingTask = New<StreamModelLODTask>(this, lodIndex);
        if (result)
            result->ContinueWith(_streamingTask);
        else
            result = _streamingTask;
    }
    else
    {
        // Do the quick data release
        Array<MeshBase*> meshes;
        for (int32 i = HighestResidentLODIndex(); i < lodMax - residency; i++)
        {
            GetMeshes(meshes, i);
            for (MeshBase* mesh : meshes)
                mesh->Release();
        }
        _loadedLODs = residency;
        ResidencyChanged();
    }

    return result;
}

void ModelBase::CancelStreamingTasks()
{
    if (_streamingTask)
    {
        _streamingTask->Cancel();
        ASSERT_LOW_LAYER(_streamingTask == nullptr);
    }
}

void ModelBase::unload(bool isReloading)
{
    // End streaming (if still active)
    if (_streamingTask != nullptr)
    {
        // Cancel streaming task
        _streamingTask->Cancel();
        _streamingTask = nullptr;
    }

    // Cleanup
    MaterialSlots.Resize(0);
    _initialized = false;
    _loadedLODs = 0;
}
