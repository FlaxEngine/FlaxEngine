// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL

#include "ModelTool.h"
#include "MeshAccelerationStructure.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/RandomStream.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Threading/JobSystem.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUBuffer.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Shader.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Engine/Units.h"
#if USE_EDITOR
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Graphics/Models/SkeletonUpdater.h"
#include "Engine/Graphics/Models/SkeletonMapping.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentImporters/CreateMaterial.h"
#include "Engine/ContentImporters/CreateMaterialInstance.h"
#include "Engine/ContentImporters/CreateCollisionData.h"
#include "Engine/Serialization/Serialization.h"
#include "Editor/Utilities/EditorUtilities.h"
#include "Engine/Animations/Graph/AnimGraph.h"
#include <ThirdParty/meshoptimizer/meshoptimizer.h>
#endif

ModelSDFHeader::ModelSDFHeader(const ModelBase::SDFData& sdf, const GPUTextureDescription& desc)
    : LocalToUVWMul(sdf.LocalToUVWMul)
    , WorldUnitsPerVoxel(sdf.WorldUnitsPerVoxel)
    , LocalToUVWAdd(sdf.LocalToUVWAdd)
    , MaxDistance(sdf.MaxDistance)
    , LocalBoundsMin(sdf.LocalBoundsMin)
    , MipLevels(desc.MipLevels)
    , LocalBoundsMax(sdf.LocalBoundsMax)
    , Width(desc.Width)
    , Height(desc.Height)
    , Depth(desc.Depth)
    , Format(desc.Format)
    , ResolutionScale(sdf.ResolutionScale)
    , LOD(sdf.LOD)
{
}

ModelSDFMip::ModelSDFMip(int32 mipIndex, uint32 rowPitch, uint32 slicePitch)
    : MipIndex(mipIndex)
    , RowPitch(rowPitch)
    , SlicePitch(slicePitch)
{
}

ModelSDFMip::ModelSDFMip(int32 mipIndex, const TextureMipData& mip)
    : MipIndex(mipIndex)
    , RowPitch(mip.RowPitch)
    , SlicePitch(mip.Data.Length())
{
}

class GPUModelSDFTask : public GPUTask
{
    ConditionVariable* _signal;
    AssetReference<Shader> _shader;
    Model* _inputModel;
    ModelData* _modelData;
    int32 _lodIndex;
    Int3 _resolution;
    ModelBase::SDFData* _sdf;
    GPUBuffer *_sdfSrc, *_sdfDst;
    GPUTexture* _sdfResult;
    Float3 _xyzToLocalMul, _xyzToLocalAdd;

    const uint32 ThreadGroupSize = 64;
    GPU_CB_STRUCT(Data {
        Int3 Resolution;
        uint32 ResolutionSize;
        float MaxDistance;
        uint32 VertexStride;
        int32 Index16bit;
        uint32 TriangleCount;
        Float3 VoxelToPosMul;
        float WorldUnitsPerVoxel;
        Float3 VoxelToPosAdd;
        uint32 ThreadGroupsX;
        });

public:
    GPUModelSDFTask(ConditionVariable& signal, Model* inputModel, ModelData* modelData, int32 lodIndex, const Int3& resolution, ModelBase::SDFData* sdf, GPUTexture* sdfResult, const Float3& xyzToLocalMul, const Float3& xyzToLocalAdd)
        : GPUTask(Type::Custom)
        , _signal(&signal)
        , _shader(Content::LoadAsyncInternal<Shader>(TEXT("Shaders/SDF")))
        , _inputModel(inputModel)
        , _modelData(modelData)
        , _lodIndex(lodIndex)
        , _resolution(resolution)
        , _sdf(sdf)
        , _sdfSrc(GPUBuffer::New())
        , _sdfDst(GPUBuffer::New())
        , _sdfResult(sdfResult)
        , _xyzToLocalMul(xyzToLocalMul)
        , _xyzToLocalAdd(xyzToLocalAdd)
    {
#if GPU_ENABLE_RESOURCE_NAMING
        _sdfSrc->SetName(TEXT("SDFSrc"));
        _sdfDst->SetName(TEXT("SDFDst"));
#endif
    }

    ~GPUModelSDFTask()
    {
        SAFE_DELETE_GPU_RESOURCE(_sdfSrc);
        SAFE_DELETE_GPU_RESOURCE(_sdfDst);
    }

    Result run(GPUTasksContext* tasksContext) override
    {
        PROFILE_GPU_CPU("GPUModelSDFTask");
        GPUContext* context = tasksContext->GPU;

        // Allocate resources
        if (_shader == nullptr || _shader->WaitForLoaded())
            return Result::Failed;
        GPUShader* shader = _shader->GetShader();
        const uint32 resolutionSize = _resolution.X * _resolution.Y * _resolution.Z;
        auto desc = GPUBufferDescription::Typed(resolutionSize, PixelFormat::R32_UInt, true);
        // TODO: use transient texture (single frame)
        if (_sdfSrc->Init(desc) || _sdfDst->Init(desc))
            return Result::Failed;
        auto cb = shader->GetCB(0);
        Data data;
        data.Resolution = _resolution;
        data.ResolutionSize = resolutionSize;
        data.MaxDistance = _sdf->MaxDistance;
        data.WorldUnitsPerVoxel = _sdf->WorldUnitsPerVoxel;
        data.VoxelToPosMul = _xyzToLocalMul;
        data.VoxelToPosAdd = _xyzToLocalAdd;

        // Dispatch in 1D and fallback to 2D when using large resolution
        Int3 threadGroups(Math::CeilToInt((float)resolutionSize / ThreadGroupSize), 1, 1);
        if (threadGroups.X > GPU_MAX_CS_DISPATCH_THREAD_GROUPS)
        {
            const uint32 groups = threadGroups.X;
            threadGroups.X = Math::CeilToInt(Math::Sqrt((float)groups));
            threadGroups.Y = Math::CeilToInt((float)groups / threadGroups.X);
        }
        data.ThreadGroupsX = threadGroups.X;

        // Init SDF volume
        context->BindCB(0, cb);
        context->UpdateCB(cb, &data);
        context->BindUA(0, _sdfSrc->View());
        context->Dispatch(shader->GetCS("CS_Init"), threadGroups.X, threadGroups.Y, threadGroups.Z);

        // Rendering input triangles into the SDF volume
        if (_inputModel)
        {
            PROFILE_GPU_CPU_NAMED("Rasterize");
            const ModelLOD& lod = _inputModel->LODs[Math::Clamp(_lodIndex, _inputModel->HighestResidentLODIndex(), _inputModel->LODs.Count() - 1)];
            GPUBuffer *vbTemp = nullptr, *ibTemp = nullptr;
            for (int32 i = 0; i < lod.Meshes.Count(); i++)
            {
                const Mesh& mesh = lod.Meshes[i];
                const MaterialSlot& materialSlot = _inputModel->MaterialSlots[mesh.GetMaterialSlotIndex()];
                if (materialSlot.Material && !materialSlot.Material->WaitForLoaded())
                {
                    // Skip transparent materials
                    if (materialSlot.Material->GetInfo().BlendMode != MaterialBlendMode::Opaque)
                        continue;
                }

                GPUBuffer* vb = mesh.GetVertexBuffer(0);
                GPUBuffer* ib = mesh.GetIndexBuffer();
                data.Index16bit = mesh.Use16BitIndexBuffer() ? 1 : 0;
                data.VertexStride = vb->GetStride();
                data.TriangleCount = mesh.GetTriangleCount();
                const uint32 groups = Math::CeilToInt((float)data.TriangleCount / ThreadGroupSize);
                if (groups > GPU_MAX_CS_DISPATCH_THREAD_GROUPS)
                {
                    // TODO: support larger meshes via 2D dispatch
                    LOG(Error, "Not supported mesh with {} triangles.", data.TriangleCount);
                    continue;
                }
                context->UpdateCB(cb, &data);
                if (!EnumHasAllFlags(vb->GetDescription().Flags, GPUBufferFlags::RawBuffer | GPUBufferFlags::ShaderResource))
                {
                    desc = GPUBufferDescription::Raw(vb->GetSize(), GPUBufferFlags::ShaderResource);
                    // TODO: use transient buffer (single frame)
                    if (!vbTemp)
                    {
                        vbTemp = GPUBuffer::New();
#if GPU_ENABLE_RESOURCE_NAMING
                        vbTemp->SetName(TEXT("SDFvb"));
#endif
                    }
                    vbTemp->Init(desc);
                    context->CopyBuffer(vbTemp, vb, desc.Size);
                    vb = vbTemp;
                }
                if (!EnumHasAllFlags(ib->GetDescription().Flags, GPUBufferFlags::RawBuffer | GPUBufferFlags::ShaderResource))
                {
                    desc = GPUBufferDescription::Raw(ib->GetSize(), GPUBufferFlags::ShaderResource);
                    // TODO: use transient buffer (single frame)
                    if (!ibTemp)
                    {
                        ibTemp = GPUBuffer::New();
#if GPU_ENABLE_RESOURCE_NAMING
                        ibTemp->SetName(TEXT("SDFib"));
#endif
                    }
                    ibTemp->Init(desc);
                    context->CopyBuffer(ibTemp, ib, desc.Size);
                    ib = ibTemp;
                }
                context->BindSR(0, vb->View());
                context->BindSR(1, ib->View());
                context->Dispatch(shader->GetCS("CS_RasterizeTriangle"), groups, 1, 1);
            }
            SAFE_DELETE_GPU_RESOURCE(vbTemp);
            SAFE_DELETE_GPU_RESOURCE(ibTemp);
        }
        else if (_modelData)
        {
            PROFILE_GPU_CPU_NAMED("Rasterize");
            const ModelLodData& lod = _modelData->LODs[Math::Clamp(_lodIndex, 0, _modelData->LODs.Count() - 1)];
            auto vb = GPUBuffer::New();
            auto ib = GPUBuffer::New();
#if GPU_ENABLE_RESOURCE_NAMING
            vb->SetName(TEXT("SDFvb"));
            ib->SetName(TEXT("SDFib"));
#endif
            for (int32 i = 0; i < lod.Meshes.Count(); i++)
            {
                const MeshData* mesh = lod.Meshes[i];
                const MaterialSlotEntry& materialSlot = _modelData->Materials[mesh->MaterialSlotIndex];
                auto material = Content::LoadAsync<MaterialBase>(materialSlot.AssetID);
                if (material && !material->WaitForLoaded())
                {
                    // Skip transparent materials
                    if (material->GetInfo().BlendMode != MaterialBlendMode::Opaque)
                        continue;
                }

                data.Index16bit = 0;
                data.VertexStride = sizeof(Float3);
                data.TriangleCount = mesh->Indices.Count() / 3;
                const uint32 groups = Math::CeilToInt((float)data.TriangleCount / ThreadGroupSize);
                if (groups > GPU_MAX_CS_DISPATCH_THREAD_GROUPS)
                {
                    // TODO: support larger meshes via 2D dispatch
                    LOG(Error, "Not supported mesh with {} triangles.", data.TriangleCount);
                    continue;
                }
                context->UpdateCB(cb, &data);
                desc = GPUBufferDescription::Raw(mesh->Positions.Count() * sizeof(Float3), GPUBufferFlags::ShaderResource);
                desc.InitData = mesh->Positions.Get();
                // TODO: use transient buffer (single frame)
                vb->Init(desc);
                desc = GPUBufferDescription::Raw(mesh->Indices.Count() * sizeof(uint32), GPUBufferFlags::ShaderResource);
                desc.InitData = mesh->Indices.Get();
                // TODO: use transient buffer (single frame)
                ib->Init(desc);
                context->BindSR(0, vb->View());
                context->BindSR(1, ib->View());
                context->Dispatch(shader->GetCS("CS_RasterizeTriangle"), groups, 1, 1);
            }
            SAFE_DELETE_GPU_RESOURCE(vb);
            SAFE_DELETE_GPU_RESOURCE(ib);
        }
        
        // Convert SDF volume data back to floats
        context->Dispatch(shader->GetCS("CS_Resolve"), threadGroups.X, threadGroups.Y, threadGroups.Z);

        // Run linear flood-fill loop to populate all voxels with valid distances (spreads the initial values from triangles rasterization)
        {
            PROFILE_GPU_CPU_NAMED("FloodFill");
            auto csFloodFill = shader->GetCS("CS_FloodFill");
            const int32 floodFillIterations = Math::Max(_resolution.MaxValue() / 2 + 1, 8);
            for (int32 floodFill = 0; floodFill < floodFillIterations; floodFill++)
            {
                context->ResetUA();
                context->BindUA(0, _sdfDst->View());
                context->BindSR(0, _sdfSrc->View());
                context->Dispatch(csFloodFill, threadGroups.X, threadGroups.Y, threadGroups.Z);
                Swap(_sdfSrc, _sdfDst);
            }
        }

        // Encode SDF values into output storage
        context->ResetUA();
        context->BindSR(0, _sdfSrc->View());
        // TODO: update GPU SDF texture within this task to skip additional CPU->GPU copy
        auto sdfTextureDesc = GPUTextureDescription::New3D(_resolution.X, _resolution.Y, _resolution.Z, PixelFormat::R16_UNorm, GPUTextureFlags::UnorderedAccess | GPUTextureFlags::RenderTarget);
        // TODO: use transient texture (single frame)
        auto sdfTexture = GPUTexture::New();
#if GPU_ENABLE_RESOURCE_NAMING
        sdfTexture->SetName(TEXT("SDFTexture"));
#endif
        sdfTexture->Init(sdfTextureDesc);
        context->BindUA(1, sdfTexture->ViewVolume());
        context->Dispatch(shader->GetCS("CS_Encode"), threadGroups.X, threadGroups.Y, threadGroups.Z);

        // Copy result data into readback buffer
        if (_sdfResult)
        {
            sdfTextureDesc = sdfTextureDesc.ToStagingReadback();
            _sdfResult->Init(sdfTextureDesc);
            context->CopyTexture(_sdfResult, 0, 0, 0, 0, sdfTexture, 0);
        }

        SAFE_DELETE_GPU_RESOURCE(sdfTexture);

        return Result::Ok;
    }

    void OnSync() override
    {
        GPUTask::OnSync();
        _signal->NotifyOne();
    }

    void OnFail() override
    {
        GPUTask::OnFail();
        _signal->NotifyOne();
    }

    void OnCancel() override
    {
        GPUTask::OnCancel();
        _signal->NotifyOne();
    }
};

bool ModelTool::GenerateModelSDF(Model* inputModel, ModelData* modelData, float resolutionScale, int32 lodIndex, ModelBase::SDFData* outputSDF, MemoryWriteStream* outputStream, const StringView& assetName, float backfacesThreshold, bool useGPU)
{
    PROFILE_CPU();
    auto startTime = Platform::GetTimeSeconds();

    // Setup SDF texture properties
    BoundingBox bounds;
    if (inputModel)
        bounds = inputModel->LODs[lodIndex].GetBox();
    else if (modelData)
        bounds = modelData->LODs[lodIndex].GetBox();
    else
        return true;
    ModelBase::SDFData sdf;
    sdf.WorldUnitsPerVoxel = METERS_TO_UNITS(0.1f) / Math::Max(resolutionScale, 0.0001f); // 1 voxel per 10 centimeters
    const float boundsMargin = sdf.WorldUnitsPerVoxel * 0.5f; // Add half-texel margin around the mesh
    bounds.Minimum -= boundsMargin;
    bounds.Maximum += boundsMargin;
    const Float3 size = bounds.GetSize();
    Int3 resolution(Float3::Ceil(Float3::Clamp(size / sdf.WorldUnitsPerVoxel, 4, 256)));
    Float3 uvwToLocalMul = size;
    Float3 uvwToLocalAdd = bounds.Minimum;
    sdf.LocalToUVWMul = Float3::One / uvwToLocalMul;
    sdf.LocalToUVWAdd = -uvwToLocalAdd / uvwToLocalMul;
    sdf.MaxDistance = size.MaxValue();
    sdf.LocalBoundsMin = bounds.Minimum;
    sdf.LocalBoundsMax = bounds.Maximum;
    sdf.ResolutionScale = resolutionScale;
    sdf.LOD = lodIndex;
    const int32 maxMips = 3;
    const int32 mipCount = Math::Min(MipLevelsCount(resolution.X, resolution.Y, resolution.Z), maxMips);
    PixelFormat format = PixelFormat::R16_UNorm;
    int32 formatStride = 2;
    float formatMaxValue = MAX_uint16;
    typedef float (*FormatRead)(void* ptr);
    typedef void (*FormatWrite)(void* ptr, float v);
    FormatRead formatRead = [](void* ptr)
    {
        return (float)*(uint16*)ptr;
    };
    FormatWrite formatWrite = [](void* ptr, float v)
    {
        *(uint16*)ptr = (uint16)v;
    };
    if (resolution.MaxValue() < 8)
    {
        // For smaller meshes use more optimized format (gives small perf and memory gain but introduces artifacts on larger meshes)
        format = PixelFormat::R8_UNorm;
        formatStride = 1;
        formatMaxValue = MAX_uint8;
        formatRead = [](void* ptr)
        {
            return (float)*(uint8*)ptr;
        };
        formatWrite = [](void* ptr, float v)
        {
            *(uint8*)ptr = (uint8)v;
        };
    }
    auto textureDesc = GPUTextureDescription::New3D(resolution.X, resolution.Y, resolution.Z, format, GPUTextureFlags::ShaderResource, mipCount);
    if (outputSDF)
    {
        *outputSDF = sdf;
        if (!outputSDF->Texture)
            outputSDF->Texture = GPUTexture::New();
        if (outputSDF->Texture->Init(textureDesc))
        {
            SAFE_DELETE_GPU_RESOURCE(outputSDF->Texture);
            return true;
        }
#if GPU_ENABLE_RESOURCE_NAMING
        outputSDF->Texture->SetName(TEXT("ModelSDF"));
#endif
    }

    // Allocate memory for the distant field
    const int32 voxelsSize = resolution.X * resolution.Y * resolution.Z * formatStride;
    BytesContainer voxels;
    voxels.Allocate(voxelsSize);
    Float3 xyzToLocalMul = uvwToLocalMul / Float3(resolution - 1);
    Float3 xyzToLocalAdd = uvwToLocalAdd;
    const Float2 encodeMAD(0.5f / sdf.MaxDistance * formatMaxValue, 0.5f * formatMaxValue);
    const Float2 decodeMAD(2.0f * sdf.MaxDistance / formatMaxValue, -sdf.MaxDistance);
    int32 voxelSizeSum = voxelsSize;

    // TODO: use optimized sparse storage for SDF data as hierarchical bricks as in papers below:
    // https://gpuopen.com/gdc-presentations/2023/GDC-2023-Sparse-Distance-Fields-For-Games.pdf + https://www.youtube.com/watch?v=iY15xhuuHPQ&ab_channel=AMD
    // https://graphics.pixar.com/library/IrradianceAtlas/paper.pdf
    // http://maverick.inria.fr/Membres/Cyril.Crassin/thesis/CCrassinThesis_EN_Web.pdf
    // http://ramakarl.com/pdfs/2016_Hoetzlein_GVDB.pdf
    // https://www.cse.chalmers.se/~uffe/HighResolutionSparseVoxelDAGs.pdf

    // Check if run SDF generation on a GPU via Compute Shader or on a Job System
    useGPU &= GPUDevice::Instance
            && GPUDevice::Instance->GetState() == GPUDevice::DeviceState::Ready
            && GPUDevice::Instance->Limits.HasCompute
            && format == PixelFormat::R16_UNorm
            && !IsInMainThread() // TODO: support GPU to generate model SDF on-the-fly directly into virtual model (if called during rendering)
            && resolution.MaxValue() > 8;
    if (useGPU)
    {
        PROFILE_CPU_NAMED("GPU");

        // TODO: skip using sdfResult and downloading SDF from GPU when updating virtual model
        auto sdfResult = GPUTexture::New();
#if GPU_ENABLE_RESOURCE_NAMING
        sdfResult->SetName(TEXT("SDFResult"));
#endif

        // Run SDF generation via GPU async task
        ConditionVariable signal;
        CriticalSection mutex;
        Task* task = New<GPUModelSDFTask>(signal, inputModel, modelData, lodIndex, resolution, &sdf, sdfResult, xyzToLocalMul, xyzToLocalAdd);
        task->Start();
        mutex.Lock();
        signal.Wait(mutex);
        mutex.Unlock();
        bool failed = task->IsFailed();

        // Gather result data from GPU to CPU
        if (!failed && sdfResult)
        {
            TextureMipData mipData;
            const uint32 rowPitch = resolution.X * formatStride;
            failed = sdfResult->GetData(0, 0, mipData, rowPitch);
            failed |= voxels.Length() != mipData.Data.Length();
            if (!failed)
                voxels = mipData.Data;
        }

        SAFE_DELETE_GPU_RESOURCE(sdfResult);
        if (failed)
            return true;
    }
    else
    {
        // Setup acceleration structure for fast ray tracing the mesh triangles
        MeshAccelerationStructure scene;
        if (inputModel)
            scene.Add(inputModel, lodIndex);
        else if (modelData)
            scene.Add(modelData, lodIndex);
        scene.BuildBVH();

        // Brute-force for each voxel to calculate distance to the closest triangle with point query and distance sign by raycasting around the voxel
        constexpr int32 sampleCount = 12;
        Float3 sampleDirections[sampleCount];
        {
            RandomStream rand;
            sampleDirections[0] = Float3::Up;
            sampleDirections[1] = Float3::Down;
            sampleDirections[2] = Float3::Left;
            sampleDirections[3] = Float3::Right;
            sampleDirections[4] = Float3::Forward;
            sampleDirections[5] = Float3::Backward;
            for (int32 i = 6; i < sampleCount; i++)
                sampleDirections[i] = rand.GetUnitVector();
        }
        Function<void(int32)> sdfJob = [&sdf, &resolution, &backfacesThreshold, sampleDirections, &sampleCount, &scene, &voxels, &xyzToLocalMul, &xyzToLocalAdd, &encodeMAD, &formatStride, &formatWrite](int32 z)
        {
            PROFILE_CPU_NAMED("Model SDF Job");
            Real hitDistance;
            Vector3 hitNormal, hitPoint;
            Triangle hitTriangle;
            const int32 zAddress = resolution.Y * resolution.X * z;
            for (int32 y = 0; y < resolution.Y; y++)
            {
                const int32 yAddress = resolution.X * y + zAddress;
                for (int32 x = 0; x < resolution.X; x++)
                {
                    Real minDistance = sdf.MaxDistance;
                    Vector3 voxelPos = Float3((float)x, (float)y, (float)z) * xyzToLocalMul + xyzToLocalAdd;

                    // Point query to find the distance to the closest surface
                    scene.PointQuery(voxelPos, minDistance, hitPoint, hitTriangle);

                    // Raycast samples around voxel to count triangle backfaces hit
                    int32 hitBackCount = 0, hitCount = 0;
                    for (int32 sample = 0; sample < sampleCount; sample++)
                    {
                        Ray sampleRay(voxelPos, sampleDirections[sample]);
                        sampleRay.Position -= sampleRay.Direction * 0.0001f; // Apply small margin
                        if (scene.RayCast(sampleRay, hitDistance, hitNormal, hitTriangle))
                        {
                            if (hitDistance < minDistance)
                                minDistance = hitDistance;
                            hitCount++;
                            const bool backHit = Float3::Dot(sampleRay.Direction, hitTriangle.GetNormal()) > 0;
                            if (backHit)
                                hitBackCount++;
                        }
                    }

                    float distance = (float)minDistance;
                    // TODO: surface thickness threshold? shift reduce distance for all voxels by something like 0.01 to enlarge thin geometry
                    // if ((float)hitBackCount > (float)hitCount * 0.3f && hitCount != 0)
                    if ((float)hitBackCount > (float)sampleCount * backfacesThreshold && hitCount != 0)
                    {
                        // Voxel is inside the geometry so turn it into negative distance to the surface
                        distance *= -1;
                    }
                    const int32 xAddress = x + yAddress;
                    formatWrite(voxels.Get() + xAddress * formatStride, distance * encodeMAD.X + encodeMAD.Y);
                }
            }
        };
        JobSystem::Execute(sdfJob, resolution.Z);
    }

    // Cache SDF data on a CPU
    if (outputStream)
    {
        outputStream->WriteInt32(1); // Version
        ModelSDFHeader data(sdf, textureDesc);
        outputStream->WriteBytes(&data, sizeof(data));
        ModelSDFMip mipData(0, resolution.X * formatStride, voxelsSize);
        outputStream->WriteBytes(&mipData, sizeof(mipData));
        outputStream->WriteBytes(voxels.Get(), voxelsSize);
    }

    // Upload data to the GPU
    if (outputSDF)
    {
        auto task = outputSDF->Texture->UploadMipMapAsync(voxels, 0, resolution.X * formatStride, voxelsSize, true);
        if (task)
            task->Start();
    }

    // Generate mip maps
    void* voxelsMipSrc = voxels.Get();
    void* voxelsMip = nullptr;
    for (int32 mipLevel = 1; mipLevel < mipCount; mipLevel++)
    {
        Int3 resolutionMip = Int3::Max(resolution / 2, Int3::One);
        const int32 voxelsMipSize = resolutionMip.X * resolutionMip.Y * resolutionMip.Z * formatStride;
        if (voxelsMip == nullptr)
            voxelsMip = Allocator::Allocate(voxelsMipSize);

        // Downscale mip
        Function<void(int32)> mipJob = [&voxelsMip, &voxelsMipSrc, &resolution, &resolutionMip, &encodeMAD, &decodeMAD, &formatStride, &formatRead, &formatWrite](int32 z)
        {
            PROFILE_CPU_NAMED("Model SDF Mip Job");
            const int32 zAddress = resolutionMip.Y * resolutionMip.X * z;
            for (int32 y = 0; y < resolutionMip.Y; y++)
            {
                const int32 yAddress = resolutionMip.X * y + zAddress;
                for (int32 x = 0; x < resolutionMip.X; x++)
                {
                    // Min-filter around the voxel
                    float distance = MAX_float;
                    for (int32 dz = 0; dz < 2; dz++)
                    {
                        const int32 dzAddress = (z * 2 + dz) * (resolution.Y * resolution.X);
                        for (int32 dy = 0; dy < 2; dy++)
                        {
                            const int32 dyAddress = (y * 2 + dy) * (resolution.X) + dzAddress;
                            for (int32 dx = 0; dx < 2; dx++)
                            {
                                const int32 dxAddress = (x * 2 + dx) + dyAddress;
                                const float d = formatRead((byte*)voxelsMipSrc + dxAddress * formatStride) * decodeMAD.X + decodeMAD.Y;
                                distance = Math::Min(distance, d);
                            }
                        }
                    }

                    const int32 xAddress = x + yAddress;
                    formatWrite((byte*)voxelsMip + xAddress * formatStride, distance * encodeMAD.X + encodeMAD.Y);
                }
            }
        };
        JobSystem::Execute(mipJob, resolutionMip.Z);

        // Cache SDF data on a CPU
        if (outputStream)
        {
            ModelSDFMip mipData(mipLevel, resolutionMip.X * formatStride, voxelsMipSize);
            outputStream->WriteBytes(&mipData, sizeof(mipData));
            outputStream->WriteBytes(voxelsMip, voxelsMipSize);
        }

        // Upload to the GPU
        if (outputSDF)
        {
            BytesContainer data;
            data.Link((byte*)voxelsMip, voxelsMipSize);
            auto task = outputSDF->Texture->UploadMipMapAsync(data, mipLevel, resolutionMip.X * formatStride, voxelsMipSize, true);
            if (task)
                task->Start();
        }

        // Go down
        voxelSizeSum += voxelsSize;
        Swap(voxelsMip, voxelsMipSrc);
        resolution = resolutionMip;
    }

    Allocator::Free(voxelsMip);

#if !BUILD_RELEASE
    auto endTime = Platform::GetTimeSeconds();
    LOG(Info, "Generated SDF {}x{}x{} ({} kB) in {}ms for {}", resolution.X, resolution.Y, resolution.Z, voxelSizeSum / 1024, (int32)((endTime - startTime) * 1000.0), assetName);
#endif
    return false;
}

#if USE_EDITOR

void ModelTool::Options::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(ModelTool::Options);

    SERIALIZE(Type);
    SERIALIZE(CalculateNormals);
    SERIALIZE(SmoothingNormalsAngle);
    SERIALIZE(FlipNormals);
    SERIALIZE(CalculateTangents);
    SERIALIZE(SmoothingTangentsAngle);
    SERIALIZE(ReverseWindingOrder);
    SERIALIZE(OptimizeMeshes);
    SERIALIZE(MergeMeshes);
    SERIALIZE(ImportLODs);
    SERIALIZE(ImportVertexColors);
    SERIALIZE(ImportBlendShapes);
    SERIALIZE(CalculateBoneOffsetMatrices);
    SERIALIZE(LightmapUVsSource);
    SERIALIZE(CollisionMeshesPrefix);
    SERIALIZE(Scale);
    SERIALIZE(Rotation);
    SERIALIZE(Translation);
    SERIALIZE(UseLocalOrigin);
    SERIALIZE(CenterGeometry);
    SERIALIZE(Duration);
    SERIALIZE(FramesRange);
    SERIALIZE(DefaultFrameRate);
    SERIALIZE(SamplingRate);
    SERIALIZE(SkipEmptyCurves);
    SERIALIZE(OptimizeKeyframes);
    SERIALIZE(ImportScaleTracks);
    SERIALIZE(RootMotion);
    SERIALIZE(RootMotionFlags);
    SERIALIZE(RootNodeName);
    SERIALIZE(GenerateLODs);
    SERIALIZE(BaseLOD);
    SERIALIZE(LODCount);
    SERIALIZE(TriangleReduction);
    SERIALIZE(SloppyOptimization);
    SERIALIZE(LODTargetError);
    SERIALIZE(ImportMaterials);
    SERIALIZE(ImportMaterialsAsInstances);
    SERIALIZE(InstanceToImportAs);
    SERIALIZE(ImportTextures);
    SERIALIZE(RestoreMaterialsOnReimport);
    SERIALIZE(SkipExistingMaterialsOnReimport);
    SERIALIZE(GenerateSDF);
    SERIALIZE(SDFResolution);
    SERIALIZE(SplitObjects);
    SERIALIZE(ObjectIndex);
    SERIALIZE(SubAssetFolder);
}

void ModelTool::Options::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(Type);
    DESERIALIZE(CalculateNormals);
    DESERIALIZE(SmoothingNormalsAngle);
    DESERIALIZE(FlipNormals);
    DESERIALIZE(CalculateTangents);
    DESERIALIZE(SmoothingTangentsAngle);
    DESERIALIZE(ReverseWindingOrder);
    DESERIALIZE(OptimizeMeshes);
    DESERIALIZE(MergeMeshes);
    DESERIALIZE(ImportLODs);
    DESERIALIZE(ImportVertexColors);
    DESERIALIZE(ImportBlendShapes);
    DESERIALIZE(CalculateBoneOffsetMatrices);
    DESERIALIZE(LightmapUVsSource);
    DESERIALIZE(CollisionMeshesPrefix);
    DESERIALIZE(Scale);
    DESERIALIZE(Rotation);
    DESERIALIZE(Translation);
    DESERIALIZE(UseLocalOrigin);
    DESERIALIZE(CenterGeometry);
    DESERIALIZE(Duration);
    DESERIALIZE(FramesRange);
    DESERIALIZE(DefaultFrameRate);
    DESERIALIZE(SamplingRate);
    DESERIALIZE(SkipEmptyCurves);
    DESERIALIZE(OptimizeKeyframes);
    DESERIALIZE(ImportScaleTracks);
    DESERIALIZE(RootMotion);
    DESERIALIZE(RootMotionFlags);
    DESERIALIZE(RootNodeName);
    DESERIALIZE(GenerateLODs);
    DESERIALIZE(BaseLOD);
    DESERIALIZE(LODCount);
    DESERIALIZE(TriangleReduction);
    DESERIALIZE(SloppyOptimization);
    DESERIALIZE(LODTargetError);
    DESERIALIZE(ImportMaterials);
    DESERIALIZE(ImportMaterialsAsInstances);
    DESERIALIZE(InstanceToImportAs);
    DESERIALIZE(ImportTextures);
    DESERIALIZE(RestoreMaterialsOnReimport);
    DESERIALIZE(SkipExistingMaterialsOnReimport);
    DESERIALIZE(GenerateSDF);
    DESERIALIZE(SDFResolution);
    DESERIALIZE(SplitObjects);
    DESERIALIZE(ObjectIndex);
    DESERIALIZE(SubAssetFolder);

    // [Deprecated on 23.11.2021, expires on 21.11.2023]
    int32 AnimationIndex = -1;
    DESERIALIZE(AnimationIndex);
    if (AnimationIndex != -1)
        ObjectIndex = AnimationIndex;

    // [Deprecated on 08.02.2024, expires on 08.02.2026]
    bool EnableRootMotion = false;
    DESERIALIZE(EnableRootMotion);
    if (EnableRootMotion)
    {
        RootMotion = RootMotionMode::ExtractNode;
        RootMotionFlags = AnimationRootMotionFlags::RootPositionXZ;
    }
}

void RemoveNamespace(String& name)
{
    const int32 namespaceStart = name.Find(':');
    if (namespaceStart != -1)
        name = name.Substring(namespaceStart + 1);
}

bool ModelTool::ImportData(const String& path, ModelData& data, Options& options, String& errorMsg)
{
    PROFILE_CPU();

    // Validate options
    options.Scale = Math::Clamp(options.Scale, 0.0001f, 100000.0f);
    options.SmoothingNormalsAngle = Math::Clamp(options.SmoothingNormalsAngle, 0.0f, 175.0f);
    options.SmoothingTangentsAngle = Math::Clamp(options.SmoothingTangentsAngle, 0.0f, 45.0f);
    options.FramesRange.Y = Math::Max(options.FramesRange.Y, options.FramesRange.X);
    options.DefaultFrameRate = Math::Max(0.0f, options.DefaultFrameRate);
    options.SamplingRate = Math::Max(0.0f, options.SamplingRate);
    if (options.SplitObjects || options.Type == ModelType::Prefab)
        options.MergeMeshes = false; // Meshes merging doesn't make sense when we want to import each mesh individually
    // TODO: maybe we could update meshes merger to collapse meshes within the same name if splitting is enabled?

    // Call importing backend
#if (USE_AUTODESK_FBX_SDK || USE_OPEN_FBX) && USE_ASSIMP
    if (path.EndsWith(TEXT(".fbx"), StringSearchCase::IgnoreCase))
    {
#if USE_AUTODESK_FBX_SDK
        if (ImportDataAutodeskFbxSdk(path, data, options, errorMsg))
            return true;
#elif USE_OPEN_FBX
        if (ImportDataOpenFBX(path, data, options, errorMsg))
            return true;
#endif
    }
    else
    {
        if (ImportDataAssimp(path, data, options, errorMsg))
            return true;
    }
#elif USE_ASSIMP
    if (ImportDataAssimp(path, data, options, errorMsg))
        return true;
#elif USE_AUTODESK_FBX_SDK
    if (ImportDataAutodeskFbxSdk(path, data, options, errorMsg))
        return true;
#elif USE_OPEN_FBX
    if (ImportDataOpenFBX(path, data, options, errorMsg))
        return true;
#else
    LOG(Error, "Compiled without model importing backend.");
    return true;
#endif

    // Remove namespace prefixes from the nodes names
    {
        for (auto& node : data.Nodes)
        {
            RemoveNamespace(node.Name);
        }
        for (auto& node : data.Skeleton.Nodes)
        {
            RemoveNamespace(node.Name);
        }
        for (auto& animation : data.Animations)
        {
            for (auto& channel : animation.Channels)
                RemoveNamespace(channel.NodeName);
        }
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                RemoveNamespace(mesh->Name);
                for (auto& blendShape : mesh->BlendShapes)
                    RemoveNamespace(blendShape.Name);
            }
        }
    }

    // Validate the animation channels
    for (auto& animation : data.Animations)
    {
        auto& channels = animation.Channels;
        if (channels.IsEmpty())
            continue;

        // Validate bone animations uniqueness
        for (int32 i = 0; i < channels.Count(); i++)
        {
            for (int32 j = i + 1; j < channels.Count(); j++)
            {
                if (channels[i].NodeName == channels[j].NodeName)
                {
                    LOG(Warning, "Animation uses two nodes with the same name ({0}). Removing duplicated channel.", channels[i].NodeName);
                    channels.RemoveAtKeepOrder(j);
                    j--;
                }
            }
        }

        // Remove channels/animations with empty tracks
        if (options.SkipEmptyCurves)
        {
            for (int32 i = 0; i < channels.Count(); i++)
            {
                auto& channel = channels[i];

                // Remove identity curves (with single keyframe and no actual animated change)
                if (channel.Position.GetKeyframes().Count() == 1 && channel.Position.GetKeyframes()[0].Value.IsZero())
                {
                    channel.Position.Clear();
                }
                if (channel.Rotation.GetKeyframes().Count() == 1 && channel.Rotation.GetKeyframes()[0].Value.IsIdentity())
                {
                    channel.Rotation.Clear();
                }
                if (channel.Scale.GetKeyframes().Count() == 1 && channel.Scale.GetKeyframes()[0].Value.IsOne())
                {
                    channel.Scale.Clear();
                }

                // Remove whole channel if has no effective data
                if (channel.Position.IsEmpty() && channel.Rotation.IsEmpty() && channel.Scale.IsEmpty())
                {
                    LOG(Warning, "Removing empty animation channel ({0}).", channel.NodeName);
                    channels.RemoveAtKeepOrder(i);
                }
            }
        }
    }

    // Flip normals of the imported geometry
    if (options.FlipNormals && EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Geometry))
    {
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (auto& n : mesh->Normals)
                    n *= -1;
                for (auto& shape : mesh->BlendShapes)
                    for (auto& v : shape.Vertices)
                        v.NormalDelta *= -1;
            }
        }
    }

    return false;
}

// Disabled by default (not finished and Assimp importer outputs nodes in a fine order)
#define USE_SKELETON_NODES_SORTING 0

#if USE_SKELETON_NODES_SORTING

bool SortDepths(const Pair<int32, int32>& a, const Pair<int32, int32>& b)
{
    return a.First < b.First;
}

void CreateLinearListFromTree(Array<SkeletonNode>& nodes, Array<int32>& mapping)
{
    // Customized breadth first tree algorithm (each node has no direct reference to the children so we build the cache for the nodes depth level)
    const int32 count = nodes.Count();
    Array<Pair<int32, int32>> depths(count); // Pair.First = Depth, Pair.Second = Node Index
    depths.Resize(count);
    depths.SetAll(-1);
    for (int32 i = 0; i < count; i++)
    {
        // Skip evaluated nodes
        if (depths[i].First != -1)
            continue;

        // Find the first node with calculated depth and get the distance to it
        int32 end = i;
        int32 lastDepth;
        int32 relativeDepth = 0;
        do
        {
            lastDepth = depths[end].First;
            end = nodes[end].ParentIndex;
            relativeDepth++;
        } while (end != -1 && lastDepth == -1);

        // Set the depth (second item is the node index)
        depths[i] = ToPair(lastDepth + relativeDepth, i);
    }
    for (int32 i = 0; i < count; i++)
    {
        // Strange divide by 2 but works
        depths[i].First = depths[i].First >> 1;
    }

    // Order nodes by depth O(n*log(n))
    depths.Sort(SortDepths);

    // Extract nodes mapping O(n^2)
    mapping.EnsureCapacity(count, false);
    mapping.Resize(count);
    for (int32 i = 0; i < count; i++)
    {
        int32 newIndex = -1;
        for (int32 j = 0; j < count; j++)
        {
            if (depths[j].Second == i)
            {
                newIndex = j;
                break;
            }
        }
        ASSERT(newIndex != -1);
        mapping[i] = newIndex;
    }
}

#endif

template<typename T>
void OptimizeCurve(LinearCurve<T>& curve)
{
    auto& oldKeyframes = curve.GetKeyframes();
    const int32 keyCount = oldKeyframes.Count();
    typename LinearCurve<T>::KeyFrameCollection newKeyframes(keyCount);
    bool lastWasEqual = false;

    for (int32 i = 0; i < keyCount; i++)
    {
        bool isEqual = false;
        const auto& curKey = oldKeyframes[i];
        if (i > 0)
        {
            const auto& prevKey = newKeyframes.Last();
            isEqual = Math::NearEqual(prevKey.Value, curKey.Value);
        }

        // More than two keys in a row are equal, remove the middle key by replacing it with this one
        if (lastWasEqual && isEqual)
        {
            auto& prevKey = newKeyframes.Last();
            prevKey = curKey;
            continue;
        }

        newKeyframes.Add(curKey);
        lastWasEqual = isEqual;
    }

    // Special case if animation has only two the same keyframes after cleaning
    if (newKeyframes.Count() == 2 && Math::NearEqual(newKeyframes[0].Value, newKeyframes[1].Value))
    {
        newKeyframes.RemoveAt(1);
    }

    // Special case if animation has only one identity keyframe (does not introduce any animation)
    if (newKeyframes.Count() == 1 && Math::NearEqual(newKeyframes[0].Value, curve.GetDefaultValue()))
    {
        newKeyframes.RemoveAt(0);
    }

    // Update keyframes if size changed
    if (keyCount != newKeyframes.Count())
    {
        curve.SetKeyframes(newKeyframes);
    }
}

void* MeshOptAllocate(size_t size)
{
    return Allocator::Allocate(size);
}

void MeshOptDeallocate(void* ptr)
{
    Allocator::Free(ptr);
}

void TrySetupMaterialParameter(MaterialInstance* instance, Span<const Char*> paramNames, const Variant& value, MaterialParameterType type)
{
    for (const Char* name : paramNames)
    {
        for (MaterialParameter& param : instance->Params)
        {
            const MaterialParameterType paramType = param.GetParameterType();
            if (type != paramType)
            {
                if (type == MaterialParameterType::Color)
                {
                    if (paramType != MaterialParameterType::Vector3 ||
                        paramType != MaterialParameterType::Vector4)
                        continue;
                }
                else
                    continue;
            }
            if (StringUtils::CompareIgnoreCase(name, param.GetName().Get()) != 0)
                continue;
            param.SetValue(value);
            param.SetIsOverride(true);
            return;
        }
    }
}

String GetAdditionalImportPath(const String& autoImportOutput, Array<String>& importedFileNames, const String& name)
{
    String filename = name;
    EditorUtilities::ValidatePathChars(filename);
    if (importedFileNames.Contains(filename))
    {
        int32 counter = 1;
        do
        {
            filename = name + TEXT(" ") + StringUtils::ToString(counter);
            counter++;
        } while (importedFileNames.Contains(filename));
    }
    importedFileNames.Add(filename);
    return autoImportOutput / filename + ASSET_FILES_EXTENSION_WITH_DOT;
}

bool ModelTool::ImportModel(const String& path, ModelData& data, Options& options, String& errorMsg, const String& autoImportOutput)
{
    PROFILE_CPU();
    LOG(Info, "Importing model from \'{0}\'", path);
    const auto startTime = DateTime::NowUTC();

    // Import data
    switch (options.Type)
    {
    case ModelType::Model:
        options.ImportTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes;
        if (options.ImportMaterials)
            options.ImportTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            options.ImportTypes |= ImportDataTypes::Textures;
        break;
    case ModelType::SkinnedModel:
        options.ImportTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes | ImportDataTypes::Skeleton;
        if (options.ImportMaterials)
            options.ImportTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            options.ImportTypes |= ImportDataTypes::Textures;
        break;
    case ModelType::Animation:
        options.ImportTypes = ImportDataTypes::Animations;
        if (options.RootMotion == RootMotionMode::ExtractCenterOfMass)
            options.ImportTypes |= ImportDataTypes::Skeleton;
        break;
    case ModelType::Prefab:
        options.ImportTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes | ImportDataTypes::Animations;
        if (options.ImportMaterials)
            options.ImportTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            options.ImportTypes |= ImportDataTypes::Textures;
        break;
    default:
        return true;
    }
    if (ImportData(path, data, options, errorMsg))
        return true;

    // Validate result data
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Geometry))
    {
        LOG(Info, "Imported model has {0} LODs, {1} meshes (in LOD0) and {2} materials", data.LODs.Count(), data.LODs.Count() != 0 ? data.LODs[0].Meshes.Count() : 0, data.Materials.Count());

        // Process blend shapes
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (int32 blendShapeIndex = mesh->BlendShapes.Count() - 1; blendShapeIndex >= 0; blendShapeIndex--)
                {
                    auto& blendShape = mesh->BlendShapes[blendShapeIndex];

                    // Remove blend shape vertices with empty deltas
                    for (int32 i = blendShape.Vertices.Count() - 1; i >= 0; i--)
                    {
                        auto& v = blendShape.Vertices.Get()[i];
                        if (v.PositionDelta.IsZero() && v.NormalDelta.IsZero())
                        {
                            blendShape.Vertices.RemoveAt(i);
                        }
                    }

                    // Remove empty blend shapes
                    if (blendShape.Vertices.IsEmpty() || blendShape.Name.IsEmpty())
                    {
                        LOG(Info, "Removing empty blend shape '{0}' from mesh '{1}'", blendShape.Name, mesh->Name);
                        mesh->BlendShapes.RemoveAt(blendShapeIndex);
                    }
                }
            }
        }
    }
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Skeleton))
    {
        LOG(Info, "Imported skeleton has {0} bones and {1} nodes", data.Skeleton.Bones.Count(), data.Nodes.Count());

        // Add single node if imported skeleton is empty
        if (data.Skeleton.Nodes.IsEmpty())
        {
            data.Skeleton.Nodes.Resize(1);
            data.Skeleton.Nodes[0].Name = TEXT("Root");
            data.Skeleton.Nodes[0].LocalTransform = Transform::Identity;
            data.Skeleton.Nodes[0].ParentIndex = -1;
        }

        // Special case if imported model has no bones but has valid skeleton and meshes.
        // We assume that every mesh uses a single bone. Copy nodes to bones.
        if (data.Skeleton.Bones.IsEmpty() && Math::IsInRange(data.Skeleton.Nodes.Count(), 1, MAX_BONES_PER_MODEL))
        {
            data.Skeleton.Bones.Resize(data.Skeleton.Nodes.Count());
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                auto& node = data.Skeleton.Nodes[i];
                auto& bone = data.Skeleton.Bones[i];

                bone.ParentIndex = node.ParentIndex;
                bone.NodeIndex = i;
                bone.LocalTransform = node.LocalTransform;

                Matrix t = Matrix::Identity;
                int32 idx = bone.NodeIndex;
                do
                {
                    t *= data.Skeleton.Nodes[idx].LocalTransform.GetWorld();
                    idx = data.Skeleton.Nodes[idx].ParentIndex;
                } while (idx != -1);
                t.Invert();
                bone.OffsetMatrix = t;
            }
        }

        // Check bones limit currently supported by the engine
        if (data.Skeleton.Bones.Count() > MAX_BONES_PER_MODEL)
        {
            errorMsg = String::Format(TEXT("Imported model skeleton has too many bones. Imported: {0}, maximum supported: {1}. Please optimize your asset."), data.Skeleton.Bones.Count(), MAX_BONES_PER_MODEL);
            return true;
        }

        // Ensure that root node is at index 0
        int32 rootIndex = -1;
        for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
        {
            const auto idx = data.Skeleton.Nodes.Get()[i].ParentIndex;
            if (idx == -1 && rootIndex == -1)
            {
                // Found root
                rootIndex = i;
            }
            else if (idx == -1)
            {
                // Found multiple roots
                errorMsg = TEXT("Imported skeleton has more than one root node.");
                return true;
            }
        }
        if (rootIndex == -1)
        {
            // Missing root node (more additional validation that possible error)
            errorMsg = TEXT("Imported skeleton has missing root node.");
            return true;
        }
        if (rootIndex != 0)
        {
            // Map the root node to index 0 (more optimized for runtime)
            LOG(Warning, "Imported skeleton root node is not at index 0. Performing the remmaping.");
            const int32 prevRootIndex = rootIndex;
            rootIndex = 0;
            Swap(data.Skeleton.Nodes[rootIndex], data.Skeleton.Nodes[prevRootIndex]);
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                auto& node = data.Skeleton.Nodes.Get()[i];
                if (node.ParentIndex == prevRootIndex)
                    node.ParentIndex = rootIndex;
                else if (node.ParentIndex == rootIndex)
                    node.ParentIndex = prevRootIndex;
            }
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                auto& bone = data.Skeleton.Bones.Get()[i];
                if (bone.NodeIndex == prevRootIndex)
                    bone.NodeIndex = rootIndex;
                else if (bone.NodeIndex == rootIndex)
                    bone.NodeIndex = prevRootIndex;
            }
        }

#if BUILD_DEBUG
        // Validate that nodes and bones hierarchies are valid (no cyclic references because its mean to be a tree)
        {
            for (int32 i = 0; i < data.Skeleton.Nodes.Count(); i++)
            {
                int32 j = i;
                int32 testsLeft = data.Skeleton.Nodes.Count();
                do
                {
                    j = data.Skeleton.Nodes[j].ParentIndex;
                } while (j != -1 && testsLeft-- > 0);
                if (testsLeft <= 0)
                {
                    Platform::Fatal(TEXT("Skeleton importer issue!"));
                }
            }
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                int32 j = i;
                int32 testsLeft = data.Skeleton.Bones.Count();
                do
                {
                    j = data.Skeleton.Bones[j].ParentIndex;
                } while (j != -1 && testsLeft-- > 0);
                if (testsLeft <= 0)
                {
                    Platform::Fatal(TEXT("Skeleton importer issue!"));
                }
            }
            for (int32 i = 0; i < data.Skeleton.Bones.Count(); i++)
            {
                if (data.Skeleton.Bones[i].NodeIndex == -1)
                {
                    Platform::Fatal(TEXT("Skeleton importer issue!"));
                }
            }
        }
#endif
    }
    if (EnumHasAllFlags(options.ImportTypes, ImportDataTypes::Geometry | ImportDataTypes::Skeleton))
    {
        // Validate skeleton bones used by the meshes
        const int32 meshesCount = data.LODs.Count() != 0 ? data.LODs[0].Meshes.Count() : 0;
        for (int32 i = 0; i < meshesCount; i++)
        {
            const auto mesh = data.LODs[0].Meshes[i];
            if (mesh->BlendIndices.IsEmpty() || mesh->BlendWeights.IsEmpty())
            {
                auto indices = Int4::Zero;
                auto weights = Float4::UnitX;

                // Check if use a single bone for skinning
                auto nodeIndex = data.Skeleton.FindNode(mesh->Name);
                auto boneIndex = data.Skeleton.FindBone(nodeIndex);
                if (boneIndex == -1 && nodeIndex != -1 && data.Skeleton.Bones.Count() < MAX_BONES_PER_MODEL)
                {
                    // Add missing bone to be used by skinned model from animated nodes pose
                    boneIndex = data.Skeleton.Bones.Count();
                    auto& bone = data.Skeleton.Bones.AddOne();
                    bone.ParentIndex = -1;
                    bone.NodeIndex = nodeIndex;
                    bone.LocalTransform = CombineTransformsFromNodeIndices(data.Nodes, -1, nodeIndex);
                    CalculateBoneOffsetMatrix(data.Skeleton.Nodes, bone.OffsetMatrix, bone.NodeIndex);
                    LOG(Warning, "Using auto-created bone {0} (index {1}) for mesh \'{2}\'", data.Skeleton.Nodes[nodeIndex].Name, boneIndex, mesh->Name);
                    indices.X = boneIndex;
                }
                else if (boneIndex != -1)
                {
                    // Fallback to already added bone
                    LOG(Warning, "Using auto-detected bone {0} (index {1}) for mesh \'{2}\'", data.Skeleton.Nodes[nodeIndex].Name, boneIndex, mesh->Name);
                    indices.X = boneIndex;
                }
                else
                {
                    // No bone
                    LOG(Warning, "Imported mesh \'{0}\' has missing skinning data. It may result in invalid rendering.", mesh->Name);
                }

                mesh->BlendIndices.Resize(mesh->Positions.Count());
                mesh->BlendWeights.Resize(mesh->Positions.Count());
                mesh->BlendIndices.SetAll(indices);
                mesh->BlendWeights.SetAll(weights);
            }
            else
            {
                auto& indices = mesh->BlendIndices;
                for (int32 j = 0; j < indices.Count(); j++)
                {
                    const Int4 ij = indices.Get()[j];
                    const int32 min = ij.MinValue();
                    const int32 max = ij.MaxValue();
                    if (min < 0 || max >= data.Skeleton.Bones.Count())
                    {
                        LOG(Warning, "Imported mesh \'{0}\' has invalid blend indices. It may result in invalid rendering.", mesh->Name);
                        break;
                    }
                }
                auto& weights = mesh->BlendWeights;
                for (int32 j = 0; j < weights.Count(); j++)
                {
                    const float sum = weights.Get()[j].SumValues();
                    if (Math::Abs(sum - 1.0f) > ZeroTolerance)
                    {
                        LOG(Warning, "Imported mesh \'{0}\' has invalid blend weights. It may result in invalid rendering.", mesh->Name);
                        break;
                    }
                }
            }
        }
    }
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Animations))
    {
        int32 index = 0;
        for (auto& animation : data.Animations)
        {
            LOG(Info, "Imported animation '{}' at index {} has {} channels, duration: {} frames ({} seconds), frames per second: {}", animation.Name, index, animation.Channels.Count(), animation.Duration, animation.GetLength(), animation.FramesPerSecond);
            if (animation.Duration <= ZeroTolerance || animation.FramesPerSecond <= ZeroTolerance)
            {
                errorMsg = TEXT("Invalid animation duration.");
                return true;
            }
            index++;
        }
    }
    switch (options.Type)
    {
    case ModelType::Model:
        if (data.LODs.IsEmpty() || data.LODs[0].Meshes.IsEmpty())
        {
            errorMsg = TEXT("Imported model has no valid geometry.");
            return true;
        }
        if (data.Nodes.IsEmpty())
        {
            errorMsg = TEXT("Missing model nodes.");
            return true;
        }
        break;
    case ModelType::SkinnedModel:
        if (data.LODs.Count() > 1)
        {
            LOG(Warning, "Imported skinned model has more than one LOD. Removing the lower LODs. Only single one is supported.");
            data.LODs.Resize(1);
        }
        break;
    case ModelType::Animation:
        if (data.Animations.IsEmpty())
        {
            errorMsg = TEXT("Imported file has no valid animations.");
            return true;
        }
        break;
    }

    // Keep additionally imported files well organized
    Array<String> importedFileNames;

    // Prepare textures
    for (int32 i = 0; i < data.Textures.Count(); i++)
    {
        auto& texture = data.Textures[i];

        // Auto-import textures
        if (autoImportOutput.IsEmpty() || EnumHasNoneFlags(options.ImportTypes, ImportDataTypes::Textures) || texture.FilePath.IsEmpty())
            continue;
        String assetPath = GetAdditionalImportPath(autoImportOutput, importedFileNames, StringUtils::GetFileNameWithoutExtension(texture.FilePath));
#if COMPILE_WITH_ASSETS_IMPORTER
        TextureTool::Options textureOptions;
        switch (texture.Type)
        {
        case TextureEntry::TypeHint::ColorRGB:
            textureOptions.Type = TextureFormatType::ColorRGB;
            break;
        case TextureEntry::TypeHint::ColorRGBA:
            textureOptions.Type = TextureFormatType::ColorRGBA;
            break;
        case TextureEntry::TypeHint::Normals:
            textureOptions.Type = TextureFormatType::NormalMap;
            break;
        }
        AssetsImportingManager::ImportIfEdited(texture.FilePath, assetPath, texture.AssetID, &textureOptions);
#endif
    }

    // Prepare materials
    for (int32 i = 0; i < data.Materials.Count(); i++)
    {
        auto& material = data.Materials[i];

        if (material.Name.IsEmpty())
            material.Name = TEXT("Material ") + StringUtils::ToString(i);

        // Auto-import materials
        if (autoImportOutput.IsEmpty() || EnumHasNoneFlags(options.ImportTypes, ImportDataTypes::Materials) || !material.UsesProperties())
            continue;
        String assetPath = GetAdditionalImportPath(autoImportOutput, importedFileNames, material.Name);
#if COMPILE_WITH_ASSETS_IMPORTER
        // When splitting imported meshes allow only the first mesh to import assets (mesh[0] is imported after all following ones so import assets during mesh[1])
        if (!options.SplitObjects && options.ObjectIndex != 1 && options.ObjectIndex != -1)
        {
            // Find that asset created previously
            AssetInfo info;
            if (Content::GetAssetInfo(assetPath, info))
                material.AssetID = info.ID;
            continue;
        }

        // Skip any materials that already exist from the model.
        // This allows the use of "import as material instances" without material properties getting overridden on each import.
        if (options.SkipExistingMaterialsOnReimport)
        {
            AssetInfo info;
            if (Content::GetAssetInfo(assetPath, info))
            {
                material.AssetID = info.ID;
                continue;
            }
        }

        if (options.ImportMaterialsAsInstances)
        {
            // Create material instance
            AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialInstanceTag, assetPath, material.AssetID);
            if (auto* materialInstance = Content::Load<MaterialInstance>(assetPath))
            {
                materialInstance->SetBaseMaterial(options.InstanceToImportAs);
                materialInstance->ResetParameters();

                // Customize base material based on imported material (blind guess based on the common names used in materials)
                Texture* tex;
#define TRY_SETUP_TEXTURE_PARAM(component, names, type) if (material.component.TextureIndex != -1 && ((tex = Content::LoadAsync<Texture>(data.Textures[material.component.TextureIndex].AssetID)))) TrySetupMaterialParameter(materialInstance, ToSpan(names, ARRAY_COUNT(names)), tex, MaterialParameterType::type);
                const Char* diffuseNames[] = { TEXT("color"), TEXT("col"), TEXT("diffuse"), TEXT("basecolor"), TEXT("base color"), TEXT("tint") };
                TrySetupMaterialParameter(materialInstance, ToSpan(diffuseNames, ARRAY_COUNT(diffuseNames)), material.Diffuse.Color, MaterialParameterType::Color);
                TRY_SETUP_TEXTURE_PARAM(Diffuse, diffuseNames, Texture);
                const Char* normalMapNames[] = { TEXT("normals"), TEXT("normalmap"), TEXT("normal map"), TEXT("normal") };
                TRY_SETUP_TEXTURE_PARAM(Normals, normalMapNames, NormalMap);
                const Char* emissiveNames[] = { TEXT("emissive"), TEXT("emission"), TEXT("light"), TEXT("glow") };
                TrySetupMaterialParameter(materialInstance, ToSpan(emissiveNames, ARRAY_COUNT(emissiveNames)), material.Emissive.Color, MaterialParameterType::Color);
                TRY_SETUP_TEXTURE_PARAM(Emissive, emissiveNames, Texture);
                const Char* opacityNames[] = { TEXT("opacity"), TEXT("alpha") };
                TrySetupMaterialParameter(materialInstance, ToSpan(opacityNames, ARRAY_COUNT(opacityNames)), material.Opacity.Value, MaterialParameterType::Float);
                TRY_SETUP_TEXTURE_PARAM(Opacity, opacityNames, Texture);
                const Char* roughnessNames[] = { TEXT("roughness"), TEXT("rough") };
                TrySetupMaterialParameter(materialInstance, ToSpan(roughnessNames, ARRAY_COUNT(roughnessNames)), material.Roughness.Value, MaterialParameterType::Float);
                TRY_SETUP_TEXTURE_PARAM(Roughness, roughnessNames, Texture);
#undef TRY_SETUP_TEXTURE_PARAM

                materialInstance->Save();
            }
            else
            {
                LOG(Error, "Failed to load material instance after creation. ({0})", assetPath);
            }
        }
        else
        {
            // Create material
            CreateMaterial::Options materialOptions;
            materialOptions.Diffuse.Color = material.Diffuse.Color;
            if (material.Diffuse.TextureIndex != -1)
                materialOptions.Diffuse.Texture = data.Textures[material.Diffuse.TextureIndex].AssetID;
            materialOptions.Diffuse.HasAlphaMask = material.Diffuse.HasAlphaMask;
            materialOptions.Emissive.Color = material.Emissive.Color;
            if (material.Emissive.TextureIndex != -1)
                materialOptions.Emissive.Texture = data.Textures[material.Emissive.TextureIndex].AssetID;
            materialOptions.Opacity.Value = material.Opacity.Value;
            if (material.Opacity.TextureIndex != -1)
                materialOptions.Opacity.Texture = data.Textures[material.Opacity.TextureIndex].AssetID;
            materialOptions.Roughness.Value = material.Roughness.Value;
            if (material.Roughness.TextureIndex != -1)
                materialOptions.Roughness.Texture = data.Textures[material.Roughness.TextureIndex].AssetID;
            if (material.Normals.TextureIndex != -1)
                materialOptions.Normals.Texture = data.Textures[material.Normals.TextureIndex].AssetID;
            if (material.TwoSided || material.Diffuse.HasAlphaMask)
                materialOptions.Info.CullMode = CullMode::TwoSided;
            if (!Math::IsOne(material.Opacity.Value) || material.Opacity.TextureIndex != -1)
                materialOptions.Info.BlendMode = MaterialBlendMode::Transparent;
            AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialTag, assetPath, material.AssetID, &materialOptions);
        }
#endif
    }

    // Prepare import transformation
    Transform importTransform(options.Translation, options.Rotation, Float3(options.Scale));
    if (options.UseLocalOrigin && data.LODs.HasItems() && data.LODs[0].Meshes.HasItems())
    {
        importTransform.Translation -= importTransform.Orientation * data.LODs[0].Meshes[0]->OriginTranslation * importTransform.Scale;
    }
    if (options.CenterGeometry && data.LODs.HasItems() && data.LODs[0].Meshes.HasItems())
    {
        // Calculate the bounding box (use LOD0 as a reference)
        BoundingBox box = data.LODs[0].GetBox();
        auto center = data.LODs[0].Meshes[0]->OriginOrientation * importTransform.Orientation * box.GetCenter() * importTransform.Scale * data.LODs[0].Meshes[0]->Scaling;
        importTransform.Translation -= center;
    }

    // Apply the import transformation
    if (!importTransform.IsIdentity() && data.Nodes.HasItems())
    {
        if (options.Type == ModelType::SkinnedModel)
        {
            // Transform the root node using the import transformation
            auto& root = data.Skeleton.RootNode();
            Transform meshTransform = root.LocalTransform.WorldToLocal(importTransform).LocalToWorld(root.LocalTransform);
            root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);

            // Apply import transform on meshes
            Matrix meshTransformMatrix;
            meshTransform.GetWorld(meshTransformMatrix);
            for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
            {
                auto& lod = data.LODs[lodIndex];
                for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
                {
                    lod.Meshes[meshIndex]->TransformBuffer(meshTransformMatrix);
                }
            }

            // Apply import transform on bones
            Matrix importMatrixInv;
            importTransform.GetWorld(importMatrixInv);
            importMatrixInv.Invert();
            for (SkeletonBone& bone : data.Skeleton.Bones)
            {
                if (bone.ParentIndex == -1)
                    bone.LocalTransform = importTransform.LocalToWorld(bone.LocalTransform);
                bone.OffsetMatrix = importMatrixInv * bone.OffsetMatrix;
            }
        }
        else
        {
            // Transform the root node using the import transformation
            auto& root = data.Nodes[0];
            root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);
        }
    }

    // Post-process imported data
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Skeleton))
    {
        if (options.CalculateBoneOffsetMatrices)
        {
            // Calculate offset matrix (inverse bind pose transform) for every bone manually
            for (SkeletonBone& bone : data.Skeleton.Bones)
            {
                CalculateBoneOffsetMatrix(data.Skeleton.Nodes, bone.OffsetMatrix, bone.NodeIndex);
            }
        }

#if USE_SKELETON_NODES_SORTING
        // Sort skeleton nodes and bones hierarchy (parents first)
        // Then it can be used with a simple linear loop update
        {
            const int32 nodesCount = data.Skeleton.Nodes.Count();
            const int32 bonesCount = data.Skeleton.Bones.Count();
            Array<int32> mapping;
            CreateLinearListFromTree(data.Skeleton.Nodes, mapping);
            for (int32 i = 0; i < nodesCount; i++)
            {
                auto& node = data.Skeleton.Nodes[i];
                node.ParentIndex = mapping[node.ParentIndex];
            }
            for (int32 i = 0; i < bonesCount; i++)
            {
                auto& bone = data.Skeleton.Bones[i];
                bone.NodeIndex = mapping[bone.NodeIndex];
            }
        }
        reorder_nodes_and_test_it_out
        !
#endif
    }
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Geometry) && options.Type != ModelType::Prefab)
    {
        // Perform simple nodes mapping to single node (will transform meshes to model local space)
        SkeletonMapping<ModelDataNode> skeletonMapping(data.Nodes, nullptr);

        // Refresh skeleton updater with model skeleton
        SkeletonUpdater<ModelDataNode> hierarchyUpdater(data.Nodes);
        hierarchyUpdater.UpdateMatrices();

        // Move meshes in the new nodes
        for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
        {
            for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
            {
                auto& mesh = *data.LODs[lodIndex].Meshes[meshIndex];

                // Check if there was a remap using model skeleton
                if (skeletonMapping.SourceToSource[mesh.NodeIndex] != mesh.NodeIndex)
                {
                    // Transform vertices
                    const Matrix transformationMatrix = hierarchyUpdater.CombineMatricesFromNodeIndices(skeletonMapping.SourceToSource[mesh.NodeIndex], mesh.NodeIndex);

                    if (!transformationMatrix.IsIdentity())
                        mesh.TransformBuffer(transformationMatrix);
                }

                // Update new node index using real asset skeleton
                mesh.NodeIndex = skeletonMapping.SourceToTarget[mesh.NodeIndex];
            }
        }
    }
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Geometry) && options.Type == ModelType::Prefab)
    {
        // Apply just the scale and rotations.
        for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
        {
            for (int32 meshIndex = 0; meshIndex < data.LODs[lodIndex].Meshes.Count(); meshIndex++)
            {
                auto& mesh = *data.LODs[lodIndex].Meshes[meshIndex];
                auto& node = data.Nodes[mesh.NodeIndex];
                auto currentNode = &data.Nodes[mesh.NodeIndex];

                Vector3 scale = Vector3::One;
                Quaternion rotation = Quaternion::Identity;
                while (true)
                {
                    scale *= currentNode->LocalTransform.Scale;
                    rotation *= currentNode->LocalTransform.Orientation;
                    if (currentNode->ParentIndex == -1)
                        break;
                    currentNode = &data.Nodes[currentNode->ParentIndex];
                }

                // Transform vertices
                auto transformationMatrix = Matrix::Identity;
                transformationMatrix.SetScaleVector(scale);
                transformationMatrix = transformationMatrix * Matrix::RotationQuaternion(rotation);

                if (!transformationMatrix.IsIdentity())
                    mesh.TransformBuffer(transformationMatrix);
            }
        }
    }
    if (EnumHasAnyFlags(options.ImportTypes, ImportDataTypes::Animations))
    {
        for (auto& animation : data.Animations)
        {
            // Trim the animation keyframes range if need to
            if (options.Duration == AnimationDuration::Custom)
            {
                // Custom animation import, frame index start and end
                const float start = options.FramesRange.X;
                const float end = options.FramesRange.Y;
                for (int32 i = 0; i < animation.Channels.Count(); i++)
                {
                    auto& anim = animation.Channels[i];
                    anim.Position.Trim(start, end);
                    anim.Rotation.Trim(start, end);
                    anim.Scale.Trim(start, end);
                }
                animation.Duration = end - start;
            }

            // Change the sampling rate if need to
            if (!Math::IsZero(options.SamplingRate))
            {
                const float timeScale = (float)(animation.FramesPerSecond / options.SamplingRate);
                if (!Math::IsOne(timeScale))
                {
                    animation.FramesPerSecond = options.SamplingRate;
                    for (int32 i = 0; i < animation.Channels.Count(); i++)
                    {
                        auto& anim = animation.Channels[i];
                        anim.Position.TransformTime(timeScale, 0.0f);
                        anim.Rotation.TransformTime(timeScale, 0.0f);
                        anim.Scale.TransformTime(timeScale, 0.0f);
                    }
                }
            }

            // Process root motion setup
            animation.RootMotionFlags = options.RootMotion != RootMotionMode::None ? options.RootMotionFlags : AnimationRootMotionFlags::None;
            animation.RootNodeName = options.RootNodeName.TrimTrailing();
            if (animation.RootMotionFlags != AnimationRootMotionFlags::None && animation.Channels.HasItems())
            {
                if (options.RootMotion == RootMotionMode::ExtractNode)
                {
                    if (animation.RootNodeName.HasChars() && animation.GetChannel(animation.RootNodeName) == nullptr)
                    {
                        LOG(Warning, "Missing Root Motion node '{}'", animation.RootNodeName);
                    }
                }
                else if (options.RootMotion == RootMotionMode::ExtractCenterOfMass && data.Skeleton.Nodes.HasItems()) // TODO: finish implementing this
                {
                    // Setup root node animation track
                    const auto& rootName = data.Skeleton.Nodes.First().Name;
                    auto rootChannelPtr = animation.GetChannel(rootName);
                    if (!rootChannelPtr)
                    {
                        animation.Channels.Insert(0, NodeAnimationData());
                        rootChannelPtr = &animation.Channels[0];
                        rootChannelPtr->NodeName = rootName;
                    }
                    animation.RootNodeName = rootName;
                    auto& rootChannel = *rootChannelPtr;
                    rootChannel.Position.Clear();

                    // Calculate skeleton center of mass position over the animation frames
                    const int32 frames = (int32)animation.Duration;
                    const int32 nodes = data.Skeleton.Nodes.Count();
                    Array<Float3> centerOfMass;
                    centerOfMass.Resize(frames);
                    for (int32 frame = 0; frame < frames; frame++)
                    {
                        auto& key = centerOfMass[frame];

                        // Evaluate skeleton pose at the animation frame
                        AnimGraphImpulse pose;
                        pose.Nodes.Resize(nodes);
                        for (int32 nodeIndex = 0; nodeIndex < nodes; nodeIndex++)
                        {
                            Transform srcNode = data.Skeleton.Nodes[nodeIndex].LocalTransform;
                            auto& node = data.Skeleton.Nodes[nodeIndex];
                            if (auto* channel = animation.GetChannel(node.Name))
                                channel->Evaluate((float)frame, &srcNode, false);
                            pose.Nodes[nodeIndex] = srcNode;
                        }

                        // Calculate average location of the pose (center of mass)
                        key = Float3::Zero;
                        for (int32 nodeIndex = 0; nodeIndex < nodes; nodeIndex++)
                            key += pose.GetNodeModelTransformation(data.Skeleton, nodeIndex).Translation;
                        key /= (float)nodes;
                    }

                    // Calculate skeleton center of mass movement over the animation frames
                    rootChannel.Position.Resize(frames);
                    const Float3 centerOfMassRefPose = centerOfMass[0];
                    for (int32 frame = 0; frame < frames; frame++)
                    {
                        auto& key = rootChannel.Position[frame];
                        key.Time = (float)frame;
                        key.Value = centerOfMass[frame] - centerOfMassRefPose;
                    }

                    // Remove root motion from the children (eg. if Root moves, then Hips should skip that movement delta)
                    Float3 maxMotion = Float3::Zero;
                    for (int32 i = 0; i < animation.Channels.Count(); i++)
                    {
                        auto& anim = animation.Channels[i];
                        const int32 animNodeIndex = data.Skeleton.FindNode(anim.NodeName);

                        // Skip channels that have one of their parents already animated
                        {
                            int32 nodeIndex = animNodeIndex;
                            nodeIndex = data.Skeleton.Nodes[nodeIndex].ParentIndex;
                            while (nodeIndex > 0)
                            {
                                const String& nodeName = data.Skeleton.Nodes[nodeIndex].Name;
                                if (animation.GetChannel(nodeName) != nullptr)
                                    break;
                                nodeIndex = data.Skeleton.Nodes[nodeIndex].ParentIndex;
                            }
                            if (nodeIndex > 0 || &anim == rootChannelPtr)
                                continue;
                        }

                        // Remove motion
                        auto& animPos = anim.Position.GetKeyframes();
                        for (int32 frame = 0; frame < animPos.Count(); frame++)
                        {
                            auto& key = animPos[frame];

                            // Evaluate root motion at the keyframe location
                            Float3 rootMotion;
                            rootChannel.Position.Evaluate(rootMotion, key.Time, false);
                            Float3::Max(maxMotion, rootMotion, maxMotion);

                            // Evaluate skeleton pose at the animation frame
                            AnimGraphImpulse pose;
                            pose.Nodes.Resize(nodes);
                            pose.Nodes[0] = data.Skeleton.Nodes[0].LocalTransform; // Use ref pose of root
                            for (int32 nodeIndex = 1; nodeIndex < nodes; nodeIndex++) // Skip new root
                            {
                                Transform srcNode = data.Skeleton.Nodes[nodeIndex].LocalTransform;
                                auto& node = data.Skeleton.Nodes[nodeIndex];
                                if (auto* channel = animation.GetChannel(node.Name))
                                    channel->Evaluate((float)frame, &srcNode, false);
                                pose.Nodes[nodeIndex] = srcNode;
                            }

                            // Convert root motion to the local space of this node so the node stays at the same location after adding new root channel
                            Transform parentNodeTransform = pose.GetNodeModelTransformation(data.Skeleton, data.Skeleton.Nodes[animNodeIndex].ParentIndex);
                            rootMotion = parentNodeTransform.WorldToLocal(rootMotion);

                            // Remove motion
                            key.Value -= rootMotion;
                        }
                    }
                    LOG(Info, "Calculated root motion: {}", maxMotion);
                }
            }

            // Optimize the keyframes
            if (options.OptimizeKeyframes)
            {
                const int32 before = animation.GetKeyframesCount();
                for (int32 i = 0; i < animation.Channels.Count(); i++)
                {
                    auto& anim = animation.Channels[i];

                    // Optimize keyframes
                    OptimizeCurve(anim.Position);
                    OptimizeCurve(anim.Rotation);
                    OptimizeCurve(anim.Scale);

                    // Remove empty channels
                    if (anim.GetKeyframesCount() == 0)
                    {
                        animation.Channels.RemoveAt(i--);
                    }
                }
                const int32 after = animation.GetKeyframesCount();
                LOG(Info, "Optimized {0} animation keyframe(s). Before: {1}, after: {2}, Ratio: {3}%", before - after, before, after, Utilities::RoundTo2DecimalPlaces((float)after / before));
            }
        }
    }

    // Collision mesh output
    if (options.CollisionMeshesPrefix.HasChars())
    {
        // Extract collision meshes from the model
        ModelData collisionModel;
        for (auto& lod : data.LODs)
        {
            for (int32 i = lod.Meshes.Count() - 1; i >= 0; i--)
            {
                auto mesh = lod.Meshes[i];
                if (mesh->Name.StartsWith(options.CollisionMeshesPrefix, StringSearchCase::IgnoreCase))
                {
                    if (collisionModel.LODs.Count() == 0)
                        collisionModel.LODs.AddOne();
                    collisionModel.LODs[0].Meshes.Add(mesh);
                    lod.Meshes.RemoveAtKeepOrder(i);
                    if (lod.Meshes.IsEmpty())
                        break;
                }
            }
        }
#if COMPILE_WITH_PHYSICS_COOKING
        if (collisionModel.LODs.HasItems() && options.CollisionType != CollisionDataType::None)
        {
            // Cook collision
            String assetPath = GetAdditionalImportPath(autoImportOutput, importedFileNames, TEXT("Collision"));
            CollisionCooking::Argument arg;
            arg.Type = options.CollisionType;
            arg.OverrideModelData = &collisionModel;
            if (CreateCollisionData::CookMeshCollision(assetPath, arg))
            {
                LOG(Error, "Failed to create collision mesh.");
            }
        }
#endif
    }

    // Merge meshes with the same parent nodes, material and skinning
    if (options.MergeMeshes)
    {
        int32 meshesMerged = 0;
        for (int32 lodIndex = 0; lodIndex < data.LODs.Count(); lodIndex++)
        {
            auto& meshes = data.LODs[lodIndex].Meshes;

            // Group meshes that can be merged together
            typedef Pair<int32, int32> MeshGroupKey;
            const Function<MeshGroupKey(MeshData* const&)> f = [](MeshData* const& x) -> MeshGroupKey
            {
                return MeshGroupKey(x->NodeIndex, x->MaterialSlotIndex);
            };
            Array<IGrouping<MeshGroupKey, MeshData*>> meshesByGroup;
            ArrayExtensions::GroupBy(meshes, f, meshesByGroup);

            for (int32 groupIndex = 0; groupIndex < meshesByGroup.Count(); groupIndex++)
            {
                auto& group = meshesByGroup[groupIndex];
                if (group.Count() <= 1)
                    continue;

                // Merge group meshes with the first one
                auto targetMesh = group[0];
                for (int32 i = 1; i < group.Count(); i++)
                {
                    auto mesh = group[i];
                    meshes.Remove(mesh);
                    targetMesh->Merge(*mesh);
                    meshesMerged++;
                    Delete(mesh);
                }
            }
        }
        if (meshesMerged)
            LOG(Info, "Merged {0} meshes", meshesMerged);
    }

    // Automatic LOD generation
    if (options.GenerateLODs && options.LODCount > 1 && data.LODs.HasItems() && options.TriangleReduction < 1.0f - ZeroTolerance)
    {
        auto lodStartTime = DateTime::NowUTC();
        meshopt_setAllocator(MeshOptAllocate, MeshOptDeallocate);
        float triangleReduction = Math::Saturate(options.TriangleReduction);
        int32 lodCount = Math::Max(options.LODCount, data.LODs.Count());
        int32 baseLOD = Math::Clamp(options.BaseLOD, 0, lodCount - 1);
        data.LODs.Resize(lodCount);
        int32 generatedLod = 0, baseLodTriangleCount = 0, baseLodVertexCount = 0;
        for (auto& mesh : data.LODs[baseLOD].Meshes)
        {
            baseLodTriangleCount += mesh->Indices.Count() / 3;
            baseLodVertexCount += mesh->Positions.Count();
        }
        Array<unsigned int> indices;
        for (int32 lodIndex = Math::Clamp(baseLOD + 1, 1, lodCount - 1); lodIndex < lodCount; lodIndex++)
        {
            auto& dstLod = data.LODs[lodIndex];
            const auto& srcLod = data.LODs[lodIndex - 1];

            int32 lodTriangleCount = 0, lodVertexCount = 0;
            dstLod.Meshes.Resize(srcLod.Meshes.Count());
            for (int32 meshIndex = 0; meshIndex < dstLod.Meshes.Count(); meshIndex++)
            {
                auto& dstMesh = dstLod.Meshes[meshIndex] = New<MeshData>();
                const auto& srcMesh = srcLod.Meshes[meshIndex];

                // Setup mesh
                dstMesh->MaterialSlotIndex = srcMesh->MaterialSlotIndex;
                dstMesh->NodeIndex = srcMesh->NodeIndex;
                dstMesh->Name = srcMesh->Name;

                // Simplify mesh using meshoptimizer
                int32 srcMeshIndexCount = srcMesh->Indices.Count();
                int32 srcMeshVertexCount = srcMesh->Positions.Count();
                int32 dstMeshIndexCountTarget = int32(srcMeshIndexCount * triangleReduction) / 3 * 3;
                if (dstMeshIndexCountTarget < 3 || dstMeshIndexCountTarget >= srcMeshIndexCount)
                    continue;
                indices.Clear();
                indices.Resize(srcMeshIndexCount);
                int32 dstMeshIndexCount = {};
                if (options.SloppyOptimization)
                    dstMeshIndexCount = (int32)meshopt_simplifySloppy(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Float3), dstMeshIndexCountTarget, options.LODTargetError);
                else
                    dstMeshIndexCount = (int32)meshopt_simplify(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Float3), dstMeshIndexCountTarget, options.LODTargetError);
                if (dstMeshIndexCount <= 0 || dstMeshIndexCount > indices.Count())
                    continue;
                indices.Resize(dstMeshIndexCount);

                // Generate simplified vertex buffer remapping table (use only vertices from LOD index buffer)
                Array<unsigned int> remap;
                remap.Resize(srcMeshVertexCount);
                int32 dstMeshVertexCount = (int32)meshopt_optimizeVertexFetchRemap(remap.Get(), indices.Get(), dstMeshIndexCount, srcMeshVertexCount);

                // Remap index buffer
                dstMesh->Indices.Resize(dstMeshIndexCount);
                meshopt_remapIndexBuffer(dstMesh->Indices.Get(), indices.Get(), dstMeshIndexCount, remap.Get());

                // Remap vertex buffer
#define REMAP_VERTEX_BUFFER(name, type) \
    if (srcMesh->name.HasItems()) \
    { \
        ASSERT(srcMesh->name.Count() == srcMeshVertexCount); \
        dstMesh->name.Resize(dstMeshVertexCount); \
        meshopt_remapVertexBuffer(dstMesh->name.Get(), srcMesh->name.Get(), srcMeshVertexCount, sizeof(type), remap.Get()); \
    }
                REMAP_VERTEX_BUFFER(Positions, Float3);
                REMAP_VERTEX_BUFFER(UVs, Float2);
                REMAP_VERTEX_BUFFER(Normals, Float3);
                REMAP_VERTEX_BUFFER(Tangents, Float3);
                REMAP_VERTEX_BUFFER(Tangents, Float3);
                REMAP_VERTEX_BUFFER(LightmapUVs, Float2);
                REMAP_VERTEX_BUFFER(Colors, Color);
                REMAP_VERTEX_BUFFER(BlendIndices, Int4);
                REMAP_VERTEX_BUFFER(BlendWeights, Float4);
#undef REMAP_VERTEX_BUFFER

                // Remap blend shapes
                dstMesh->BlendShapes.Resize(srcMesh->BlendShapes.Count());
                for (int32 blendShapeIndex = 0; blendShapeIndex < srcMesh->BlendShapes.Count(); blendShapeIndex++)
                {
                    const auto& srcBlendShape = srcMesh->BlendShapes[blendShapeIndex];
                    auto& dstBlendShape = dstMesh->BlendShapes[blendShapeIndex];

                    dstBlendShape.Name = srcBlendShape.Name;
                    dstBlendShape.Weight = srcBlendShape.Weight;
                    dstBlendShape.Vertices.EnsureCapacity(srcBlendShape.Vertices.Count());
                    for (int32 i = 0; i < srcBlendShape.Vertices.Count(); i++)
                    {
                        auto v = srcBlendShape.Vertices[i];
                        v.VertexIndex = remap[v.VertexIndex];
                        if (v.VertexIndex != ~0u)
                        {
                            dstBlendShape.Vertices.Add(v);
                        }
                    }
                }

                // Remove empty blend shapes
                for (int32 blendShapeIndex = dstMesh->BlendShapes.Count() - 1; blendShapeIndex >= 0; blendShapeIndex--)
                {
                    if (dstMesh->BlendShapes[blendShapeIndex].Vertices.IsEmpty())
                        dstMesh->BlendShapes.RemoveAt(blendShapeIndex);
                }

                // Optimize generated LOD
                meshopt_optimizeVertexCache(dstMesh->Indices.Get(), dstMesh->Indices.Get(), dstMeshIndexCount, dstMeshVertexCount);
                meshopt_optimizeOverdraw(dstMesh->Indices.Get(), dstMesh->Indices.Get(), dstMeshIndexCount, (const float*)dstMesh->Positions.Get(), dstMeshVertexCount, sizeof(Float3), 1.05f);

                lodTriangleCount += dstMeshIndexCount / 3;
                lodVertexCount += dstMeshVertexCount;
                generatedLod++;
            }

            // Remove empty meshes (no LOD was generated for them)
            for (int32 i = dstLod.Meshes.Count() - 1; i >= 0; i--)
            {
                MeshData* mesh = dstLod.Meshes[i];
                if (mesh->Indices.IsEmpty() || mesh->Positions.IsEmpty())
                {
                    Delete(mesh);
                    dstLod.Meshes.RemoveAtKeepOrder(i);
                }
            }

            LOG(Info, "Generated LOD{0}: triangles: {1} ({2}% of base LOD), verticies: {3} ({4}% of base LOD)",
                lodIndex,
                lodTriangleCount, (int32)(lodTriangleCount * 100 / baseLodTriangleCount),
                lodVertexCount, (int32)(lodVertexCount * 100 / baseLodVertexCount));
        }
        for (int32 lodIndex = data.LODs.Count() - 1; lodIndex > 0; lodIndex--)
        {
            if (data.LODs[lodIndex].Meshes.IsEmpty())
                data.LODs.RemoveAt(lodIndex);
            else
                break;
        }
        if (generatedLod)
        {
            auto lodEndTime = DateTime::NowUTC();
            LOG(Info, "Generated LODs for {1} meshes in {0} ms", static_cast<int32>((lodEndTime - lodStartTime).GetTotalMilliseconds()), generatedLod);
        }
    }

    // Calculate blend shapes vertices ranges
    for (auto& lod : data.LODs)
    {
        for (auto& mesh : lod.Meshes)
        {
            for (auto& blendShape : mesh->BlendShapes)
            {
                // Compute min/max for used vertex indices
                blendShape.MinVertexIndex = MAX_uint32;
                blendShape.MaxVertexIndex = 0;
                blendShape.UseNormals = false;
                for (int32 i = 0; i < blendShape.Vertices.Count(); i++)
                {
                    auto& v = blendShape.Vertices[i];
                    blendShape.MinVertexIndex = Math::Min(blendShape.MinVertexIndex, v.VertexIndex);
                    blendShape.MaxVertexIndex = Math::Max(blendShape.MaxVertexIndex, v.VertexIndex);
                    blendShape.UseNormals |= !v.NormalDelta.IsZero();
                }
            }
        }
    }

    // Auto calculate LODs transition settings
    data.CalculateLODsScreenSizes();

    const auto endTime = DateTime::NowUTC();
    LOG(Info, "Model file imported in {0} ms", static_cast<int32>((endTime - startTime).GetTotalMilliseconds()));

    return false;
}

int32 ModelTool::DetectLodIndex(const String& nodeName)
{
    int32 index = nodeName.FindLast(TEXT("LOD"), StringSearchCase::IgnoreCase);
    if (index != -1)
    {
        // Some models use LOD_0 to identify LOD levels
        if (nodeName.Length() > index + 4 && nodeName[index + 3] == '_')
            index++;

        int32 num;
        if (!StringUtils::Parse(nodeName.Get() + index + 3, &num))
        {
            if (num >= 0 && num < MODEL_MAX_LODS)
                return num;
            LOG(Warning, "Invalid mesh level of detail index at node \'{0}\'. Maximum supported amount of LODs is {1}.", nodeName, MODEL_MAX_LODS);
        }
    }
    return 0;
}

bool ModelTool::FindTexture(const String& sourcePath, const String& file, String& path)
{
    const String sourceFolder = StringUtils::GetDirectoryName(sourcePath);
    path = sourceFolder / file;
    if (!FileSystem::FileExists(path))
    {
        const String filename = StringUtils::GetFileName(file);
        path = sourceFolder / filename;
        if (!FileSystem::FileExists(path))
        {
            path = sourceFolder / TEXT("textures") / filename;
            if (!FileSystem::FileExists(path))
            {
                path = sourceFolder / TEXT("Textures") / filename;
                if (!FileSystem::FileExists(path))
                {
                    path = sourceFolder / TEXT("texture") / filename;
                    if (!FileSystem::FileExists(path))
                    {
                        path = sourceFolder / TEXT("Texture") / filename;
                        if (!FileSystem::FileExists(path))
                        {
                            path = sourceFolder / TEXT("../textures") / filename;
                            if (!FileSystem::FileExists(path))
                            {
                                path = sourceFolder / TEXT("../Textures") / filename;
                                if (!FileSystem::FileExists(path))
                                {
                                    path = sourceFolder / TEXT("../texture") / filename;
                                    if (!FileSystem::FileExists(path))
                                    {
                                        path = sourceFolder / TEXT("../Texture") / filename;
                                        if (!FileSystem::FileExists(path))
                                        {
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    FileSystem::NormalizePath(path);
    return false;
}

void ModelTool::CalculateBoneOffsetMatrix(const Array<SkeletonNode>& nodes, Matrix& offsetMatrix, int32 nodeIndex)
{
    offsetMatrix = Matrix::Identity;
    int32 idx = nodeIndex;
    do
    {
        const SkeletonNode& node = nodes[idx];
        offsetMatrix *= node.LocalTransform.GetWorld();
        idx = node.ParentIndex;
    } while (idx != -1);
    offsetMatrix.Invert();
}

#endif

#endif
