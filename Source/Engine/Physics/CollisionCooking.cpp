// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PHYSICS_COOKING

#include "CollisionCooking.h"
#include "Engine/Threading/Task.h"
#include "Engine/Graphics/Async/GPUTask.h"
#include "Engine/Graphics/Models/MeshBase.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Core/Log.h"

bool CollisionCooking::CookCollision(const Argument& arg, CollisionData::SerializedOptions& outputOptions, BytesContainer& outputData)
{
    PROFILE_CPU();
    int32 convexVertexLimit = Math::Clamp(arg.ConvexVertexLimit, CONVEX_VERTEX_MIN, CONVEX_VERTEX_MAX);
    if (arg.ConvexVertexLimit == 0)
        convexVertexLimit = CONVEX_VERTEX_MAX;

    DataContainer<Float3> finalVertexData;
    DataContainer<uint32> finalIndexData;
    const bool needIndexBuffer = arg.Type == CollisionDataType::TriangleMesh;

    // Check if use custom model (specified in the argument, used for fast internal collision cooking by e.g. CSGBuilder)
    if (arg.OverrideModelData)
    {
        // Validate model data
        if (arg.OverrideModelData->LODs.IsEmpty())
        {
            LOG(Warning, "Missing model data.");
            return true;
        }

        // Pick a proper model LOD
        const int32 lodIndex = Math::Clamp(arg.ModelLodIndex, 0, arg.OverrideModelData->LODs.Count());
        auto lod = &arg.OverrideModelData->LODs[lodIndex];
        const int32 meshesCount = lod->Meshes.Count();

        // Count vertex/index buffer sizes
        int32 vCount = 0;
        int32 iCount = 0;
        for (int32 i = 0; i < meshesCount; i++)
        {
            const auto mesh = lod->Meshes[i];
            if ((arg.MaterialSlotsMask & (1 << mesh->MaterialSlotIndex)) == 0)
                continue;
            vCount += mesh->Positions.Count();
            if (needIndexBuffer)
                iCount += mesh->Indices.Count() * 3;
        }

        if (meshesCount == 1 && vCount != 0)
        {
            // Link a single mesh
            const auto mesh = lod->Meshes[0];
            finalVertexData.Link(mesh->Positions);
            if (needIndexBuffer)
            {
                finalIndexData.Link(mesh->Indices);
            }
        }
        else
        {
            // Combine meshes into one
            finalVertexData.Allocate(vCount);
            finalIndexData.Allocate(iCount);
            int32 vertexCounter = 0, indexCounter = 0;
            for (int32 i = 0; i < meshesCount; i++)
            {
                const auto mesh = lod->Meshes[i];
                if ((arg.MaterialSlotsMask & (1 << mesh->MaterialSlotIndex)) == 0)
                    continue;

                const int32 firstVertexIndex = vertexCounter;
                const int32 vertexCount = mesh->Positions.Count();
                Platform::MemoryCopy(finalVertexData.Get() + firstVertexIndex, mesh->Positions.Get(), vertexCount * sizeof(Float3));
                vertexCounter += vertexCount;

                if (needIndexBuffer)
                {
                    auto dst = finalIndexData.Get() + indexCounter;
                    auto src = mesh->Indices.Get();
                    for (int32 j = 0; j < mesh->Indices.Count(); j++)
                    {
                        *dst++ = firstVertexIndex + *src++;
                    }
                    indexCounter += mesh->Indices.Count();
                }
            }
        }
    }
    else
    {
        // Ensure model is loaded
        if (arg.Model == nullptr)
        {
            LOG(Warning, "Missing model.");
            return true;
        }
        if (arg.Model->WaitForLoaded())
        {
            LOG(Warning, "Model loading failed.");
            return true;
        }

        // Pick a proper model LOD
        const int32 lodIndex = Math::Clamp(arg.ModelLodIndex, 0, arg.Model->GetLODsCount());
        Array<MeshBase*> meshes;
        arg.Model->GetMeshes(meshes, lodIndex);

        // Get mesh data
        const int32 meshesCount = meshes.Count();
        Array<BytesContainer> vertexBuffers;
        Array<BytesContainer> indexBuffers;
        Array<int32> vertexCounts;
        Array<int32> indexCounts;
        vertexBuffers.Resize(meshesCount);
        vertexCounts.Resize(meshesCount);
        indexBuffers.Resize(needIndexBuffer ? meshesCount : 0);
        indexCounts.Resize(needIndexBuffer ? meshesCount : 0);
        vertexCounts.SetAll(0);
        indexCounts.SetAll(0);
        bool useCpuData = IsInMainThread() || !arg.Model->IsVirtual();
        if (useCpuData)
        {
            // Read directly from the asset storage
            for (int32 i = 0; i < meshesCount; i++)
            {
                const auto& mesh = *meshes[i];
                if ((arg.MaterialSlotsMask & (1 << mesh.GetMaterialSlotIndex())) == 0)
                    continue;
                if (mesh.GetVertexCount() == 0)
                    continue;

                int32 count;
                if (mesh.DownloadDataCPU(MeshBufferType::Vertex0, vertexBuffers[i], count))
                {
                    LOG(Error, "Failed to download mesh {0} data from model {1} LOD{2}", i, arg.Model.ToString(), lodIndex);
                    return true;
                }
                vertexCounts[i] = count;

                if (needIndexBuffer)
                {
                    if (mesh.DownloadDataCPU(MeshBufferType::Index, indexBuffers[i], count))
                    {
                        LOG(Error, "Failed to download mesh {0} data from model {1} LOD{2}", i, arg.Model.ToString(), lodIndex);
                        return true;
                    }
                    indexCounts[i] = count;
                }
            }
        }
        else
        {
            // Download model LOD data from the GPU.
            // It's easier than reading internal, versioned mesh storage format.
            // Also it works with virtual assets that have no dedicated storage.
            // Note: request all meshes data at once and wait for the tasks to be done.
            Array<Task*> tasks(meshesCount + 2);
            for (int32 i = 0; i < meshesCount; i++)
            {
                const auto& mesh = *meshes[i];
                if ((arg.MaterialSlotsMask & (1 << mesh.GetMaterialSlotIndex())) == 0)
                    continue;
                if (mesh.GetVertexCount() == 0)
                    continue;

                auto task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex0, vertexBuffers[i]);
                if (task == nullptr)
                {
                    LOG(Error, "Failed to download mesh {0} data from model {1} LOD{2}", i, arg.Model.ToString(), lodIndex);
                    return true;
                }
                int32 count = mesh.GetVertexCount();
                task->Start();
                tasks.Add(task);
                vertexCounts[i] = count;

                if (needIndexBuffer)
                {
                    task = mesh.DownloadDataGPUAsync(MeshBufferType::Index, indexBuffers[i]);
                    if (task == nullptr)
                    {
                        LOG(Error, "Failed to download mesh {0} data from model {1} LOD{2}", i, arg.Model.ToString(), lodIndex);
                        return true;
                    }
                    count = mesh.GetTriangleCount() * 3;
                    task->Start();
                    tasks.Add(task);
                    indexCounts[i] = count;
                }
            }
            if (Task::WaitAll(tasks))
                return true;
        }

        // Combine meshes into one
        int32 vCount = 0;
        for (int32 i = 0; i < vertexCounts.Count(); i++)
            vCount += vertexCounts[i];
        finalVertexData.Allocate(vCount);
        int32 iCount = 0;
        for (int32 i = 0; i < indexCounts.Count(); i++)
            iCount += indexCounts[i];
        finalIndexData.Allocate(iCount);
        int32 vertexCounter = 0, indexCounter = 0;
        for (int32 i = 0; i < meshesCount; i++)
        {
            const auto& mesh = *meshes[i];
            if ((arg.MaterialSlotsMask & (1 << mesh.GetMaterialSlotIndex())) == 0)
                continue;
            const auto& vData = vertexBuffers[i];

            const int32 firstVertexIndex = vertexCounter;
            const int32 vertexCount = vertexCounts[i];
            if (vertexCount == 0)
                continue;
            const int32 vStride = vData.Length() / vertexCount;
            if (vStride == sizeof(Float3))
                Platform::MemoryCopy(finalVertexData.Get() + firstVertexIndex, vData.Get(), vertexCount * sizeof(Float3));
            else
            {
                // This assumes that each vertex structure contains position as Float3 in the beginning
                auto dst = finalVertexData.Get() + firstVertexIndex;
                auto src = vData.Get();
                for (int32 j = 0; j < vertexCount; j++)
                {
                    *dst++ = *(Float3*)src;
                    src += vStride;
                
                }
            }
            vertexCounter += vertexCount;

            if (needIndexBuffer)
            {
                const auto& iData = indexBuffers[i];
                const int32 indexCount = indexCounts[i];
                if (iData.Length() / indexCount == sizeof(uint16))
                {
                    auto dst = finalIndexData.Get() + indexCounter;
                    auto src = iData.Get<uint16>();
                    for (int32 j = 0; j < indexCount; j++)
                    {
                        *dst++ = firstVertexIndex + *src++;
                    }
                    indexCounter += indexCount;
                }
                else
                {
                    auto dst = finalIndexData.Get() + indexCounter;
                    auto src = iData.Get<uint32>();
                    for (int32 j = 0; j < indexCount; j++)
                    {
                        *dst++ = firstVertexIndex + *src++;
                    }
                    indexCounter += indexCount;
                }
            }
        }
    }

    // Prepare cooking options
    CookingInput cookingInput;
    cookingInput.VertexCount = finalVertexData.Length();
    cookingInput.VertexData = finalVertexData.Get();
    cookingInput.IndexCount = finalIndexData.Length();
    cookingInput.IndexData = finalIndexData.Get();
    cookingInput.Is16bitIndexData = false;
    cookingInput.ConvexFlags = arg.ConvexFlags;
    cookingInput.ConvexVertexLimit = convexVertexLimit;

    // Cook!
    if (arg.Type == CollisionDataType::ConvexMesh)
    {
        if (CookConvexMesh(cookingInput, outputData))
            return true;
    }
    else if (arg.Type == CollisionDataType::TriangleMesh)
    {
        if (CookTriangleMesh(cookingInput, outputData))
            return true;
    }
    else
    {
        LOG(Warning, "Invalid collision data type.");
        return true;
    }

    // Setup options
    Platform::MemoryClear(&outputOptions, sizeof(outputOptions));
    outputOptions.Type = arg.Type;
    outputOptions.Model = arg.Model.GetID();
    outputOptions.ModelLodIndex = arg.ModelLodIndex;
    outputOptions.ConvexFlags = arg.ConvexFlags;
    outputOptions.ConvexVertexLimit = arg.ConvexVertexLimit;
    outputOptions.MaterialSlotsMask = arg.MaterialSlotsMask;

    return false;
}

#endif
