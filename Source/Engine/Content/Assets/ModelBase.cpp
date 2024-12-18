// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ModelBase.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Graphics/Config.h"
#if GPU_ENABLE_ASYNC_RESOURCES_CREATION
#include "Engine/Threading/ThreadPoolTask.h"
#define STREAM_TASK_BASE ThreadPoolTask
#else
#include "Engine/Threading/MainThreadTask.h"
#define STREAM_TASK_BASE MainThreadTask
#endif
#include "SkinnedModel.h" // TODO: remove this
#include "Model.h" // TODO: remove this

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

        // Note: this is running on thread pool task so we must be sure that updated LOD is not used at all (for rendering)

        // Load model LOD (initialize vertex and index buffers)
        // TODO: reformat model data storage to be the same for both assets (only custom per-mesh type data like BlendShapes)
        Array<MeshBase*> meshes;
        model->GetMeshes(meshes, _lodIndex);
        if (model->Is<SkinnedModel>())
        {
            byte version = stream.ReadByte();
            for (int32 i = 0; i < meshes.Count(); i++)
            {
                auto& mesh = (SkinnedMesh&)*meshes[i];

                // #MODEL_DATA_FORMAT_USAGE
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                uint16 blendShapesCount;
                stream.ReadUint16(&blendShapesCount);
                if (blendShapesCount != mesh.BlendShapes.Count())
                {
                    LOG(Warning, "Cannot initialize mesh {} in LOD{} for model \'{}\'. Incorrect blend shapes amount: {} (expected: {})", i, _lodIndex, model->ToString(), blendShapesCount, mesh.BlendShapes.Count());
                    return true;
                }
                for (auto& blendShape : mesh.BlendShapes)
                {
                    blendShape.UseNormals = stream.ReadBool();
                    stream.ReadUint32(&blendShape.MinVertexIndex);
                    stream.ReadUint32(&blendShape.MaxVertexIndex);
                    uint32 blendShapeVertices;
                    stream.ReadUint32(&blendShapeVertices);
                    blendShape.Vertices.Resize(blendShapeVertices);
                    stream.ReadBytes(blendShape.Vertices.Get(), blendShape.Vertices.Count() * sizeof(BlendShapeVertex));
                }
                const uint32 indicesCount = triangles * 3;
                const bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                const uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                if (vertices == 0 || triangles == 0)
                    return true;
                const auto vb0 = stream.Move<VB0SkinnedElementType>(vertices);
                const auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Setup GPU resources
                if (mesh.Load(vertices, triangles, vb0, ib, use16BitIndexBuffer))
                {
                    LOG(Warning, "Cannot initialize mesh {} in LOD{} for model \'{}\'. Vertices: {}, triangles: {}", i, _lodIndex, model->ToString(), vertices, triangles);
                    return true;
                }
            }
        }
        else
        {
            for (int32 i = 0; i < meshes.Count(); i++)
            {
                auto& mesh = (Mesh&)*meshes[i];

                // #MODEL_DATA_FORMAT_USAGE
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                if (vertices == 0 || triangles == 0)
                    return true;
                auto vb0 = stream.Move<VB0ElementType>(vertices);
                auto vb1 = stream.Move<VB1ElementType>(vertices);
                bool hasColors = stream.ReadBool();
                VB2ElementType18* vb2 = nullptr;
                if (hasColors)
                {
                    vb2 = stream.Move<VB2ElementType18>(vertices);
                }
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Setup GPU resources
                if (mesh.Load(vertices, triangles, vb0, vb1, vb2, ib, use16BitIndexBuffer))
                {
                    LOG(Warning, "Cannot initialize mesh {} in LOD{} for model \'{}\'. Vertices: {}, triangles: {}", i, _lodIndex, model->ToString(), vertices, triangles);
                    return true;
                }
            }
        }

        // Update residency level
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
