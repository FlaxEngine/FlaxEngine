// Copyright (c) Wojciech Figat. All rights reserved.

#include "Model.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Content/Upgraders/ModelAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
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
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Renderer/DrawCall.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Tools/ModelTool/ModelTool.h"
#if USE_EDITOR
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Textures/TextureData.h"
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

bool Model::HasAnyLODInitialized() const
{
    return LODs.HasItems() && LODs.Last().HasAnyMeshInitialized();
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
        chunk->Data.Copy(ToSpan(sdfStream));
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
        if (meshesCount < 0 || meshesCount > MODEL_MAX_MESHES)
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

bool Model::LoadHeader(ReadStream& stream, byte& headerVersion)
{
    if (ModelBase::LoadHeader(stream, headerVersion))
        return true;
    
    // LODs
    byte lods = stream.ReadByte();
    if (lods == 0 || lods > MODEL_MAX_LODS)
        return true;
    LODs.Resize(lods);
    _initialized = true;
    for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        lod._model = this;
        lod._lodIndex = lodIndex;
        stream.ReadFloat(&lod.ScreenSize);

        // Meshes
        uint16 meshesCount;
        stream.ReadUint16(&meshesCount);
        if (meshesCount > MODEL_MAX_MESHES)
            return true;
        ASSERT(lodIndex == 0 || LODs[0].Meshes.Count() >= meshesCount);
        lod.Meshes.Resize(meshesCount, false);
        for (uint16 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
        {
            Mesh& mesh = lod.Meshes[meshIndex];
            mesh.Link(this, lodIndex, meshIndex);

            // Material Slot index
            int32 materialSlotIndex;
            stream.ReadInt32(&materialSlotIndex);
            if (materialSlotIndex < 0 || materialSlotIndex >= MaterialSlots.Count())
            {
                LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, MaterialSlots.Count());
                return true;
            }
            mesh.SetMaterialSlotIndex(materialSlotIndex);

            // Bounds
            BoundingBox box;
            stream.Read(box);
            BoundingSphere sphere;
            stream.Read(sphere);
            mesh.SetBounds(box, sphere);
            
            // Lightmap UVs channel
            int8 lightmapUVs;
            stream.ReadInt8(&lightmapUVs);
            mesh.LightmapUVsIndex = (int32)lightmapUVs;
        }
    }

    return false;
}

#if USE_EDITOR

bool Model::SaveHeader(WriteStream& stream) const
{
    if (ModelBase::SaveHeader(stream))
        return true;
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");
    
    // LODs
    stream.Write((byte)LODs.Count());
    for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
    {
        auto& lod = LODs[lodIndex];
        stream.Write(lod.ScreenSize);

        // Meshes
        stream.Write((uint16)lod.Meshes.Count());
        for (const auto& mesh : lod.Meshes)
        {
            stream.Write(mesh.GetMaterialSlotIndex());
            stream.Write(mesh.GetBox());
            stream.Write(mesh.GetSphere());
            stream.Write((int8)mesh.LightmapUVsIndex);
        }
    }

    return false;
}

bool Model::SaveHeader(WriteStream& stream, const ModelData& modelData)
{
    if (ModelBase::SaveHeader(stream, modelData))
        return true;
    static_assert(MODEL_HEADER_VERSION == 2, "Update code");
    
    // LODs
    stream.Write((byte)modelData.LODs.Count());
    for (int32 lodIndex = 0; lodIndex < modelData.LODs.Count(); lodIndex++)
    {
        auto& lod = modelData.LODs[lodIndex];
        stream.Write(lod.ScreenSize);

        // Meshes
        stream.Write((uint16)lod.Meshes.Count());
        for (const auto& mesh : lod.Meshes)
        {
            BoundingBox box;
            BoundingSphere sphere;
            mesh->CalculateBounds(box, sphere);
            stream.Write(mesh->MaterialSlotIndex);
            stream.Write(box);
            stream.Write(sphere);
            stream.Write((int8)mesh->LightmapUVsIndex);
        }
    }

    return false;
}

bool Model::Save(bool withMeshDataFromGpu, Function<FlaxChunk*(int32)>& getChunk) const
{
    if (ModelBase::Save(withMeshDataFromGpu, getChunk))
        return true;
    
    if (withMeshDataFromGpu)
    {
        // Download SDF data
        if (SDF.Texture)
        {
            auto sdfChunk = getChunk(15);
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
            sdfChunk->Data.Copy(ToSpan(sdfStream));
        }
    }
    else
    {
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

    return false;
}

#endif

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

const ModelLODBase* Model::GetLOD(int32 lodIndex) const
{
    CHECK_RETURN(LODs.IsValidIndex(lodIndex), nullptr);
    return &LODs.Get()[lodIndex];
}

ModelLODBase* Model::GetLOD(int32 lodIndex)
{
    CHECK_RETURN(LODs.IsValidIndex(lodIndex), nullptr);
    return &LODs.Get()[lodIndex];
}

const MeshBase* Model::GetMesh(int32 meshIndex, int32 lodIndex) const
{
    auto& lod = LODs[lodIndex];
    return &lod.Meshes[meshIndex];
}

MeshBase* Model::GetMesh(int32 meshIndex, int32 lodIndex)
{
    auto& lod = LODs[lodIndex];
    return &lod.Meshes[meshIndex];
}

void Model::GetMeshes(Array<const MeshBase*>& meshes, int32 lodIndex) const
{
    LODs[lodIndex].GetMeshes(meshes);
}

void Model::GetMeshes(Array<MeshBase*>& meshes, int32 lodIndex)
{
    LODs[lodIndex].GetMeshes(meshes);
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
   
    // Load asset data (anything but mesh contents that use streaming)
    byte headerVersion;
    if (LoadHeader(headerStream, headerVersion))
        return LoadResult::InvalidData;

    // Load SDF
    auto chunk15 = GetChunk(15);
    if (chunk15 && chunk15->IsLoaded() && EnableModelSDF == 1)
    {
        PROFILE_CPU_NAMED("SDF");
        MemoryReadStream sdfStream(chunk15->Get(), chunk15->Size());
        int32 version;
        sdfStream.Read(version);
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

int32 ModelLOD::GetMeshesCount() const
{
    return Meshes.Count();
}

const MeshBase* ModelLOD::GetMesh(int32 index) const
{
    return Meshes.Get() + index;
}

MeshBase* ModelLOD::GetMesh(int32 index)
{
    return Meshes.Get() + index;
}

void ModelLOD::GetMeshes(Array<MeshBase*>& meshes)
{
    meshes.Resize(Meshes.Count());
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
        meshes[meshIndex] = &Meshes.Get()[meshIndex];
}

void ModelLOD::GetMeshes(Array<const MeshBase*>& meshes) const
{
    meshes.Resize(Meshes.Count());
    for (int32 meshIndex = 0; meshIndex < Meshes.Count(); meshIndex++)
        meshes[meshIndex] = &Meshes.Get()[meshIndex];
}
