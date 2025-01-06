// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Models/BlendShape.h"
#include "Engine/Graphics/Shaders/GPUVertexLayout.h"

/// <summary>
/// Skinned Model Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class SkinnedModelAssetUpgrader : public BinaryAssetUpgrader
{
public:
    SkinnedModelAssetUpgrader()
    {
        const Upgrader upgraders[] =
        {
            { 5, 30, Upgrade_5_To_30 }, // [Deprecated in v1.10]
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    static bool Upgrade_5_To_30(AssetMigrationContext& context)
    {
        // [Deprecated in v1.10]
        ASSERT(context.Input.SerializedVersion == 5 && context.Output.SerializedVersion == 30);
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
            byte headerVersionOld;
            stream.Read(headerVersionOld);
            CHECK_RETURN(headerVersionOld == 1, true);
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
            CHECK_RETURN(materialSlotsCount <= 6, true);
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
                    uint16 blendShapesCount;
                    stream.Read(blendShapesCount);
                    output.Write(blendShapesCount);
                    for (int32 blendShapeIndex = 0; blendShapeIndex < blendShapesCount; blendShapeIndex++)
                    {
                        String name;
                        stream.Read(name, 13);
                        output.Write(name, 13);
                        float weight;
                        stream.Read(weight);
                        output.Write(weight);
                    }
                }
            }

            // Skeleton
            {
                int32 nodesCount;
                stream.Read(nodesCount);
                output.Write(nodesCount);
                if (nodesCount < 0)
                    return true;
                for (int32 i = 0 ; i < nodesCount; i++)
                {
                    int32 parentIndex;
                    Transform localTransform;
                    String name;
                    stream.Read(parentIndex);
                    output.Write(parentIndex);
                    stream.Read(localTransform);
                    output.Write(localTransform);
                    stream.Read(name, 71);
                    output.Write(name, 71);
                }

                int32 bonesCount;
                stream.Read(bonesCount);
                output.Write(bonesCount);
                if (bonesCount < 0)
                    return true;
                for (int32 i = 0 ; i < bonesCount; i++)
                {
                    int32 parentIndex, nodeIndex;
                    Transform localTransform;
                    String name;
                    Matrix offsetMatrix;
                    stream.Read(parentIndex);
                    output.Write(parentIndex);
                    stream.Read(nodeIndex);
                    output.Write(nodeIndex);
                    stream.Read(localTransform);
                    output.Write(localTransform);
                    stream.Read(offsetMatrix);
                    output.Write(offsetMatrix);
                }
            }

            // Retargeting
            {
                int32 entriesCount;
                stream.Read(entriesCount);
                output.Write(entriesCount);
                for (int32 i = 0 ; i < entriesCount; i++)
                {
                    Guid sourceAsset, skeletonAsset;
                    Dictionary<String, String> nodesMapping;
                    stream.Read(sourceAsset);
                    output.Write(sourceAsset);
                    stream.Read(skeletonAsset);
                    output.Write(skeletonAsset);
                    stream.Read(nodesMapping);
                    output.Write(nodesMapping);
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
            byte headerVersionOld;
            stream.Read(headerVersionOld);
            CHECK_RETURN(headerVersionOld == 1, true);
            output.Write(meshVersion);
            for (int32 meshIndex = 0; meshIndex < meshesCountPerLod[lodIndex]; meshIndex++)
            {
                // Descriptor
                uint32 vertices, triangles;
                stream.Read(vertices);
                stream.Read(triangles);
                output.Write(vertices);
                output.Write(triangles);
                uint16 blendShapesCount;
                stream.Read(blendShapesCount);
                Array<BlendShape> blendShapes;
                blendShapes.Resize(blendShapesCount);
                for (auto& blendShape : blendShapes)
                    blendShape.Load(stream, meshVersion);
                auto vb0 = stream.Move<VB0SkinnedElementType2>(vertices);
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                auto ibData = stream.Move(indicesCount * ibStride);
                output.Write((byte)1);
                output.Write(VB0SkinnedElementType2::GetLayout()->GetElements());

                // Buffers
                output.WriteBytes(vb0, vertices * sizeof(VB0SkinnedElementType2));
                output.WriteBytes(ibData, indicesCount * ibStride);

                // Blend Shapes
                output.Write(blendShapesCount);
                for (auto& blendShape : blendShapes)
                    blendShape.Save(output);
            }

            if (context.AllocateChunk(chunkIndex))
                return true;
            context.Output.Header.Chunks[chunkIndex]->Data.Copy(output.GetHandle(), output.GetPosition());
        }

        return false;
    }
PRAGMA_ENABLE_DEPRECATION_WARNINGS
};

#endif
