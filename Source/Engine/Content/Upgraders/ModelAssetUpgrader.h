// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"

/// <summary>
/// Model Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class ModelAssetUpgrader : public BinaryAssetUpgrader
{
public:
    ModelAssetUpgrader()
    {
        Upgrader upgraders[] =
        {
            { 25, 30, Upgrade_25_To_30 }, // [Deprecated in v1.10]
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    static bool Upgrade_25_To_30(AssetMigrationContext& context)
    {
        // [Deprecated in v1.10]
        ASSERT(context.Input.SerializedVersion == 25 && context.Output.SerializedVersion == 30);
        MemoryWriteStream output;

        // Upgrade header
        byte lodsCount;
        Array<int32, FixedAllocation<6>> meshesCountPerLod;
        {
            if (context.AllocateChunk(0))
                return true;
            auto& data = context.Input.Header.Chunks[0]->Data;
            MemoryReadStream stream(data.Get(), data.Length());
            
            constexpr byte headerVersion = 2;
            output.Write(headerVersion);

            float minScreenSize;
            stream.Read(minScreenSize);
            output.Write(minScreenSize);
            
            // Materials
            int32 materialSlotsCount;
            stream.Read(materialSlotsCount);
            output.Write(materialSlotsCount);
            CHECK_RETURN(materialSlotsCount >= 0 && materialSlotsCount < 4096, true);
            for (int32 materialSlotIndex = 0; materialSlotIndex < materialSlotsCount; materialSlotIndex++)
            {
                Guid materialId;
                stream.Read(materialId);
                output.Write(materialId);
                byte shadowsCastingMode;
                stream.Read(shadowsCastingMode);
                output.Write(shadowsCastingMode);
                String name;
                stream.Read(name, 11);
                output.Write(name, 11);
            }

            // LODs
            stream.Read(lodsCount);
            output.Write(lodsCount);
            CHECK_RETURN(lodsCount <= 6, true);
            meshesCountPerLod.Resize(lodsCount);
            for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
            {
                float screenSize;
                stream.Read(screenSize);
                output.Write(screenSize);

                // Amount of meshes
                uint16 meshesCount;
                stream.Read(meshesCount);
                output.Write(meshesCount);
                CHECK_RETURN(meshesCount > 0 && meshesCount < 4096, true);
                meshesCountPerLod[lodIndex] = meshesCount;
                for (uint16 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    int32 materialSlotIndex;
                    stream.Read(materialSlotIndex);
                    output.Write(materialSlotIndex);
                    BoundingBox box;
                    stream.Read(box);
                    output.Write(box);
                    BoundingSphere sphere;
                    stream.Read(sphere);
                    output.Write(sphere);
                    bool hasLightmapUVs = stream.ReadBool();
                    int8 lightmapUVsIndex = hasLightmapUVs ? 1 : -1;
                    output.Write(lightmapUVsIndex);
                }
            }

            context.Output.Header.Chunks[0]->Data.Copy(output.GetHandle(), output.GetPosition());
        }

        // Upgrade meshes data
        for (int32 lodIndex = 0; lodIndex < lodsCount; lodIndex++)
        {
            output.SetPosition(0);
            const int32 chunkIndex = lodIndex + 1;
            const FlaxChunk* lodData = context.Input.Header.Chunks[chunkIndex];
            MemoryReadStream stream(lodData->Get(), lodData->Size());

            constexpr byte meshVersion = 2;
            output.Write(meshVersion);
            for (int32 meshIndex = 0; meshIndex < meshesCountPerLod[lodIndex]; meshIndex++)
            {
                // Descriptor
                uint32 vertices, triangles;
                stream.Read(vertices);
                stream.Read(triangles);
                output.Write(vertices);
                output.Write(triangles);
                auto vb0 = stream.Move<VB0ElementType18>(vertices);
                auto vb1 = stream.Move<VB1ElementType18>(vertices);
                bool hasColors = stream.ReadBool();
                VB2ElementType18* vb2 = nullptr;
                if (hasColors)
                    vb2 = stream.Move<VB2ElementType18>(vertices);
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ibData = stream.Move(indicesCount * ibStride);
                byte vbCount = hasColors ? 3 : 2;
                output.Write(vbCount);
                output.Write(VB0ElementType18::GetLayout()->GetElements());
                output.Write(VB1ElementType18::GetLayout()->GetElements());
                if (hasColors)
                    output.Write(VB2ElementType18::GetLayout()->GetElements());

                // Buffers
                output.WriteBytes(vb0, vertices * sizeof(VB0ElementType18));
                output.WriteBytes(vb1, vertices * sizeof(VB1ElementType18));
                if (hasColors)
                    output.WriteBytes(vb2, vertices * sizeof(VB2ElementType18));
                output.WriteBytes(ibData, indicesCount * ibStride);
            }

            if (context.AllocateChunk(chunkIndex))
                return true;
            context.Output.Header.Chunks[chunkIndex]->Data.Copy(output.GetHandle(), output.GetPosition());
        }

        // Copy SDF data
        return CopyChunk(context, 15);
    }
PRAGMA_ENABLE_DEPRECATION_WARNINGS
};

#endif
