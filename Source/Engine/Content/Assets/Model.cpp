// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Model.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Content/Upgraders/ModelAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Debug/DebugDraw.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/Models/ModelInstanceEntry.h"
#include "Engine/Streaming/StreamingGroup.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Async/Tasks/GPUUploadTextureMipTask.h"
#include "Engine/Graphics/Models/MeshDeformation.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Tools/ModelTool/ModelTool.h"
#if USE_EDITOR
#include "Engine/Serialization/MemoryWriteStream.h"
#endif

REGISTER_BINARY_ASSET_ABSTRACT(ModelBase, "FlaxEngine.ModelBase");

class StreamModelSDFTask : public GPUUploadTextureMipTask
{
private:
    WeakAssetReference<Model> _asset;
    FlaxStorage::LockData _dataLock;

public:
    StreamModelSDFTask(Model* model, GPUTexture* texture, const Span<byte>& data, int32 mipIndex, int32 rowPitch, int32 slicePitch)
        : GPUUploadTextureMipTask(texture, mipIndex, data, rowPitch, slicePitch, false)
        , _asset(model)
        , _dataLock(model->Storage->Lock())
    {
    }

    bool HasReference(Object* resource) const override
    {
        return _asset == resource;
    }

    Result run(GPUTasksContext* context) override
    {
        AssetReference<Model> model = _asset.Get();
        if (model == nullptr)
            return Result::MissingResources;
        return GPUUploadTextureMipTask::run(context);
    }

    void OnEnd() override
    {
        _dataLock.Release();

        // Base
        GPUUploadTextureMipTask::OnEnd();
    }
};

REGISTER_BINARY_ASSET_WITH_UPGRADER(Model, "FlaxEngine.Model", ModelAssetUpgrader, true);

static byte EnableModelSDF = 0;

Model::Model(const SpawnParams& params, const AssetInfo* info)
    : ModelBase(params, info, StreamingGroups::Instance()->Models())
{
    if (EnableModelSDF == 0 && GPUDevice::Instance)
    {
        const bool enable = GPUDevice::Instance->GetFeatureLevel() >= FeatureLevel::SM5;
        EnableModelSDF = enable ? 1 : 2;
    }
}

Model::~Model()
{
    ASSERT(_streamingTask == nullptr);
}

bool Model::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, Mesh** mesh, int32 lodIndex)
{
    return LODs[lodIndex].Intersects(ray, world, distance, normal, mesh);
}

bool Model::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, Mesh** mesh, int32 lodIndex)
{
    return LODs[lodIndex].Intersects(ray, transform, distance, normal, mesh);
}

BoundingBox Model::GetBox(const Matrix& world, int32 lodIndex) const
{
    return LODs[lodIndex].GetBox(world);
}

BoundingBox Model::GetBox(int32 lodIndex) const
{
    return LODs[lodIndex].GetBox();
}

void Model::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, StaticFlags flags, bool receiveDecals, int8 sortOrder) const
{
    if (!CanBeRendered())
        return;

    // Select a proper LOD index (model may be culled)
    const BoundingBox box = GetBox(world);
    BoundingSphere sphere;
    BoundingSphere::FromBox(box, sphere);
    int32 lodIndex = RenderTools::ComputeModelLOD(this, sphere.Center - renderContext.View.Origin, (float)sphere.Radius, renderContext);
    if (lodIndex == -1)
        return;
    lodIndex += renderContext.View.ModelLODBias;
    lodIndex = ClampLODIndex(lodIndex);

    // Draw
    LODs[lodIndex].Draw(renderContext, material, world, flags, receiveDecals, DrawPass::Default, 0, sortOrder);
}

template<typename ContextType>
FORCE_INLINE void ModelDraw(Model* model, const RenderContext& renderContext, const ContextType& context, const Mesh::DrawInfo& info)
{
    ASSERT(info.Buffer);
    if (!model->CanBeRendered())
        return;
    if (!info.Buffer->IsValidFor(model))
		info.Buffer->Setup(model);
    const auto frame = Engine::FrameCount;
    const auto modelFrame = info.DrawState->PrevFrame + 1;

    // Select a proper LOD index (model may be culled)
    int32 lodIndex;
    if (info.ForcedLOD != -1)
    {
        lodIndex = info.ForcedLOD;
    }
    else
    {
        lodIndex = RenderTools::ComputeModelLOD(model, info.Bounds.Center, (float)info.Bounds.Radius, renderContext);
        if (lodIndex == -1)
        {
            // Handling model fade-out transition
            if (modelFrame == frame && info.DrawState->PrevLOD != -1 && !renderContext.View.IsSingleFrame)
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
                    const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
                    const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
                    model->LODs.Get()[prevLOD].Draw(renderContext, info, normalizedProgress);
                }
            }

            return;
        }
    }
    lodIndex += info.LODBias + renderContext.View.ModelLODBias;
    lodIndex = model->ClampLODIndex(lodIndex);

    if (renderContext.View.IsSingleFrame)
    {
    }
    // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
    else if (modelFrame == frame)
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
    if (info.DrawState->PrevLOD == lodIndex || renderContext.View.IsSingleFrame)
    {
        model->LODs.Get()[lodIndex].Draw(context, info, 0.0f);
    }
    else if (info.DrawState->PrevLOD == -1)
    {
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        model->LODs.Get()[lodIndex].Draw(context, info, 1.0f - normalizedProgress);
    }
    else
    {
        const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
        model->LODs.Get()[prevLOD].Draw(context, info, normalizedProgress);
        model->LODs.Get()[lodIndex].Draw(context, info, normalizedProgress - 1.0f);
    }
}

void Model::Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info)
{
    ModelDraw(this, renderContext, renderContext, info);
}

void Model::Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info)
{
    ModelDraw(this, renderContextBatch.GetMainContext(), renderContextBatch, info);
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
            stream->Write(id);
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
                stream->WriteBoundingBox(box);

                // Sphere
                const auto sphere = mesh.GetSphere();
                stream->WriteBoundingSphere(sphere);

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
                auto task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex0, meshData.VB0);
                if (task == nullptr)
                    return true;
                task->Start();
                tasks.Add(task);

                // Vertex Buffer 1 (required)
                task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex1, meshData.VB1);
                if (task == nullptr)
                    return true;
                task->Start();
                tasks.Add(task);

                // Vertex Buffer 2 (optional)
                task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex2, meshData.VB2);
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

                    // #MODEL_DATA_FORMAT_USAGE
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

        // Download SDF data
        if (SDF.Texture)
        {
            auto sdfChunk = GET_CHUNK(15);
            if (sdfChunk == nullptr)
                return true;
            MemoryWriteStream sdfStream;
            sdfStream.WriteInt32(1); // Version
            ModelSDFHeader data(SDF, SDF.Texture->GetDescription());
            sdfStream.WriteBytes(&data, sizeof(data));
            TextureData sdfTextureData;
            if (SDF.Texture->DownloadData(sdfTextureData))
                return true;
            for (int32 mipLevel = 0; mipLevel < sdfTextureData.Items[0].Mips.Count(); mipLevel++)
            {
                auto& mip = sdfTextureData.Items[0].Mips[mipLevel];
                ModelSDFMip mipData(mipLevel, mip);
                sdfStream.WriteBytes(&mipData, sizeof(mipData));
                sdfStream.WriteBytes(mip.Data.Get(), mip.Data.Length());
            }
            sdfChunk->Data.Copy(sdfStream.GetHandle(), sdfStream.GetPosition());
        }
    }
    else
    {
        // Load all chunks with a mesh data
        for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
        {
            if (LoadChunk(MODEL_LOD_TO_CHUNK_INDEX(lodIndex)))
                return true;
        }

        if (SDF.Texture)
        {
            // SDF data from file (only if has no cached texture data)
            if (LoadChunk(15))
                return true;
        }
        else
        {
            // No SDF texture
            ReleaseChunk(15);
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

bool Model::GenerateSDF(float resolutionScale, int32 lodIndex, bool cacheData, float backfacesThreshold, bool useGPU)
{
    if (EnableModelSDF == 2)
        return true; // Not supported
    ScopeLock lock(Locker);
    if (!HasAnyLODInitialized())
        return true;
    if (IsInMainThread() && IsVirtual())
    {
        // TODO: could be supported if algorithm could run on a GPU and called during rendering
        LOG(Warning, "Cannot generate SDF for virtual models on a main thread.");
        return true;
    }
    lodIndex = Math::Clamp(lodIndex, HighestResidentLODIndex(), LODs.Count() - 1);

    // Generate SDF
#if USE_EDITOR
    cacheData &= Storage != nullptr; // Cache only if has storage linked
    MemoryWriteStream sdfStream;
    MemoryWriteStream* outputStream = cacheData ? &sdfStream : nullptr;
#else
    class MemoryWriteStream* outputStream = nullptr;
#endif
    Locker.Unlock();
    const bool failed = ModelTool::GenerateModelSDF(this, nullptr, resolutionScale, lodIndex, &SDF, outputStream, GetPath(), backfacesThreshold, useGPU);
    Locker.Lock();
    if (failed)
        return true;

#if USE_EDITOR
    // Set asset data
    if (cacheData)
    {
        auto chunk = GetOrCreateChunk(15);
        chunk->Data.Copy(sdfStream.GetHandle(), sdfStream.GetPosition());
        chunk->Flags |= FlaxChunkFlags::KeepInMemory; // Prevent GC-ing chunk data so it will be properly saved
    }
#endif

    return false;
}

void Model::SetSDF(const SDFData& sdf)
{
    ScopeLock lock(Locker);
    if (SDF.Texture == sdf.Texture)
        return;
    SAFE_DELETE_GPU_RESOURCE(SDF.Texture);
    SDF = sdf;
    ReleaseChunk(15);
}

bool Model::Init(const Span<int32>& meshesCountPerLod)
{
    if (meshesCountPerLod.IsInvalid() || meshesCountPerLod.Length() > MODEL_MAX_LODS)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }

    // Dispose previous data and disable streaming (will start data uploading tasks manually)
    StopStreaming();

    // Setup
    MaterialSlots.Resize(1);
    MinScreenSize = 0.0f;
    SAFE_DELETE_GPU_RESOURCE(SDF.Texture);

    // Setup LODs
    LODs.Resize(meshesCountPerLod.Length());
    _initialized = true;

    // Setup meshes
    for (int32 lodIndex = 0; lodIndex < meshesCountPerLod.Length(); lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod.Link(this, lodIndex);
        lod.ScreenSize = 1.0f;
        const int32 meshesCount = meshesCountPerLod[lodIndex];
        if (meshesCount <= 0 || meshesCount > MODEL_MAX_MESHES)
            return true;

        lod.Meshes.Resize(meshesCount);
        for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
        {
            lod.Meshes[meshIndex].Link(this, lodIndex, meshIndex);
        }
    }

    // Update resource residency
    _loadedLODs = meshesCountPerLod.Length();
    ResidencyChanged();

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

int32 Model::GetLODsCount() const
{
    return LODs.Count();
}

void Model::GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex)
{
    auto& lod = LODs[lodIndex];
    meshes.Resize(lod.Meshes.Count());
    for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
        meshes[meshIndex] = &lod.Meshes[meshIndex];
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

int32 Model::GetAllocatedResidency() const
{
    return LODs.Count();
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
        stream->Read(materialId);
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
    _initialized = true;

    // For each LOD
    for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod._model = this;
        lod._lodIndex = lodIndex;

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
            Mesh& mesh = lod.Meshes[meshIndex];
            mesh.Link(this, lodIndex, meshIndex);

            // Material Slot index
            int32 materialSlotIndex;
            stream->ReadInt32(&materialSlotIndex);
            if (materialSlotIndex < 0 || materialSlotIndex >= materialSlotsCount)
            {
                LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, materialSlotsCount);
                return LoadResult::InvalidData;
            }
            mesh.SetMaterialSlotIndex(materialSlotIndex);

            // Bounds
            BoundingBox box;
            stream->ReadBoundingBox(&box);
            BoundingSphere sphere;
            stream->ReadBoundingSphere(&sphere);
            mesh.SetBounds(box, sphere);

            // Has Lightmap UVs
            bool hasLightmapUVs = stream->ReadBool();
            mesh.LightmapUVsIndex = hasLightmapUVs ? 1 : -1;
        }
    }

    // Load SDF
    auto chunk15 = GetChunk(15);
    if (chunk15 && chunk15->IsLoaded() && EnableModelSDF == 1)
    {
        PROFILE_CPU_NAMED("SDF");
        MemoryReadStream sdfStream(chunk15->Get(), chunk15->Size());
        int32 version;
        sdfStream.ReadInt32(&version);
        switch (version)
        {
        case 1:
        {
            ModelSDFHeader data;
            sdfStream.ReadBytes(&data, sizeof(data));
            if (!SDF.Texture)
            {
                String name;
#if !BUILD_RELEASE
                name = GetPath() + TEXT(".SDF");
#endif
                SDF.Texture = GPUDevice::Instance->CreateTexture(name);
            }
            if (SDF.Texture->Init(GPUTextureDescription::New3D(data.Width, data.Height, data.Depth, data.Format, GPUTextureFlags::ShaderResource, data.MipLevels)))
                return LoadResult::Failed;
            SDF.LocalToUVWMul = data.LocalToUVWMul;
            SDF.LocalToUVWAdd = data.LocalToUVWAdd;
            SDF.WorldUnitsPerVoxel = data.WorldUnitsPerVoxel;
            SDF.MaxDistance = data.MaxDistance;
            SDF.LocalBoundsMin = data.LocalBoundsMin;
            SDF.LocalBoundsMax = data.LocalBoundsMax;
            SDF.ResolutionScale = data.ResolutionScale;
            SDF.LOD = data.LOD;
            for (int32 mipLevel = 0; mipLevel < data.MipLevels; mipLevel++)
            {
                ModelSDFMip mipData;
                sdfStream.ReadBytes(&mipData, sizeof(mipData));
                void* mipBytes = sdfStream.Move(mipData.SlicePitch);
                auto task = ::New<StreamModelSDFTask>(this, SDF.Texture, Span<byte>((byte*)mipBytes, mipData.SlicePitch), mipData.MipIndex, mipData.RowPitch, mipData.SlicePitch);
                task->Start();
            }
            break;
        }
        default:
            LOG(Warning, "Unknown SDF data version {0} in {1}", version, ToString());
            break;
        }
    }

#if !BUILD_RELEASE
    // Validate LODs
    for (int32 lodIndex = 1; lodIndex < LODs.Count(); lodIndex++)
    {
        const auto prevSS = LODs[lodIndex - 1].ScreenSize;
        const auto thisSS = LODs[lodIndex].ScreenSize;
        if (prevSS <= thisSS)
        {
            LOG(Warning, "Model LOD {0} has invalid screen size compared to LOD {1} (asset: {2})", lodIndex, lodIndex - 1, ToString());
        }
    }
#endif

    // Request resource streaming
    StartStreaming(true);

    return LoadResult::Ok;
}

void Model::unload(bool isReloading)
{
    ModelBase::unload(isReloading);

    // Cleanup
    SAFE_DELETE_GPU_RESOURCE(SDF.Texture);
    LODs.Clear();
}

AssetChunksFlag Model::getChunksToPreload() const
{
    // Note: we don't preload any LODs here because it's done by the Streaming Manager
    return GET_CHUNK_FLAG(0) | GET_CHUNK_FLAG(15);
}

bool ModelLOD::Intersects(const Ray& ray, const Matrix& world, Real& distance, Vector3& normal, Mesh** mesh)
{
    bool result = false;
    Real closest = MAX_Real;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        Real dst;
        Vector3 nrm;
        if (Meshes[i].Intersects(ray, world, dst, nrm) && dst < closest)
        {
            result = true;
            *mesh = &Meshes[i];
            closest = dst;
            closestNormal = nrm;
        }
    }
    distance = closest;
    normal = closestNormal;
    return result;
}

bool ModelLOD::Intersects(const Ray& ray, const Transform& transform, Real& distance, Vector3& normal, Mesh** mesh)
{
    bool result = false;
    Real closest = MAX_Real;
    Vector3 closestNormal = Vector3::Up;
    for (int32 i = 0; i < Meshes.Count(); i++)
    {
        Real dst;
        Vector3 nrm;
        if (Meshes[i].Intersects(ray, transform, dst, nrm) && dst < closest)
        {
            result = true;
            *mesh = &Meshes[i];
            closest = dst;
            closestNormal = nrm;
        }
    }
    distance = closest;
    normal = closestNormal;
    return result;
}

BoundingBox ModelLOD::GetBox(const Matrix& world) const
{
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
    {
        const auto& mesh = Meshes[meshIndex];
        mesh.GetBox().GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            Vector3::Transform(corners[i], world, tmp);
            min = Vector3::Min(min, tmp);
            max = Vector3::Max(max, tmp);
        }
    }
    return BoundingBox(min, max);
}

BoundingBox ModelLOD::GetBox(const Transform& transform, const MeshDeformation* deformation) const
{
    Vector3 tmp, min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
    {
        const auto& mesh = Meshes[meshIndex];
        BoundingBox box = mesh.GetBox();
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

BoundingBox ModelLOD::GetBox() const
{
    Vector3 min = Vector3::Maximum, max = Vector3::Minimum;
    Vector3 corners[8];
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
    {
        Meshes[meshIndex].GetBox().GetCorners(corners);
        for (int32 i = 0; i < 8; i++)
        {
            min = Vector3::Min(min, corners[i]);
            max = Vector3::Max(max, corners[i]);
        }
    }
    return BoundingBox(min, max);
}
