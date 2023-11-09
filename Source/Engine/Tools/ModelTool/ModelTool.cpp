// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_MODEL_TOOL

#include "ModelTool.h"
#include "MeshAccelerationStructure.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/RandomStream.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/JobSystem.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Content.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#if USE_EDITOR
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Graphics/Models/SkeletonUpdater.h"
#include "Engine/Graphics/Models/SkeletonMapping.h"
#include "Engine/Core/Utilities.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Utilities/RectPack.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/ContentImporters/AssetsImportingManager.h"
#include "Engine/ContentImporters/CreateMaterial.h"
#include "Engine/ContentImporters/CreateMaterialInstance.h"
#include "Engine/ContentImporters/CreateCollisionData.h"
#include "Engine/Serialization/Serialization.h"
#include "Editor/Utilities/EditorUtilities.h"
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

bool ModelTool::GenerateModelSDF(Model* inputModel, ModelData* modelData, float resolutionScale, int32 lodIndex, ModelBase::SDFData* outputSDF, MemoryWriteStream* outputStream, const StringView& assetName, float backfacesThreshold)
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
    Float3 size = bounds.GetSize();
    ModelBase::SDFData sdf;
    sdf.WorldUnitsPerVoxel = 10 / Math::Max(resolutionScale, 0.0001f);
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
    // TODO: maybe apply 1 voxel margin around the geometry?
    const int32 maxMips = 3;
    const int32 mipCount = Math::Min(MipLevelsCount(resolution.X, resolution.Y, resolution.Z, true), maxMips);
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
    GPUTextureDescription textureDesc = GPUTextureDescription::New3D(resolution.X, resolution.Y, resolution.Z, format, GPUTextureFlags::ShaderResource, mipCount);
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
#if !BUILD_RELEASE
        outputSDF->Texture->SetName(TEXT("ModelSDF"));
#endif
    }

    // TODO: support GPU to generate model SDF on-the-fly (if called during rendering)

    // Setup acceleration structure for fast ray tracing the mesh triangles
    MeshAccelerationStructure scene;
    if (inputModel)
        scene.Add(inputModel, lodIndex);
    else if (modelData)
        scene.Add(modelData, lodIndex);
    scene.BuildBVH();

    // Allocate memory for the distant field
    const int32 voxelsSize = resolution.X * resolution.Y * resolution.Z * formatStride;
    void* voxels = Allocator::Allocate(voxelsSize);
    Float3 xyzToLocalMul = uvwToLocalMul / Float3(resolution - 1);
    Float3 xyzToLocalAdd = uvwToLocalAdd;
    const Float2 encodeMAD(0.5f / sdf.MaxDistance * formatMaxValue, 0.5f * formatMaxValue);
    const Float2 decodeMAD(2.0f * sdf.MaxDistance / formatMaxValue, -sdf.MaxDistance);
    int32 voxelSizeSum = voxelsSize;

    // TODO: use optimized sparse storage for SDF data as hierarchical bricks as in papers below:
    // https://graphics.pixar.com/library/IrradianceAtlas/paper.pdf
    // http://maverick.inria.fr/Membres/Cyril.Crassin/thesis/CCrassinThesis_EN_Web.pdf
    // http://ramakarl.com/pdfs/2016_Hoetzlein_GVDB.pdf
    // https://www.cse.chalmers.se/~uffe/HighResolutionSparseVoxelDAGs.pdf

    // Brute-force for each voxel to calculate distance to the closest triangle with point query and distance sign by raycasting around the voxel
    const int32 sampleCount = 12;
    Array<Float3> sampleDirections;
    sampleDirections.Resize(sampleCount);
    {
        RandomStream rand;
        sampleDirections.Get()[0] = Float3::Up;
        sampleDirections.Get()[1] = Float3::Down;
        sampleDirections.Get()[2] = Float3::Left;
        sampleDirections.Get()[3] = Float3::Right;
        sampleDirections.Get()[4] = Float3::Forward;
        sampleDirections.Get()[5] = Float3::Backward;
        for (int32 i = 6; i < sampleCount; i++)
            sampleDirections.Get()[i] = rand.GetUnitVector();
    }
    Function<void(int32)> sdfJob = [&sdf, &resolution, &backfacesThreshold, &sampleDirections, &scene, &voxels, &xyzToLocalMul, &xyzToLocalAdd, &encodeMAD, &formatStride, &formatWrite](int32 z)
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
                for (int32 sample = 0; sample < sampleDirections.Count(); sample++)
                {
                    Ray sampleRay(voxelPos, sampleDirections[sample]);
                    if (scene.RayCast(sampleRay, hitDistance, hitNormal, hitTriangle))
                    {
                        hitCount++;
                        const bool backHit = Float3::Dot(sampleRay.Direction, hitTriangle.GetNormal()) > 0;
                        if (backHit)
                            hitBackCount++;
                    }
                }

                float distance = (float)minDistance;
                // TODO: surface thickness threshold? shift reduce distance for all voxels by something like 0.01 to enlarge thin geometry
                // if ((float)hitBackCount > (float)hitCount * 0.3f && hitCount != 0)
                if ((float)hitBackCount > (float)sampleDirections.Count() * backfacesThreshold && hitCount != 0)
                {
                    // Voxel is inside the geometry so turn it into negative distance to the surface
                    distance *= -1;
                }
                const int32 xAddress = x + yAddress;
                formatWrite((byte*)voxels + xAddress * formatStride, distance * encodeMAD.X + encodeMAD.Y);
            }
        }
    };
    JobSystem::Execute(sdfJob, resolution.Z);

    // Cache SDF data on a CPU
    if (outputStream)
    {
        outputStream->WriteInt32(1); // Version
        ModelSDFHeader data(sdf, textureDesc);
        outputStream->WriteBytes(&data, sizeof(data));
        ModelSDFMip mipData(0, resolution.X * formatStride, voxelsSize);
        outputStream->WriteBytes(&mipData, sizeof(mipData));
        outputStream->WriteBytes(voxels, voxelsSize);
    }

    // Upload data to the GPU
    if (outputSDF)
    {
        BytesContainer data;
        data.Link((byte*)voxels, voxelsSize);
        auto task = outputSDF->Texture->UploadMipMapAsync(data, 0, resolution.X * formatStride, voxelsSize, true);
        if (task)
            task->Start();
    }

    // Generate mip maps
    void* voxelsMip = nullptr;
    for (int32 mipLevel = 1; mipLevel < mipCount; mipLevel++)
    {
        Int3 resolutionMip = Int3::Max(resolution / 2, Int3::One);
        const int32 voxelsMipSize = resolutionMip.X * resolutionMip.Y * resolutionMip.Z * formatStride;
        if (voxelsMip == nullptr)
            voxelsMip = Allocator::Allocate(voxelsMipSize);

        // Downscale mip
        Function<void(int32)> mipJob = [&voxelsMip, &voxels, &resolution, &resolutionMip, &encodeMAD, &decodeMAD, &formatStride, &formatRead, &formatWrite](int32 z)
        {
            PROFILE_CPU_NAMED("Model SDF Mip Job");
            const int32 zAddress = resolutionMip.Y * resolutionMip.X * z;
            for (int32 y = 0; y < resolutionMip.Y; y++)
            {
                const int32 yAddress = resolutionMip.X * y + zAddress;
                for (int32 x = 0; x < resolutionMip.X; x++)
                {
                    // Linear box filter around the voxel
                    // TODO: use min distance for nearby texels (texel distance + distance to texel)
                    float distance = 0;
                    for (int32 dz = 0; dz < 2; dz++)
                    {
                        const int32 dzAddress = (z * 2 + dz) * (resolution.Y * resolution.X);
                        for (int32 dy = 0; dy < 2; dy++)
                        {
                            const int32 dyAddress = (y * 2 + dy) * (resolution.X) + dzAddress;
                            for (int32 dx = 0; dx < 2; dx++)
                            {
                                const int32 dxAddress = (x * 2 + dx) + dyAddress;
                                const float d = formatRead((byte*)voxels + dxAddress * formatStride) * decodeMAD.X + decodeMAD.Y;
                                distance += d;
                            }
                        }
                    }
                    distance *= 1.0f / 8.0f;

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
        Swap(voxelsMip, voxels);
        resolution = resolutionMip;
    }

    Allocator::Free(voxelsMip);
    Allocator::Free(voxels);

#if !BUILD_RELEASE
    auto endTime = Platform::GetTimeSeconds();
    LOG(Info, "Generated SDF {}x{}x{} ({} kB) in {}ms for {}", resolution.X, resolution.Y, resolution.Z, voxelSizeSum / 1024, (int32)((endTime - startTime) * 1000.0), assetName);
#endif
    return false;
}

#if USE_EDITOR

BoundingBox ImportedModelData::LOD::GetBox() const
{
    if (Meshes.IsEmpty())
        return BoundingBox::Empty;

    BoundingBox box;
    Meshes[0]->CalculateBox(box);
    for (int32 i = 1; i < Meshes.Count(); i++)
    {
        if (Meshes[i]->Positions.HasItems())
        {
            BoundingBox t;
            Meshes[i]->CalculateBox(t);
            BoundingBox::Merge(box, t, box);
        }
    }

    return box;
}

void ModelTool::Options::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(ModelTool::Options);

    SERIALIZE(Type);
    SERIALIZE(CalculateNormals);
    SERIALIZE(SmoothingNormalsAngle);
    SERIALIZE(FlipNormals);
    SERIALIZE(CalculateTangents);
    SERIALIZE(SmoothingTangentsAngle);
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
    SERIALIZE(EnableRootMotion);
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
    DESERIALIZE(EnableRootMotion);
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
}

void RemoveNamespace(String& name)
{
    const int32 namespaceStart = name.Find(':');
    if (namespaceStart != -1)
        name = name.Substring(namespaceStart + 1);
}

bool ModelTool::ImportData(const String& path, ImportedModelData& data, Options& options, String& errorMsg)
{
    // Validate options
    options.Scale = Math::Clamp(options.Scale, 0.0001f, 100000.0f);
    options.SmoothingNormalsAngle = Math::Clamp(options.SmoothingNormalsAngle, 0.0f, 175.0f);
    options.SmoothingTangentsAngle = Math::Clamp(options.SmoothingTangentsAngle, 0.0f, 45.0f);
    options.FramesRange.Y = Math::Max(options.FramesRange.Y, options.FramesRange.X);
    options.DefaultFrameRate = Math::Max(0.0f, options.DefaultFrameRate);
    options.SamplingRate = Math::Max(0.0f, options.SamplingRate);

    // Validate path
    // Note: Assimp/Autodesk supports only ANSI characters in imported file path
    StringAnsi importPath;
    String tmpPath;
    if (path.IsANSI() == false)
    {
        // Use temporary file
        LOG(Warning, "Model Tool doesn't support importing files from paths using non ASNI characters. Using temporary file.");
        FileSystem::GetTempFilePath(tmpPath);
        if (tmpPath.IsANSI() == false || FileSystem::CopyFile(tmpPath, path))
        {
            errorMsg = TEXT("Path with non ANSI characters is invalid.");
            return true;
        }
        importPath = tmpPath.ToStringAnsi();
    }
    else
    {
        importPath = path.ToStringAnsi();
    }

    // Call importing backend
#if (USE_AUTODESK_FBX_SDK || USE_OPEN_FBX) && USE_ASSIMP
    if (path.EndsWith(TEXT(".fbx"), StringSearchCase::IgnoreCase))
    {
#if USE_AUTODESK_FBX_SDK
        if (ImportDataAutodeskFbxSdk(importPath.Get(), data, options, errorMsg))
            return true;
#elif USE_OPEN_FBX
        if (ImportDataOpenFBX(importPath.Get(), data, options, errorMsg))
            return true;
#endif
    }
    else
    {
        if (ImportDataAssimp(importPath.Get(), data, options, errorMsg))
            return true;
    }
#elif USE_ASSIMP
    if (ImportDataAssimp(importPath.Get(), data, options, errorMsg))
        return true;
#elif USE_AUTODESK_FBX_SDK
    if (ImportDataAutodeskFbxSdk(importPath.Get(), data, options, errorMsg))
        return true;
#elif USE_OPEN_FBX
    if (ImportDataOpenFBX(importPath.Get(), data, options, errorMsg))
        return true;
#else
    LOG(Error, "Compiled without model importing backend.");
    return true;
#endif

    // Remove temporary file
    if (tmpPath.HasChars() && FileSystem::FileExists(tmpPath))
    {
        FileSystem::DeleteFile(tmpPath);
    }

    // TODO: check model LODs sequence (eg. {LOD0, LOD2, LOD5} is invalid)

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
        for (auto& channel : data.Animation.Channels)
        {
            RemoveNamespace(channel.NodeName);
        }
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                RemoveNamespace(mesh->Name);
                for (auto& blendShape : mesh->BlendShapes)
                {
                    RemoveNamespace(blendShape.Name);
                }
            }
        }
    }

    // Validate the animation channels
    if (data.Animation.Channels.HasItems())
    {
        // Validate bone animations uniqueness
        auto& channels = data.Animation.Channels;
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
    if (options.FlipNormals && EnumHasAnyFlags(data.Types, ImportDataTypes::Geometry))
    {
        for (auto& lod : data.LODs)
        {
            for (auto& mesh : lod.Meshes)
            {
                for (auto& n : mesh->Normals)
                    n *= -1;
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
            return;
        }
    }
}

bool ModelTool::ImportModel(const String& path, ModelData& meshData, Options& options, String& errorMsg, const String& autoImportOutput)
{
    LOG(Info, "Importing model from \'{0}\'", path);
    const auto startTime = DateTime::NowUTC();

    // Import data
    ImportDataTypes importDataTypes;
    switch (options.Type)
    {
    case ModelType::Model:
        importDataTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes | ImportDataTypes::Textures;
        if (options.ImportMaterials)
            importDataTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            importDataTypes |= ImportDataTypes::Textures;
        break;
    case ModelType::SkinnedModel:
        importDataTypes = ImportDataTypes::Geometry | ImportDataTypes::Nodes | ImportDataTypes::Skeleton;
        if (options.ImportMaterials)
            importDataTypes |= ImportDataTypes::Materials;
        if (options.ImportTextures)
            importDataTypes |= ImportDataTypes::Textures;
        break;
    case ModelType::Animation:
        importDataTypes = ImportDataTypes::Animations;
        break;
    default:
        return true;
    }
    ImportedModelData data(importDataTypes);
    if (ImportData(path, data, options, errorMsg))
        return true;

    // Validate result data
    switch (options.Type)
    {
    case ModelType::Model:
    {
        // Validate
        if (data.LODs.IsEmpty() || data.LODs[0].Meshes.IsEmpty())
        {
            errorMsg = TEXT("Imported model has no valid geometry.");
            return true;
        }

        LOG(Info, "Imported model has {0} LODs, {1} meshes (in LOD0) and {2} materials", data.LODs.Count(), data.LODs[0].Meshes.Count(), data.Materials.Count());
        break;
    }
    case ModelType::SkinnedModel:
    {
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

        // Validate
        if (data.Skeleton.Bones.Count() > MAX_BONES_PER_MODEL)
        {
            errorMsg = String::Format(TEXT("Imported model skeleton has too many bones. Imported: {0}, maximum supported: {1}. Please optimize your asset."), data.Skeleton.Bones.Count(), MAX_BONES_PER_MODEL);
            return true;
        }
        if (data.LODs.Count() > 1)
        {
            LOG(Warning, "Imported skinned model has more than one LOD. Removing the lower LODs. Only single one is supported.");
            data.LODs.Resize(1);
        }
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
#if BUILD_DEBUG
            else
            {
                auto& indices = mesh->BlendIndices;
                for (int32 j = 0; j < indices.Count(); j++)
                {
                    const int32 min = indices[j].MinValue();
                    const int32 max = indices[j].MaxValue();
                    if (min < 0 || max >= data.Skeleton.Bones.Count())
                    {
                        LOG(Warning, "Imported mesh \'{0}\' has invalid blend indices. It may result in invalid rendering.", mesh->Name);
                    }
                }

                auto& weights = mesh->BlendWeights;
                for (int32 j = 0; j < weights.Count(); j++)
                {
                    const float sum = weights[j].SumValues();
                    if (Math::Abs(sum - 1.0f) > ZeroTolerance)
                    {
                        LOG(Warning, "Imported mesh \'{0}\' has invalid blend weights. It may result in invalid rendering.", mesh->Name);
                    }
                }
            }
#endif
        }

        LOG(Info, "Imported skeleton has {0} bones, {3} nodes, {1} meshes and {2} material", data.Skeleton.Bones.Count(), meshesCount, data.Materials.Count(), data.Nodes.Count());
        break;
    }
    case ModelType::Animation:
    {
        // Validate
        if (data.Animation.Channels.IsEmpty())
        {
            errorMsg = TEXT("Imported file has no valid animations.");
            return true;
        }

        LOG(Info, "Imported animation has {0} channels, duration: {1} frames, frames per second: {2}", data.Animation.Channels.Count(), data.Animation.Duration, data.Animation.FramesPerSecond);
        break;
    }
    }

    // Prepare textures
    Array<String> importedFileNames;
    for (int32 i = 0; i < data.Textures.Count(); i++)
    {
        auto& texture = data.Textures[i];

        // Auto-import textures
        if (autoImportOutput.IsEmpty() || (data.Types & ImportDataTypes::Textures) == ImportDataTypes::None || texture.FilePath.IsEmpty())
            continue;
        String filename = StringUtils::GetFileNameWithoutExtension(texture.FilePath);
        for (int32 j = filename.Length() - 1; j >= 0; j--)
        {
            if (EditorUtilities::IsInvalidPathChar(filename[j]))
                filename[j] = ' ';
        }
        if (importedFileNames.Contains(filename))
        {
            int32 counter = 1;
            do
            {
                filename = String(StringUtils::GetFileNameWithoutExtension(texture.FilePath)) + TEXT(" ") + StringUtils::ToString(counter);
                counter++;
            } while (importedFileNames.Contains(filename));
        }
        importedFileNames.Add(filename);
#if COMPILE_WITH_ASSETS_IMPORTER
        auto assetPath = autoImportOutput / filename + ASSET_FILES_EXTENSION_WITH_DOT;
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
        default: ;
        }
        AssetsImportingManager::ImportIfEdited(texture.FilePath, assetPath, texture.AssetID, &textureOptions);
#endif
    }

    // Prepare material
    for (int32 i = 0; i < data.Materials.Count(); i++)
    {
        auto& material = data.Materials[i];

        if (material.Name.IsEmpty())
            material.Name = TEXT("Material ") + StringUtils::ToString(i);

        // Auto-import materials
        if (autoImportOutput.IsEmpty() || (data.Types & ImportDataTypes::Materials) == ImportDataTypes::None || !material.UsesProperties())
            continue;
        auto filename = material.Name;
        for (int32 j = filename.Length() - 1; j >= 0; j--)
        {
            if (EditorUtilities::IsInvalidPathChar(filename[j]))
                filename[j] = ' ';
        }
        if (importedFileNames.Contains(filename))
        {
            int32 counter = 1;
            do
            {
                filename = material.Name + TEXT(" ") + StringUtils::ToString(counter);
                counter++;
            } while (importedFileNames.Contains(filename));
        }
        importedFileNames.Add(filename);
#if COMPILE_WITH_ASSETS_IMPORTER
        auto assetPath = autoImportOutput / filename + ASSET_FILES_EXTENSION_WITH_DOT;

        // When splitting imported meshes allow only the first mesh to import assets (mesh[0] is imported after all following ones so import assets during mesh[1])
        if (!options.SplitObjects && options.ObjectIndex != 1 && options.ObjectIndex != -1)
        {
            // Find that asset create previously
            AssetInfo info;
            if (Content::GetAssetInfo(assetPath, info))
                material.AssetID = info.ID;
            continue;
        }

        if (options.ImportMaterialsAsInstances)
        {
            // Create material instance
            AssetsImportingManager::Create(AssetsImportingManager::CreateMaterialInstanceTag, assetPath, material.AssetID);
            if (auto* materialInstance = Content::Load<MaterialInstance>(assetPath))
            {
                materialInstance->SetBaseMaterial(options.InstanceToImportAs);

                // Customize base material based on imported material (blind guess based on the common names used in materials)
                const Char* diffuseColorNames[] = { TEXT("color"), TEXT("col"), TEXT("diffuse"), TEXT("basecolor"), TEXT("base color") };
                TrySetupMaterialParameter(materialInstance, ToSpan(diffuseColorNames, ARRAY_COUNT(diffuseColorNames)), material.Diffuse.Color, MaterialParameterType::Color);
                const Char* emissiveColorNames[] = { TEXT("emissive"), TEXT("emission"), TEXT("light") };
                TrySetupMaterialParameter(materialInstance, ToSpan(emissiveColorNames, ARRAY_COUNT(emissiveColorNames)), material.Emissive.Color, MaterialParameterType::Color);
                const Char* opacityValueNames[] = { TEXT("opacity"), TEXT("alpha") };
                TrySetupMaterialParameter(materialInstance, ToSpan(opacityValueNames, ARRAY_COUNT(opacityValueNames)), material.Opacity.Value, MaterialParameterType::Float);

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
    const bool applyImportTransform = !importTransform.IsIdentity();

    // Post-process imported data based on a target asset type
    if (options.Type == ModelType::Model)
    {
        if (data.Nodes.IsEmpty())
        {
            errorMsg = TEXT("Missing model nodes.");
            return true;
        }

        // Apply the import transformation
        if (applyImportTransform)
        {
            // Transform the root node using the import transformation
            auto& root = data.Nodes[0];
            root.LocalTransform = importTransform.LocalToWorld(root.LocalTransform);
        }

        // Perform simple nodes mapping to single node (will transform meshes to model local space)
        SkeletonMapping<ImportedModelData::Node> skeletonMapping(data.Nodes, nullptr);

        // Refresh skeleton updater with model skeleton
        SkeletonUpdater<ImportedModelData::Node> hierarchyUpdater(data.Nodes);
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
                    const auto transformationMatrix = hierarchyUpdater.CombineMatricesFromNodeIndices(skeletonMapping.SourceToSource[mesh.NodeIndex], mesh.NodeIndex);
                    if (!transformationMatrix.IsIdentity())
                        mesh.TransformBuffer(transformationMatrix);
                }

                // Update new node index using real asset skeleton
                mesh.NodeIndex = skeletonMapping.SourceToTarget[mesh.NodeIndex];
            }
        }

        // Collision mesh output
        if (options.CollisionMeshesPrefix.HasChars())
        {
            // Extract collision meshes
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
            if (collisionModel.LODs.HasItems())
            {
#if COMPILE_WITH_PHYSICS_COOKING
                // Create collision
                CollisionCooking::Argument arg;
                arg.Type = options.CollisionType;
                arg.OverrideModelData = &collisionModel;
                auto assetPath = autoImportOutput / StringUtils::GetFileNameWithoutExtension(path) + TEXT("Collision") ASSET_FILES_EXTENSION_WITH_DOT;
                if (CreateCollisionData::CookMeshCollision(assetPath, arg))
                {
                    LOG(Error, "Failed to create collision mesh.");
                }
#endif
            }
        }

        // For generated lightmap UVs coordinates needs to be moved so all meshes are in unique locations in [0-1]x[0-1] coordinates space
        if (options.LightmapUVsSource == ModelLightmapUVsSource::Generate && data.LODs.HasItems() && data.LODs[0].Meshes.Count() > 1)
        {
            // Use weight-based coordinates space placement and rect-pack to allocate more space for bigger meshes in the model lightmap chart
            int32 lodIndex = 0;
            auto& lod = data.LODs[lodIndex];

            // Build list of meshes with their area
            struct LightmapUVsPack : RectPack<LightmapUVsPack, float>
            {
                LightmapUVsPack(float x, float y, float width, float height)
                    : RectPack<LightmapUVsPack, float>(x, y, width, height)
                {
                }

                void OnInsert()
                {
                }
            };
            struct MeshEntry
            {
                MeshData* Mesh;
                float Area;
                float Size;
                LightmapUVsPack* Slot;
            };
            Array<MeshEntry> entries;
            entries.Resize(lod.Meshes.Count());
            float areaSum = 0;
            for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
            {
                auto& entry = entries[meshIndex];
                entry.Mesh = lod.Meshes[meshIndex];
                entry.Area = entry.Mesh->CalculateTrianglesArea();
                entry.Size = Math::Sqrt(entry.Area);
                areaSum += entry.Area;
            }

            if (areaSum > ZeroTolerance)
            {
                // Pack all surfaces into atlas
                float atlasSize = Math::Sqrt(areaSum) * 1.02f;
                int32 triesLeft = 10;
                while (triesLeft--)
                {
                    bool failed = false;
                    const float chartsPadding = (4.0f / 256.0f) * atlasSize;
                    LightmapUVsPack root(chartsPadding, chartsPadding, atlasSize - chartsPadding, atlasSize - chartsPadding);
                    for (auto& entry : entries)
                    {
                        entry.Slot = root.Insert(entry.Size, entry.Size, chartsPadding);
                        if (entry.Slot == nullptr)
                        {
                            // Failed to insert surface, increase atlas size and try again
                            atlasSize *= 1.5f;
                            failed = true;
                            break;
                        }
                    }

                    if (!failed)
                    {
                        // Transform meshes lightmap UVs into the slots in the whole atlas
                        const float atlasSizeInv = 1.0f / atlasSize;
                        for (const auto& entry : entries)
                        {
                            Float2 uvOffset(entry.Slot->X * atlasSizeInv, entry.Slot->Y * atlasSizeInv);
                            Float2 uvScale((entry.Slot->Width - chartsPadding) * atlasSizeInv, (entry.Slot->Height - chartsPadding) * atlasSizeInv);
                            // TODO: SIMD
                            for (auto& uv : entry.Mesh->LightmapUVs)
                            {
                                uv = uv * uvScale + uvOffset;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    else if (options.Type == ModelType::SkinnedModel)
    {
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

        // Apply the import transformation
        if (applyImportTransform)
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
                {
                    bone.LocalTransform = importTransform.LocalToWorld(bone.LocalTransform);
                }
                bone.OffsetMatrix = importMatrixInv * bone.OffsetMatrix;
            }
        }

        // Perform simple nodes mapping to single node (will transform meshes to model local space)
        SkeletonMapping<ImportedModelData::Node> skeletonMapping(data.Nodes, nullptr);

        // Refresh skeleton updater with model skeleton
        SkeletonUpdater<ImportedModelData::Node> hierarchyUpdater(data.Nodes);
        hierarchyUpdater.UpdateMatrices();

        if (options.CalculateBoneOffsetMatrices)
        {
            // Calculate offset matrix (inverse bind pose transform) for every bone manually
            for (SkeletonBone& bone : data.Skeleton.Bones)
            {
                CalculateBoneOffsetMatrix(data.Skeleton.Nodes, bone.OffsetMatrix, bone.NodeIndex);
            }
        }

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
                    const auto transformationMatrix = hierarchyUpdater.CombineMatricesFromNodeIndices(skeletonMapping.SourceToSource[mesh.NodeIndex], mesh.NodeIndex);
                    if (!transformationMatrix.IsIdentity())
                        mesh.TransformBuffer(transformationMatrix);
                }

                // Update new node index using real asset skeleton
                mesh.NodeIndex = skeletonMapping.SourceToTarget[mesh.NodeIndex];
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
    else if (options.Type == ModelType::Animation)
    {
        // Trim the animation keyframes range if need to
        if (options.Duration == AnimationDuration::Custom)
        {
            // Custom animation import, frame index start and end
            const float start = options.FramesRange.X;
            const float end = options.FramesRange.Y;
            for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
            {
                auto& anim = data.Animation.Channels[i];
                anim.Position.Trim(start, end);
                anim.Rotation.Trim(start, end);
                anim.Scale.Trim(start, end);
            }
            data.Animation.Duration = end - start;
        }

        // Change the sampling rate if need to
        if (!Math::IsZero(options.SamplingRate))
        {
            const float timeScale = (float)(data.Animation.FramesPerSecond / options.SamplingRate);
            if (!Math::IsOne(timeScale))
            {
                data.Animation.FramesPerSecond = options.SamplingRate;
                for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
                {
                    auto& anim = data.Animation.Channels[i];

                    anim.Position.TransformTime(timeScale, 0.0f);
                    anim.Rotation.TransformTime(timeScale, 0.0f);
                    anim.Scale.TransformTime(timeScale, 0.0f);
                }
            }
        }

        // Optimize the keyframes
        if (options.OptimizeKeyframes)
        {
            const int32 before = data.Animation.GetKeyframesCount();
            for (int32 i = 0; i < data.Animation.Channels.Count(); i++)
            {
                auto& anim = data.Animation.Channels[i];

                // Optimize keyframes
                OptimizeCurve(anim.Position);
                OptimizeCurve(anim.Rotation);
                OptimizeCurve(anim.Scale);

                // Remove empty channels
                if (anim.GetKeyframesCount() == 0)
                {
                    data.Animation.Channels.RemoveAt(i--);
                }
            }
            const int32 after = data.Animation.GetKeyframesCount();
            LOG(Info, "Optimized {0} animation keyframe(s). Before: {1}, after: {2}, Ratio: {3}%", before - after, before, after, Utilities::RoundTo2DecimalPlaces((float)after / before));
        }

        data.Animation.EnableRootMotion = options.EnableRootMotion;
        data.Animation.RootNodeName = options.RootNodeName;
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
        {
            LOG(Info, "Merged {0} meshes", meshesMerged);
        }
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
                Array<unsigned int> indices;
                indices.Resize(dstMeshIndexCountTarget);
                int32 dstMeshIndexCount = {};
                if (options.SloppyOptimization)
                    dstMeshIndexCount = (int32)meshopt_simplifySloppy(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Float3), dstMeshIndexCountTarget);
                else
                    dstMeshIndexCount = (int32)meshopt_simplify(indices.Get(), srcMesh->Indices.Get(), srcMeshIndexCount, (const float*)srcMesh->Positions.Get(), srcMeshVertexCount, sizeof(Float3), dstMeshIndexCountTarget, options.LODTargetError);
                indices.Resize(dstMeshIndexCount);
                if (dstMeshIndexCount == 0)
                    continue;

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

            // Remove empty meshes
            for (int32 i = dstLod.Meshes.Count() - 1; i >= 0; i--)
            {
                if (dstLod.Meshes[i]->Indices.IsEmpty())
                    dstLod.Meshes.RemoveAt(i--);
            }

            LOG(Info, "Generated LOD{0}: triangles: {1} ({2}% of base LOD), verticies: {3} ({4}% of base LOD)",
                lodIndex,
                lodTriangleCount, (int32)(lodTriangleCount * 100 / baseLodTriangleCount),
                lodVertexCount, (int32)(lodVertexCount * 100 / baseLodVertexCount));
        }
        if (generatedLod)
        {
            auto lodEndTime = DateTime::NowUTC();
            LOG(Info, "Generated LODs for {1} meshes in {0} ms", static_cast<int32>((lodEndTime - lodStartTime).GetTotalMilliseconds()), generatedLod);
        }
    }

    // Export imported data to the output container (we reduce vertex data copy operations to minimum)
    {
        meshData.Textures.Swap(data.Textures);
        meshData.Materials.Swap(data.Materials);
        meshData.LODs.Resize(data.LODs.Count(), false);
        for (int32 i = 0; i < data.LODs.Count(); i++)
        {
            auto& dst = meshData.LODs[i];
            auto& src = data.LODs[i];

            dst.Meshes = src.Meshes;
        }
        meshData.Skeleton.Swap(data.Skeleton);
        meshData.Animation.Swap(data.Animation);

        // Clear meshes from imported data (we link them to result model data). This reduces amount of allocations.
        data.LODs.Resize(0);
    }

    // Calculate blend shapes vertices ranges
    for (auto& lod : meshData.LODs)
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
