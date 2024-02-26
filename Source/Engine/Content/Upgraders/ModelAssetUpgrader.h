// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Core/Core.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/Asset.h"

/// <summary>
/// Model Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class ModelAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ModelAssetUpgrader"/> class.
    /// </summary>
    ModelAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 24, 25, &Upgrade_With_Repack }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
            { 23, 24, &Upgrade_22OrNewer_To_Newest }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
            { 22, 24, &Upgrade_22OrNewer_To_Newest }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
            { 1, 24, &Upgrade_Old_To_Newest }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    // ============================================
    //                  Version 25:
    // The same as version 24 except Vertex Buffer 1 has `Color32 Color` component per vertex added
    // ============================================
    //                  Version 24:
    // Designed: 9/24/2017
    // Custom Data: not used
    // Chunk 0:
    //  - Min Screen Size: float
    //  - Amount of material slots: int32
    //  - For each material slot:
    //     - Material: Guid
    //     - Shadows Mode: byte
    //     - Name: String (offset: 11)
    //  - Amount of LODs: byte
    //  - For each LOD:
    //     - Screen Size: float
    //     - Amount of meshes: uint16
    //     - For each mesh:
    //        - Material Slot index: int32
    //        - Box: BoundingBox
    //        - Sphere: BoundingSphere
    //        - HasLightmapUVs: bool
    // Next chunks: (the highest LOD is at chunk no. 1, followed by the lower LODs:
    //  - For each mesh in the LOD:
    //     - Vertices Count: uint32
    //     - Triangles Count: uint32
    //     - Vertex Buffer 0: byte[]
    //         - For each vertex
    //            - Position  : R32G32B32_Float
    //     - Vertex Buffer 1: byte[]
    //         - For each vertex
    //            - TexCoord   : R16G16_Float
    //            - Normal     : R10G10B10A2_UNorm
    //            - Tangent    : R10G10B10A2_UNorm
    //            - LightampUV : R16G16_Float
    //     - Index Buffer: uint16[] or uint32[] (based on amount of triangles)
    // ============================================
    //                  Version 23:
    // Designed: 9/18/2017
    // Custom Data: not used
    // Chunk 0:
    //  - Amount of material slots: int32
    //  - For each material slot:
    //     - Material: Guid
    //     - Shadows Mode: byte
    //     - Name: String (offset: 11)
    //  - Amount of LODs: byte
    //  - For each LOD:
    //     - Amount of meshes: uint16
    //     - For each mesh:
    //        - Material Slot index: int32
    //        - Box: BoundingBox
    //        - Sphere: BoundingSphere
    // Next chunks: (the highest LOD is at chunk no. 1, followed by the lower LODs:
    //  - For each mesh in the LOD:
    //     - Vertices Count: uint32
    //     - Triangles Count: uint32
    //     - Vertex Buffer 0: byte[]
    //         - For each vertex
    //            - Position  : R32G32B32_Float
    //     - Vertex Buffer 1: byte[]
    //         - For each vertex
    //            - TexCoord   : R16G16_Float
    //            - Normal     : R10G10B10A2_UNorm
    //            - Tangent    : R10G10B10A2_UNorm
    //            - LightampUV : R16G16_Float
    //     - Index Buffer: uint16[] or uint32[] (based on amount of triangles)
    // ============================================
    //                  Version 22:
    // Designed: 4/24/2017
    // Custom Data: not used
    // Chunk 0:
    //  - Amount of LODs: byte
    //  - For each LOD:
    //     - Amount of meshes: uint16
    //     - For each mesh:
    //        - Name: String (offset: 7)
    //        - Force Two Sided: bool
    //        - Cast Shadows: bool
    //        - Material ID: Guid
    //        - Box: BoundingBox
    //        - Sphere: BoundingSphere
    // Next chunks: (the highest LOD is at chunk no. 1, followed by the lower LODs:
    //  - For each mesh in the LOD:
    //     - Vertices Count: uint32
    //     - Triangles Count: uint32
    //     - Vertex Buffer 0: byte[]
    //         - For each vertex
    //            - Position  : R32G32B32_Float
    //     - Vertex Buffer 1: byte[]
    //         - For each vertex
    //            - TexCoord   : R16G16_Float
    //            - Normal     : R10G10B10A2_UNorm
    //            - Tangent    : R10G10B10A2_UNorm
    //            - LightampUV : R16G16_Float
    //     - Index Buffer: uint16[] or uint32[] (based on amount of triangles)
    // ============================================

    static Asset::LoadResult LoadOld(AssetMigrationContext& context, int32 version, ReadStream& headerStream, ModelData& modelData)
    {
        // Load version
        Asset::LoadResult result;
        switch (version)
        {
        case 25:
            result = loadVersion25(context, &headerStream, &modelData);
            break;
        case 24:
            result = loadVersion24(context, &headerStream, &modelData);
            break;
        case 23:
            result = loadVersion23(context, &headerStream, &modelData);
            break;
        case 22:
            result = loadVersion22(context, &headerStream, &modelData);
            break;
        case 20:
            result = loadVersion20(context, &headerStream, &modelData);
            break;
        case 19:
            result = loadVersion19(context, &headerStream, &modelData);
            break;
        case 18:
            result = loadVersion18(context, &headerStream, &modelData);
            break;
        case 17:
            result = loadVersion17(context, &headerStream, &modelData);
            break;
        case 16:
            result = loadVersion16(context, &headerStream, &modelData);
            break;
        case 15:
            result = loadVersion15(context, &headerStream, &modelData);
            break;
        default:
            LOG(Warning, "Unsupported model data version.");
            result = Asset::LoadResult::InvalidData;
            break;
        }

        return result;
    }

    static Asset::LoadResult loadVersion25(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Min Screen Size
        headerStream->ReadFloat(&data->MinScreenSize);

        // Amount of material slots
        int32 materialSlotsCount;
        headerStream->ReadInt32(&materialSlotsCount);
        data->Materials.Resize(materialSlotsCount, false);

        // For each material slot
        for (int32 i = 0; i < materialSlotsCount; i++)
        {
            auto& slot = data->Materials[i];

            // Material
            headerStream->Read(slot.AssetID);

            // Shadows Mode
            slot.ShadowsMode = static_cast<ShadowsCastingMode>(headerStream->ReadByte());

            // Name
            headerStream->ReadString(&slot.Name, 11);
        }

        // Amount of LODs
        const int32 lodsCount = headerStream->ReadByte();
        data->LODs.Resize(lodsCount, false);

        // For each LOD
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            auto& lod = data->LODs[lodIndex];

            // Screen Size
            headerStream->ReadFloat(&lod.ScreenSize);

            // Amount of meshes (and material slots for older formats)
            uint16 meshesCount;
            headerStream->ReadUint16(&meshesCount);
            lod.Meshes.Resize(meshesCount);
            lod.Meshes.SetAll(nullptr);

            // Get meshes data
            {
                auto lodData = context.Input.Header.Chunks[1 + lodIndex];
                MemoryReadStream stream(lodData->Get(), lodData->Size());

                // Load LOD for each mesh
                for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    // Load mesh data
                    uint32 vertices;
                    stream.ReadUint32(&vertices);
                    uint32 triangles;
                    stream.ReadUint32(&triangles);
                    if (vertices == 0 || triangles == 0)
                        return Asset::LoadResult::InvalidData;

                    // Vertex buffers
                    auto vb0 = stream.Move<VB0ElementType18>(vertices);
                    auto vb1 = stream.Move<VB1ElementType18>(vertices);
                    bool hasColors = stream.ReadBool();
                    VB2ElementType18* vb2 = nullptr;
                    if (hasColors)
                    {
                        vb2 = stream.Move<VB2ElementType18>(vertices);
                    }

                    // Index Buffer
                    uint32 indicesCount = triangles * 3;
                    bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                    auto ib = stream.Move<byte>(indicesCount * ibStride);

                    // Allocate mesh
                    lod.Meshes[meshIndex] = New<MeshData>();
                    auto& mesh = *lod.Meshes[meshIndex];

                    // Copy data
                    mesh.InitFromModelVertices(vb0, vb1, vb2, vertices);
                    mesh.SetIndexBuffer(ib, indicesCount);
                }
            }

            // For each mesh
            for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
            {
                auto& mesh = *lod.Meshes[meshIndex];

                // Material Slot index
                int32 materialSlotIndex;
                headerStream->ReadInt32(&materialSlotIndex);
                if (materialSlotIndex < 0 || materialSlotIndex >= materialSlotsCount)
                {
                    LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, materialSlotsCount);
                    return Asset::LoadResult::InvalidData;
                }

                // Box
                BoundingBox box;
                headerStream->Read(box);

                // Sphere
                BoundingSphere sphere;
                headerStream->Read(sphere);

                // Has Lightmap UVs
                bool hasLightmapUVs = headerStream->ReadBool();
                if (!hasLightmapUVs)
                    mesh.LightmapUVs.Resize(0);
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion24(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Min Screen Size
        headerStream->ReadFloat(&data->MinScreenSize);

        // Amount of material slots
        int32 materialSlotsCount;
        headerStream->ReadInt32(&materialSlotsCount);
        data->Materials.Resize(materialSlotsCount, false);

        // For each material slot
        for (int32 i = 0; i < materialSlotsCount; i++)
        {
            auto& slot = data->Materials[i];

            // Material
            headerStream->Read(slot.AssetID);

            // Shadows Mode
            slot.ShadowsMode = static_cast<ShadowsCastingMode>(headerStream->ReadByte());

            // Name
            headerStream->ReadString(&slot.Name, 11);
        }

        // Amount of LODs
        const int32 lodsCount = headerStream->ReadByte();
        data->LODs.Resize(lodsCount, false);

        // For each LOD
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            auto& lod = data->LODs[lodIndex];

            // Screen Size
            headerStream->ReadFloat(&lod.ScreenSize);

            // Amount of meshes (and material slots for older formats)
            uint16 meshesCount;
            headerStream->ReadUint16(&meshesCount);
            lod.Meshes.Resize(meshesCount);
            lod.Meshes.SetAll(nullptr);

            // Get meshes data
            {
                auto lodData = context.Input.Header.Chunks[1 + lodIndex];
                MemoryReadStream stream(lodData->Get(), lodData->Size());

                // Load LOD for each mesh
                for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    // Load mesh data
                    uint32 vertices;
                    stream.ReadUint32(&vertices);
                    uint32 triangles;
                    stream.ReadUint32(&triangles);
                    if (vertices == 0 || triangles == 0)
                        return Asset::LoadResult::InvalidData;

                    // Vertex buffers
                    auto vb0 = stream.Move<VB0ElementType18>(vertices);
                    auto vb1 = stream.Move<VB1ElementType18>(vertices);

                    // Index Buffer
                    uint32 indicesCount = triangles * 3;
                    bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                    auto ib = stream.Move<byte>(indicesCount * ibStride);

                    // Allocate mesh
                    lod.Meshes[meshIndex] = New<MeshData>();
                    auto& mesh = *lod.Meshes[meshIndex];

                    // Copy data
                    mesh.InitFromModelVertices(vb0, vb1, vertices);
                    mesh.SetIndexBuffer(ib, indicesCount);
                }
            }

            // For each mesh
            for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
            {
                auto& mesh = *lod.Meshes[meshIndex];

                // Material Slot index
                int32 materialSlotIndex;
                headerStream->ReadInt32(&materialSlotIndex);
                if (materialSlotIndex < 0 || materialSlotIndex >= materialSlotsCount)
                {
                    LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, materialSlotsCount);
                    return Asset::LoadResult::InvalidData;
                }

                // Box
                BoundingBox box;
                headerStream->Read(box);

                // Sphere
                BoundingSphere sphere;
                headerStream->Read(sphere);

                // Has Lightmap UVs
                bool hasLightmapUVs = headerStream->ReadBool();
                if (!hasLightmapUVs)
                    mesh.LightmapUVs.Resize(0);
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion23(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of material slots
        int32 materialSlotsCount;
        headerStream->ReadInt32(&materialSlotsCount);
        data->Materials.Resize(materialSlotsCount, false);

        // For each material slot
        for (int32 i = 0; i < materialSlotsCount; i++)
        {
            auto& slot = data->Materials[i];

            // Material
            headerStream->Read(slot.AssetID);

            // Shadows Mode
            slot.ShadowsMode = static_cast<ShadowsCastingMode>(headerStream->ReadByte());

            // Name
            headerStream->ReadString(&slot.Name, 11);
        }

        // Amount of LODs
        const int32 lodsCount = headerStream->ReadByte();
        data->LODs.Resize(lodsCount, false);

        // For each LOD
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            // Amount of meshes (and material slots for older formats)
            uint16 meshesCount;
            headerStream->ReadUint16(&meshesCount);
            data->LODs[lodIndex].Meshes.Resize(meshesCount);
            data->LODs[lodIndex].Meshes.SetAll(nullptr);

            // For each mesh
            for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
            {
                data->LODs[lodIndex].Meshes[meshIndex] = New<MeshData>();
                auto& meshEntry = *data->LODs[lodIndex].Meshes[meshIndex];

                // Material Slot index
                int32 materialSlotIndex;
                headerStream->ReadInt32(&materialSlotIndex);
                if (materialSlotIndex < 0 || materialSlotIndex >= materialSlotsCount)
                {
                    LOG(Warning, "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, materialSlotsCount);
                    return Asset::LoadResult::InvalidData;
                }
                meshEntry.MaterialSlotIndex = materialSlotIndex;

                // Box
                BoundingBox box;
                headerStream->Read(box);

                // Sphere
                BoundingSphere sphere;
                headerStream->Read(sphere);
            }

            // Get meshes data
            {
                auto lodData = context.Input.Header.Chunks[1 + lodIndex];
                MemoryReadStream stream(lodData->Get(), lodData->Size());

                // Load LOD for each mesh
                for (int32 i = 0; i < meshesCount; i++)
                {
                    // Load mesh data
                    uint32 vertices;
                    stream.ReadUint32(&vertices);
                    uint32 triangles;
                    stream.ReadUint32(&triangles);
                    if (vertices == 0 || triangles == 0)
                        return Asset::LoadResult::InvalidData;

                    // Vertex buffers
                    auto vb0 = stream.Move<VB0ElementType18>(vertices);
                    auto vb1 = stream.Move<VB1ElementType18>(vertices);

                    // Index Buffer
                    uint32 indicesCount = triangles * 3;
                    bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                    uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                    auto ib = stream.Move<byte>(indicesCount * ibStride);

                    // Copy data
                    auto& mesh = *data->LODs[lodIndex].Meshes[i];
                    mesh.InitFromModelVertices(vb0, vb1, vertices);
                    mesh.SetIndexBuffer(ib, indicesCount);
                }
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion22(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of LODs
        const int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;
        data->LODs.Resize(lodsCount, false);

        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Conversion from older format: 1-1 material slot - mesh mapping, new format allows to use the same material slot for many meshes across different lods
            meshEntry.MaterialSlotIndex = i;

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Force Two Sided
            headerStream->ReadBool();

            // Cast Shadows
            const bool castShadows = headerStream->ReadBool();
            slot.ShadowsMode = castShadows ? ShadowsCastingMode::All : ShadowsCastingMode::None;

            // Default material ID
            headerStream->Read(slot.AssetID);

            // Box
            BoundingBox box;
            headerStream->Read(box);

            // Sphere
            BoundingSphere sphere;
            headerStream->Read(sphere);
        }

        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffers
                auto vb0 = stream.Move<VB0ElementType18>(vertices);
                auto vb1 = stream.Move<VB1ElementType18>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb0, vb1, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion20(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs.Resize(1, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // Amount of LODs
        int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Force Two Sided
            headerStream->ReadBool();

            // Default material ID
            headerStream->Read(slot.AssetID);

            // Box
            BoundingBox box;
            headerStream->Read(box);

            // Sphere
            BoundingSphere sphere;
            headerStream->Read(sphere);
        }

        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffers
                auto vb0 = stream.Move<VB0ElementType18>(vertices);
                auto vb1 = stream.Move<VB1ElementType18>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb0, vb1, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion19(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs.Resize(1, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // Amount of LODs
        int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Force Two Sided
            headerStream->ReadBool();

            // Default material ID
            headerStream->Read(slot.AssetID);

            // Box
            BoundingBox box;
            headerStream->Read(box);

            // Sphere
            BoundingSphere sphere;
            headerStream->Read(sphere);
        }

        // Load all LODs
        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffers
                auto vb0 = stream.Move<VB0ElementType18>(vertices);
                auto vb1 = stream.Move<VB1ElementType18>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb0, vb1, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion18(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs.Resize(1, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // Amount of LODs
        int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;

        // Options version code
        uint32 optionsVersionCode;
        headerStream->ReadUint32(&optionsVersionCode);

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Local Transform
            Transform transform;
            headerStream->Read(transform);

            // Force Two Sided
            headerStream->ReadBool();

            // Default material ID
            headerStream->Read(slot.AssetID);

            // Corners
            BoundingBox box;
            headerStream->Read(box);
        }

        // Load all LODs
        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffers
                auto vb0 = stream.Move<VB0ElementType18>(vertices);
                auto vb1 = stream.Move<VB1ElementType18>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb0, vb1, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion17(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs.Resize(1, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // Amount of LODs
        int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;

        // Options version code
        uint32 optionsVersionCode;
        headerStream->ReadUint32(&optionsVersionCode);

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Local Transform
            Transform transform;
            headerStream->Read(transform);

            // Force Two Sided
            headerStream->ReadBool();

            // Default material ID
            headerStream->Read(slot.AssetID);

            // Corners
            Vector3 corner;
            for (int32 cornerIndex = 0; cornerIndex < 8; cornerIndex++)
                headerStream->Read(corner);
        }

        // Load all LODs
        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffers
                auto vb0 = stream.Move<VB0ElementType15>(vertices);
                auto vb1 = stream.Move<VB1ElementType15>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb0, vb1, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);

#if COMPILE_WITH_MODEL_TOOL
                if (mesh.GenerateLightmapUVs())
                    return Asset::LoadResult::Failed;
#endif
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion16(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs.Resize(1, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // Amount of LODs
        int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;

        // Options version code
        uint32 optionsVersionCode;
        headerStream->ReadUint32(&optionsVersionCode);

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Local Transform
            Transform transform;
            headerStream->Read(transform);

            // Force Two Sided
            headerStream->ReadBool();

            // Default material ID
            headerStream->Read(slot.AssetID);
        }

        // Load all LODs
        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffers
                auto vb0 = stream.Move<VB0ElementType15>(vertices);
                auto vb1 = stream.Move<VB1ElementType15>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb0, vb1, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);

#if COMPILE_WITH_MODEL_TOOL
                if (mesh.GenerateLightmapUVs())
                    return Asset::LoadResult::Failed;
#endif
            }
        }

        return Asset::LoadResult::Ok;
    }

    static Asset::LoadResult loadVersion15(AssetMigrationContext& context, ReadStream* headerStream, ModelData* data)
    {
        // Amount of meshes (and material slots for older formats)
        uint16 meshesCount;
        headerStream->ReadUint16(&meshesCount);
        data->Materials.Resize(meshesCount, false);
        data->LODs.Resize(1, false);
        data->LODs[0].Meshes.Resize(meshesCount);

        // Amount of LODs
        int32 lodsCount = headerStream->ReadByte();
        if (lodsCount != 1)
            return Asset::LoadResult::InvalidData;

        // Options version code
        uint32 optionsVersionCode;
        headerStream->ReadUint32(&optionsVersionCode);

        // For each mesh
        for (int32 i = 0; i < meshesCount; i++)
        {
            data->LODs[0].Meshes[i] = New<MeshData>();
            auto& meshEntry = *data->LODs[0].Meshes[i];
            auto& slot = data->Materials[i];

            // Name
            headerStream->ReadString(&slot.Name, 7);

            // Local Transform
            Transform transform;
            headerStream->Read(transform);

            // Force Two Sided
            headerStream->ReadBool();

            // Default material ID
            headerStream->Read(slot.AssetID);
        }

        // Load all LODs
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            // Get data
            auto lodData = context.Input.Header.Chunks[1];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            // Load LOD for each mesh
            for (int32 i = 0; i < meshesCount; i++)
            {
                // Load mesh data
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0)
                    return Asset::LoadResult::InvalidData;

                // Vertex buffer
                auto vb = stream.Move<ModelVertex15>(vertices);

                // Index Buffer
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                // Copy data
                auto& mesh = *data->LODs[0].Meshes[i];
                mesh.InitFromModelVertices(vb, vertices);
                mesh.SetIndexBuffer(ib, indicesCount);

#if COMPILE_WITH_MODEL_TOOL
                if (mesh.GenerateLightmapUVs())
                    return Asset::LoadResult::Failed;
#endif
            }
        }

        return Asset::LoadResult::Ok;
    }

    static bool Upgrade_22OrNewer_To_Newest(AssetMigrationContext& context)
    {
        // Get header chunk
        auto chunk0 = context.Input.Header.Chunks[0];
        if (chunk0 == nullptr || chunk0->IsMissing())
            return true;
        MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());

        // Load model data from the older format
        ModelData modelData;
        auto result = LoadOld(context, context.Input.SerializedVersion, headerStream, modelData);
        if (result != Asset::LoadResult::Ok)
        {
            LOG(Warning, "Model old-format data conversion failed.");
            return true;
        }

        // Pack model header
        {
            if (context.AllocateChunk(0))
                return true;

            MemoryWriteStream newHeaderStream(512);
            if (modelData.Pack2ModelHeader(&newHeaderStream))
                return true;
            context.Output.Header.Chunks[0]->Data.Copy(newHeaderStream.GetHandle(), newHeaderStream.GetPosition());
        }

        // Copy LODs
        for (int32 lodIndex = 0; lodIndex < modelData.LODs.Count(); lodIndex++)
        {
            int32 chunkIndex = lodIndex + 1;
            auto srcLodData = context.Input.Header.Chunks[chunkIndex];
            if (srcLodData == nullptr || srcLodData->IsMissing())
            {
                LOG(Warning, "Missing model LOD data chunk");
                return true;
            }
            if (context.AllocateChunk(chunkIndex))
                return true;
            context.Output.Header.Chunks[chunkIndex]->Data.Copy(context.Input.Header.Chunks[chunkIndex]->Data);
        }

        return false;
    }

    static bool Upgrade_With_Repack(AssetMigrationContext& context)
    {
        // Get header chunk
        auto chunk0 = context.Input.Header.Chunks[0];
        if (chunk0 == nullptr || chunk0->IsMissing())
            return true;
        MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());

        // Load model data from the older format
        ModelData modelData;
        auto result = LoadOld(context, context.Input.SerializedVersion, headerStream, modelData);
        if (result != Asset::LoadResult::Ok)
        {
            LOG(Warning, "Model old-format data conversion failed.");
            return true;
        }

        // Pack model header
        {
            if (context.AllocateChunk(0))
                return true;

            MemoryWriteStream newHeaderStream(512);
            if (modelData.Pack2ModelHeader(&newHeaderStream))
                return true;
            context.Output.Header.Chunks[0]->Data.Copy(newHeaderStream.GetHandle(), newHeaderStream.GetPosition());
        }

        // Pack model LODs data
        MemoryWriteStream stream(4095);
        const auto lodsCount = modelData.GetLODsCount();
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            stream.SetPosition(0);

            // Pack meshes
            auto& meshes = modelData.LODs[lodIndex].Meshes;
            for (int32 meshIndex = 0; meshIndex < meshes.Count(); meshIndex++)
            {
                if (meshes[meshIndex]->Pack2Model(&stream))
                {
                    LOG(Warning, "Cannot pack mesh.");
                    return true;
                }
            }

            const int32 chunkIndex = lodIndex + 1;
            if (context.AllocateChunk(chunkIndex))
                return true;
            context.Output.Header.Chunks[chunkIndex]->Data.Copy(stream.GetHandle(), stream.GetPosition());
        }

        return false;
    }

    static bool Upgrade_Old_To_Newest(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1);

        // Get header chunk
        auto chunk0 = context.Input.Header.Chunks[0];
        if (chunk0 == nullptr || chunk0->IsMissing())
            return true;
        MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());

        // Load header entry
        int32 magicCode;
        headerStream.ReadInt32(&magicCode);
        if (magicCode != -842185139)
        {
            LOG(Warning, "Invalid header data.");
            return true;
        }
        int32 version;
        headerStream.ReadInt32(&version);

        // Load model data from the older format
        ModelData modelData;
        auto result = LoadOld(context, version, headerStream, modelData);
        if (result != Asset::LoadResult::Ok)
        {
            LOG(Warning, "Model old-format data conversion failed.");
            return true;
        }

        // Pack model header
        {
            if (context.AllocateChunk(0))
                return true;

            MemoryWriteStream newHeaderStream(512);
            if (modelData.Pack2ModelHeader(&newHeaderStream))
                return true;
            context.Output.Header.Chunks[0]->Data.Copy(newHeaderStream.GetHandle(), newHeaderStream.GetPosition());
        }

        // Copy LOD (older versons were using just a single LOD)
        {
            auto srcLodData = context.Input.Header.Chunks[1];
            if (srcLodData == nullptr || srcLodData->IsMissing())
            {
                LOG(Warning, "Missing model LOD data chunk");
                return true;
            }
            if (context.AllocateChunk(1))
                return true;
            context.Output.Header.Chunks[1]->Data.Copy(context.Input.Header.Chunks[1]->Data);
        }

        return false;
    }
};

#endif
