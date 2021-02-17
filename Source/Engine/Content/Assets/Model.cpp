// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Model.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Content/Upgraders/ModelAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Models/ModelInstanceEntry.h"
#include "Engine/Streaming/StreamingGroup.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#if GPU_ENABLE_ASYNC_RESOURCES_CREATION
#include "Engine/Threading/ThreadPoolTask.h"
#define STREAM_TASK_BASE ThreadPoolTask
#else
#include "Engine/Threading/MainThreadTask.h"
#define STREAM_TASK_BASE MainThreadTask
#endif

#define CHECK_INVALID_BUFFER(buffer) \
    if (buffer->IsValidFor(this) == false) \
	{ \
		LOG(Warning, "Invalid Model Instance Buffer size {0} for Model {1}. It should be {2}. Manual update to proper size.", buffer->Count(), ToString(), MaterialSlots.Count()); \
		buffer->Setup(this); \
	}

REGISTER_BINARY_ASSET_ABSTRACT(ModelBase, "FlaxEngine.ModelBase");

/// <summary>
/// Model LOD streaming task.
/// </summary>
class StreamModelLODTask : public STREAM_TASK_BASE
{
private:

    WeakAssetReference<Model> _asset;
    int32 _lodIndex;
    FlaxStorage::LockData _dataLock;

public:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="model">Parent model</param>
    /// <param name="lodIndex">LOD to stream index</param>
    StreamModelLODTask(Model* model, int32 lodIndex)
        : _asset(model)
        , _lodIndex(lodIndex)
        , _dataLock(model->Storage->Lock())
    {
    }

public:

    // [ThreadPoolTask]
    bool HasReference(Object* resource) const override
    {
        return _asset == resource || (_asset && _asset == resource);
    }

protected:

    // [ThreadPoolTask]
    bool Run() override
    {
        AssetReference<Model> model = _asset.Get();
        if (model == nullptr)
        {
            return true;
        }

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
        if (model->LODs[_lodIndex].Load(stream))
        {
            LOG(Warning, "Cannot load LOD{1} for model \'{0}\'", model->ToString(), _lodIndex);
            return true;
        }

        // Update residency level
        model->_loadedLODs++;

        return false;
    }

    void OnEnd() override
    {
        // Unlink
        if (_asset)
        {
            ASSERT(_asset->_streamingTask == this);
            _asset->_streamingTask = nullptr;
            _asset = nullptr;
        }
        _dataLock.Release();

        // Base
        STREAM_TASK_BASE::OnEnd();
    }
};

REGISTER_BINARY_ASSET(Model, "FlaxEngine.Model", ::New<ModelAssetUpgrader>(), true);

Model::Model(const SpawnParams& params, const AssetInfo* info)
    : ModelBase(params, info, StreamingGroups::Instance()->Models())
{
}

Model::~Model()
{
    // Ensure to be fully disposed
    ASSERT(IsInitialized() == false);
    ASSERT(_streamingTask == nullptr);
}

bool Model::Intersects(const Ray& ray, const Matrix& world, float& distance, Vector3& normal, Mesh** mesh, int32 lodIndex)
{
    return LODs[lodIndex].Intersects(ray, world, distance, normal, mesh);
}

BoundingBox Model::GetBox(const Matrix& world, int32 lodIndex) const
{
    return LODs[lodIndex].GetBox(world);
}

BoundingBox Model::GetBox(int32 lodIndex) const
{
    return LODs[lodIndex].GetBox();
}

void Model::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, StaticFlags flags, bool receiveDecals) const
{
    if (!CanBeRendered())
        return;

    // Select a proper LOD index (model may be culled)
    const BoundingBox box = GetBox(world);
    BoundingSphere sphere;
    BoundingSphere::FromBox(box, sphere);
    int32 lodIndex = RenderTools::ComputeModelLOD(this, sphere.Center, sphere.Radius, renderContext);
    if (lodIndex == -1)
        return;
    lodIndex += renderContext.View.ModelLODBias;
    lodIndex = ClampLODIndex(lodIndex);

    // Draw
    LODs[lodIndex].Draw(renderContext, material, world, flags, receiveDecals);
}

void Model::Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info)
{
    ASSERT(info.Buffer);
    if (!CanBeRendered())
        return;
    const auto frame = Engine::FrameCount;
    const auto modelFrame = info.DrawState->PrevFrame + 1;
    CHECK_INVALID_BUFFER(info.Buffer);

    // Select a proper LOD index (model may be culled)
    int32 lodIndex;
    if (info.ForcedLOD != -1)
    {
        lodIndex = info.ForcedLOD;
    }
    else
    {
        lodIndex = RenderTools::ComputeModelLOD(this, info.Bounds.Center, info.Bounds.Radius, renderContext);
        if (lodIndex == -1)
        {
            // Handling model fade-out transition
            if (modelFrame == frame && info.DrawState->PrevLOD != -1)
            {
                // Check if start transition
                if (info.DrawState->LODTransition == 255)
                {
                    info.DrawState->LODTransition = 0;
                }

                RenderTools::UpdateModelLODTransition(info.DrawState->LODTransition);

                // Check if end transition
                if (info.DrawState->LODTransition == 255)
                {
                    info.DrawState->PrevLOD = lodIndex;
                }
                else
                {
                    const auto prevLOD = ClampLODIndex(info.DrawState->PrevLOD);
                    const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
                    LODs[prevLOD].Draw(renderContext, info, normalizedProgress);
                }
            }

            return;
        }
    }
    lodIndex += info.LODBias + renderContext.View.ModelLODBias;
    lodIndex = ClampLODIndex(lodIndex);

    // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
    if (modelFrame == frame)
    {
        // Check if start transition
        if (info.DrawState->PrevLOD != lodIndex && info.DrawState->LODTransition == 255)
        {
            info.DrawState->LODTransition = 0;
        }

        RenderTools::UpdateModelLODTransition(info.DrawState->LODTransition);

        // Check if end transition
        if (info.DrawState->LODTransition == 255)
        {
            info.DrawState->PrevLOD = lodIndex;
        }
    }
        // Check if there was a gap between frames in drawing this model instance
    else if (modelFrame < frame || info.DrawState->PrevLOD == -1)
    {
        // Reset state
        info.DrawState->PrevLOD = lodIndex;
        info.DrawState->LODTransition = 255;
    }

    // Draw
    if (info.DrawState->PrevLOD == lodIndex)
    {
        LODs[lodIndex].Draw(renderContext, info, 0.0f);
    }
    else if (info.DrawState->PrevLOD == -1)
    {
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        LODs[lodIndex].Draw(renderContext, info, 1.0f - normalizedProgress);
    }
    else
    {
        const auto prevLOD = ClampLODIndex(info.DrawState->PrevLOD);
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        LODs[prevLOD].Draw(renderContext, info, normalizedProgress);
        LODs[lodIndex].Draw(renderContext, info, normalizedProgress - 1.0f);
    }
}

bool Model::SetupLODs(const Span<int32>& meshesCountPerLod)
{
    ScopeLock lock(Locker);

    // Validate input and state
    if (!IsVirtual())
    {
        LOG(Error, "Only virtual models can be updated at runtime.");
        return true;
    }

    return Init(meshesCountPerLod);
}

#if USE_EDITOR

bool Model::Save(bool withMeshDataFromGpu, const StringView& path)
{
    // Validate state
    if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }
    if (IsVirtual() && path.IsEmpty())
    {
        LOG(Error, "To save virtual asset asset you need to specify the target asset path location.");
        return true;
    }
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

    // Create model data header
    MemoryWriteStream headerStream(1024);
    MemoryWriteStream* stream = &headerStream;
    {
        // Min Screen Size
        stream->WriteFloat(MinScreenSize);

        // Amount of material slots
        stream->WriteInt32(MaterialSlots.Count());

        // For each material slot
        for (int32 materialSlotIndex = 0; materialSlotIndex < MaterialSlots.Count(); materialSlotIndex++)
        {
            auto& slot = MaterialSlots[materialSlotIndex];

            const auto id = slot.Material.GetID();
            stream->Write(&id);
            stream->WriteByte(static_cast<byte>(slot.ShadowsMode));
            stream->WriteString(slot.Name, 11);
        }

        // Amount of LODs
        const int32 lods = LODs.Count();
        stream->WriteByte(lods);

        // For each LOD
        for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
        {
            auto& lod = LODs[lodIndex];

            // Screen Size
            stream->WriteFloat(lod.ScreenSize);

            // Amount of meshes
            const int32 meshes = lod.Meshes.Count();
            stream->WriteUint16(meshes);

            // For each mesh
            for (int32 meshIndex = 0; meshIndex < meshes; meshIndex++)
            {
                const auto& mesh = lod.Meshes[meshIndex];

                // Material Slot index
                stream->WriteInt32(mesh.GetMaterialSlotIndex());

                // Box
                const auto box = mesh.GetBox();
                stream->Write(&box);

                // Sphere
                const auto sphere = mesh.GetSphere();
                stream->Write(&sphere);

                // Has Lightmap UVs
                stream->WriteBool(mesh.HasLightmapUVs());
            }
        }
    }

    // Use a temporary chunks for data storage for virtual assets
    FlaxChunk* tmpChunks[ASSET_FILE_DATA_CHUNKS];
    Platform::MemoryClear(tmpChunks, sizeof(tmpChunks));
    Array<FlaxChunk> chunks;
    if (IsVirtual())
        chunks.Resize(ASSET_FILE_DATA_CHUNKS);
#define GET_CHUNK(index) (IsVirtual() ? tmpChunks[index] = &chunks[index] : GetOrCreateChunk(index))

    // Check if use data from drive or from GPU
    if (withMeshDataFromGpu)
    {
        // Download all meshes buffers
        Array<Task*> tasks;
        for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
        {
            auto& lod = LODs[lodIndex];

            const int32 meshesCount = lod.Meshes.Count();
            struct MeshData
            {
                BytesContainer VB0;
                BytesContainer VB1;
                BytesContainer VB2;
                BytesContainer IB;

                uint32 DataSize() const
                {
                    return VB0.Length() + VB1.Length() + VB2.Length() + IB.Length();
                }
            };
            Array<MeshData> meshesData;
            meshesData.Resize(meshesCount);
            tasks.EnsureCapacity(meshesCount * 4);

            for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
            {
                const auto& mesh = lod.Meshes[meshIndex];
                auto& meshData = meshesData[meshIndex];

                // Vertex Buffer 0 (required)
                auto task = mesh.ExtractDataAsync(MeshBufferType::Vertex0, meshData.VB0);
                if (task == nullptr)
                    return true;
                task->Start();
                tasks.Add(task);

                // Vertex Buffer 1 (required)
                task = mesh.ExtractDataAsync(MeshBufferType::Vertex1, meshData.VB1);
                if (task == nullptr)
                    return true;
                task->Start();
                tasks.Add(task);

                // Vertex Buffer 2 (optional)
                task = mesh.ExtractDataAsync(MeshBufferType::Vertex2, meshData.VB2);
                if (task)
                {
                    task->Start();
                    tasks.Add(task);
                }

                // Index Buffer (required)
                task = mesh.ExtractDataAsync(MeshBufferType::Index, meshData.IB);
                if (task == nullptr)
                    return true;
                task->Start();
                tasks.Add(task);
            }

            // Wait for all
            if (Task::WaitAll(tasks))
                return true;
            tasks.Clear();

            // Create meshes data
            {
                int32 dataSize = meshesCount * (2 * sizeof(uint32) + sizeof(bool));
                for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    dataSize += meshesData[meshIndex].DataSize();
                }

                MemoryWriteStream meshesStream(dataSize);

                for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    const auto& mesh = lod.Meshes[meshIndex];
                    const auto& meshData = meshesData[meshIndex];

                    uint32 vertices = mesh.GetVertexCount();
                    uint32 triangles = mesh.GetTriangleCount();
                    bool hasColors = meshData.VB2.IsValid();
                    uint32 vb0Size = vertices * sizeof(VB0ElementType);
                    uint32 vb1Size = vertices * sizeof(VB1ElementType);
                    uint32 vb2Size = vertices * sizeof(VB2ElementType);
                    uint32 indicesCount = triangles * 3;
                    bool shouldUse16BitIndexBuffer = indicesCount <= MAX_uint16;
                    bool use16BitIndexBuffer = mesh.Use16BitIndexBuffer();
                    uint32 ibSize = indicesCount * (use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32));

                    if (vertices == 0 || triangles == 0)
                    {
                        LOG(Warning, "Cannot save model with empty meshes.");
                        return true;
                    }
                    if ((uint32)meshData.VB0.Length() < vb0Size)
                    {
                        LOG(Warning, "Invalid vertex buffer 0 size.");
                        return true;
                    }
                    if ((uint32)meshData.VB1.Length() < vb1Size)
                    {
                        LOG(Warning, "Invalid vertex buffer 1 size.");
                        return true;
                    }
                    if (hasColors && (uint32)meshData.VB2.Length() < vb2Size)
                    {
                        LOG(Warning, "Invalid vertex buffer 2 size.");
                        return true;
                    }
                    if ((uint32)meshData.IB.Length() < ibSize)
                    {
                        LOG(Warning, "Invalid index buffer size.");
                        return true;
                    }

                    meshesStream.WriteUint32(vertices);
                    meshesStream.WriteUint32(triangles);

                    meshesStream.WriteBytes(meshData.VB0.Get(), vb0Size);
                    meshesStream.WriteBytes(meshData.VB1.Get(), vb1Size);

                    meshesStream.WriteBool(hasColors);

                    if (hasColors)
                    {
                        meshesStream.WriteBytes(meshData.VB2.Get(), vb2Size);
                    }

                    if (shouldUse16BitIndexBuffer == use16BitIndexBuffer)
                    {
                        meshesStream.WriteBytes(meshData.IB.Get(), ibSize);
                    }
                    else if (shouldUse16BitIndexBuffer)
                    {
                        auto ib = (const int32*)meshData.IB.Get();
                        for (uint32 i = 0; i < indicesCount; i++)
                        {
                            meshesStream.WriteUint16(ib[i]);
                        }
                    }
                    else
                    {
                        CRASH;
                    }
                }

                // Override LOD data chunk with the fetched GPU meshes memory
                auto lodChunk = GET_CHUNK(MODEL_LOD_TO_CHUNK_INDEX(lodIndex));
                if (lodChunk == nullptr)
                    return true;
                lodChunk->Data.Copy(meshesStream.GetHandle(), meshesStream.GetPosition());
            }
        }
    }
    else
    {
        ASSERT(!IsVirtual());

        // Load all chunks with a mesh data
        for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
        {
            if (LoadChunk(MODEL_LOD_TO_CHUNK_INDEX(lodIndex)))
                return true;
        }
    }

    // Set mesh header data
    auto headerChunk = GET_CHUNK(0);
    ASSERT(headerChunk != nullptr);
    headerChunk->Data.Copy(headerStream.GetHandle(), headerStream.GetPosition());

#undef GET_CHUNK

    // Save
    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
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

bool Model::Init(const Span<int32>& meshesCountPerLod)
{
    if (meshesCountPerLod.IsInvalid() || meshesCountPerLod.Length() > MODEL_MAX_LODS)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }

    // Dispose previous data and disable streaming (will start data uploading tasks manually)
    stopStreaming();

    // Setup
    MaterialSlots.Resize(1);
    MinScreenSize = 0.0f;

    // Setup LODs
    for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
    {
        LODs[lodIndex].Dispose();
    }
    LODs.Resize(meshesCountPerLod.Length());
    _loadedLODs = meshesCountPerLod.Length();

    // Setup meshes
    for (int32 lodIndex = 0; lodIndex < meshesCountPerLod.Length(); lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod._model = this;
        lod.ScreenSize = 1.0f;
        const int32 meshesCount = meshesCountPerLod[lodIndex];
        if (meshesCount <= 0 || meshesCount > MODEL_MAX_MESHES)
            return true;

        lod.Meshes.Resize(meshesCount);
        for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
        {
            lod.Meshes[meshIndex].Init(this, lodIndex, meshIndex, 0, BoundingBox::Zero, BoundingSphere::Empty, true);
        }
    }

    return false;
}

void Model::SetupMaterialSlots(int32 slotsCount)
{
    ModelBase::SetupMaterialSlots(slotsCount);

    // Adjust meshes indices for slots
    for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
    {
        for (int32 meshIndex = 0; meshIndex < LODs[lodIndex].Meshes.Count(); meshIndex++)
        {
            auto& mesh = LODs[lodIndex].Meshes[meshIndex];
            if (mesh.GetMaterialSlotIndex() >= slotsCount)
                mesh.SetMaterialSlotIndex(slotsCount - 1);
        }
    }
}

void Model::InitAsVirtual()
{
    // Init with a single LOD and one mesh
    int32 meshesCount = 1;
    Init(ToSpan(&meshesCount, 1));

    // Base
    BinaryAsset::InitAsVirtual();
}

int32 Model::GetMaxResidency() const
{
    return LODs.Count();
}

int32 Model::GetCurrentResidency() const
{
    return _loadedLODs;
}

int32 Model::GetAllocatedResidency() const
{
    return LODs.Count();
}

bool Model::CanBeUpdated() const
{
    // Check if is ready and has no streaming tasks running
    return IsInitialized() && _streamingTask == nullptr;
}

Task* Model::UpdateAllocation(int32 residency)
{
    // Models are not using dynamic allocation feature
    return nullptr;
}

Task* Model::CreateStreamingTask(int32 residency)
{
    ScopeLock lock(Locker);

    ASSERT(IsInitialized() && Math::IsInRange(residency, 0, LODs.Count()) && _streamingTask == nullptr);
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
        ASSERT(IsInMainThread());

        // Do the quick data release
        for (int32 i = HighestResidentLODIndex(); i < LODs.Count() - residency; i++)
            LODs[i].Unload();
        _loadedLODs = residency;
    }

    return result;
}

Asset::LoadResult Model::load()
{
    // Get header chunk
    auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;
    MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());
    ReadStream* stream = &headerStream;

    // Min Screen Size
    stream->ReadFloat(&MinScreenSize);

    // Amount of material slots
    int32 materialSlotsCount;
    stream->ReadInt32(&materialSlotsCount);
    if (materialSlotsCount <= 0 || materialSlotsCount > 4096)
        return LoadResult::InvalidData;
    MaterialSlots.Resize(materialSlotsCount, false);

    // For each material slot
    for (int32 materialSlotIndex = 0; materialSlotIndex < materialSlotsCount; materialSlotIndex++)
    {
        auto& slot = MaterialSlots[materialSlotIndex];

        // Material
        Guid materialId;
        stream->Read(&materialId);
        slot.Material = materialId;

        // Shadows Mode
        slot.ShadowsMode = static_cast<ShadowsCastingMode>(stream->ReadByte());

        // Name
        stream->ReadString(&slot.Name, 11);
    }

    // Amount of LODs
    byte lods;
    stream->ReadByte(&lods);
    if (lods == 0 || lods > MODEL_MAX_LODS)
        return LoadResult::InvalidData;
    LODs.Resize(lods);

    // For each LOD
    for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod._model = this;

        // Screen Size
        stream->ReadFloat(&lod.ScreenSize);

        // Amount of meshes
        uint16 meshesCount;
        stream->ReadUint16(&meshesCount);
        if (meshesCount == 0 || meshesCount > MODEL_MAX_MESHES)
            return LoadResult::InvalidData;
        ASSERT(lodIndex == 0 || LODs[0].Meshes.Count() >= meshesCount);

        // Allocate memory
        lod.Meshes.Resize(meshesCount, false);

        // For each mesh
        for (uint16 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
        {
            // Material Slot index
            int32 materialSlotIndex;
            stream->ReadInt32(&materialSlotIndex);
            if (materialSlotIndex < 0 || materialSlotIndex >= materialSlotsCount)
            {
                LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, materialSlotsCount);
                return LoadResult::InvalidData;
            }

            // Box
            BoundingBox box;
            stream->Read(&box);

            // Sphere
            BoundingSphere sphere;
            stream->Read(&sphere);

            // Has Lightmap UVs
            bool hasLightmapUVs = stream->ReadBool();

            lod.Meshes[meshIndex].Init(this, lodIndex, meshIndex, materialSlotIndex, box, sphere, hasLightmapUVs);
        }
    }

#if BUILD_DEBUG || BUILD_DEVELOPMENT
    // Validate LODs
    for (int32 lodIndex = 1; lodIndex < LODs.Count(); lodIndex++)
    {
        const auto prevSS = LODs[lodIndex - 1].ScreenSize;
        const auto thisSS = LODs[lodIndex].ScreenSize;
        if (prevSS <= thisSS)
        {
            LOG(Warning, "Model LOD {0} has invalid screen size compared to LOD {1} (asset: {2})", lodIndex, lodIndex - 1, GetPath());
        }
    }
#endif

    // Request resource streaming
    startStreaming(true);

    return LoadResult::Ok;
}

void Model::unload(bool isReloading)
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
    for (int32 i = 0; i < LODs.Count(); i++)
        LODs[i].Dispose();
    LODs.Clear();
    _loadedLODs = 0;
}

bool Model::init(AssetInitData& initData)
{
    // Validate
    if (initData.SerializedVersion != SerializedVersion)
    {
        LOG(Error, "Invalid serialized model version.");
        return true;
    }

    return false;
}

AssetChunksFlag Model::getChunksToPreload() const
{
    // Note: we don't preload any LODs here because it's done by the Streaming Manager
    return GET_CHUNK_FLAG(0);
}
