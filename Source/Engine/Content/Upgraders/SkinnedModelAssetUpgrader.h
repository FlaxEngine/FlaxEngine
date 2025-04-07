// Copyright (c) Wojciech Figat. All rights reserved.

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
            { 4, 5, &Upgrade_4_To_5 }, // [Deprecated in v1.10]
            { 5, 30, Upgrade_5_To_30 }, // [Deprecated in v1.10]
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

PRAGMA_DISABLE_DEPRECATION_WARNINGS
    static bool Upgrade_4_To_5(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 4 && context.Output.SerializedVersion == 5);

        // Changes:
        // - added version number to header (for easier changes in future)
        // - added version number to mesh data (for easier changes in future)
        // - added skeleton retarget setups to header

        // Rewrite header chunk (added header version and retarget entries)
        byte lodCount;
        Array<uint16> meshesCounts;
        {
            const auto srcData = context.Input.Header.Chunks[0];
            if (srcData == nullptr || srcData->IsMissing())
            {
                LOG(Warning, "Missing model header chunk");
                return true;
            }
            MemoryReadStream stream(srcData->Get(), srcData->Size());
            MemoryWriteStream output(srcData->Size());

            // Header Version
            output.WriteByte(1);

            // Min Screen Size
            float minScreenSize;
            stream.ReadFloat(&minScreenSize);
            output.WriteFloat(minScreenSize);

            // Amount of material slots
            int32 materialSlotsCount;
            stream.ReadInt32(&materialSlotsCount);
            output.WriteInt32(materialSlotsCount);

            // For each material slot
            for (int32 materialSlotIndex = 0; materialSlotIndex < materialSlotsCount; materialSlotIndex++)
            {
                // Material
                Guid materialId;
                stream.Read(materialId);
                output.Write(materialId);

                // Shadows Mode
                output.WriteByte(stream.ReadByte());

                // Name
                String name;
                stream.ReadString(&name, 11);
                output.WriteString(name, 11);
            }

            // Amount of LODs
            stream.ReadByte(&lodCount);
            output.WriteByte(lodCount);
            meshesCounts.Resize(lodCount);

            // For each LOD
            for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
            {
                // Screen Size
                float screenSize;
                stream.ReadFloat(&screenSize);
                output.WriteFloat(screenSize);

                // Amount of meshes
                uint16 meshesCount;
                stream.ReadUint16(&meshesCount);
                output.WriteUint16(meshesCount);
                meshesCounts[lodIndex] = meshesCount;

                // For each mesh
                for (uint16 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    // Material Slot index
                    int32 materialSlotIndex;
                    stream.ReadInt32(&materialSlotIndex);
                    output.WriteInt32(materialSlotIndex);

                    // Box
                    BoundingBox box;
                    stream.Read(box);
                    output.Write(box);

                    // Sphere
                    BoundingSphere sphere;
                    stream.Read(sphere);
                    output.Write(sphere);

                    // Blend Shapes
                    uint16 blendShapes;
                    stream.ReadUint16(&blendShapes);
                    output.WriteUint16(blendShapes);
                    for (int32 blendShapeIndex = 0; blendShapeIndex < blendShapes; blendShapeIndex++)
                    {
                        String blendShapeName;
                        stream.ReadString(&blendShapeName, 13);
                        output.WriteString(blendShapeName, 13);
                        float blendShapeWeight;
                        stream.ReadFloat(&blendShapeWeight);
                        output.WriteFloat(blendShapeWeight);
                    }
                }
            }

            // Skeleton
            {
                int32 nodesCount;
                stream.ReadInt32(&nodesCount);
                output.WriteInt32(nodesCount);

                // For each node
                for (int32 nodeIndex = 0; nodeIndex < nodesCount; nodeIndex++)
                {
                    int32 parentIndex;
                    stream.ReadInt32(&parentIndex);
                    output.WriteInt32(parentIndex);

                    Transform localTransform;
                    stream.Read(localTransform);
                    output.Write(localTransform);

                    String name;
                    stream.ReadString(&name, 71);
                    output.WriteString(name, 71);
                }

                int32 bonesCount;
                stream.ReadInt32(&bonesCount);
                output.WriteInt32(bonesCount);

                // For each bone
                for (int32 boneIndex = 0; boneIndex < bonesCount; boneIndex++)
                {
                    int32 parentIndex;
                    stream.ReadInt32(&parentIndex);
                    output.WriteInt32(parentIndex);

                    int32 nodeIndex;
                    stream.ReadInt32(&nodeIndex);
                    output.WriteInt32(nodeIndex);

                    Transform localTransform;
                    stream.Read(localTransform);
                    output.Write(localTransform);

                    Matrix offsetMatrix;
                    stream.ReadBytes(&offsetMatrix, sizeof(Matrix));
                    output.WriteBytes(&offsetMatrix, sizeof(Matrix));
                }
            }
                    
            // Retargeting
            {
                output.WriteInt32(0);
            }

            // Save new data
            if (stream.GetPosition() != stream.GetLength())
            {
                LOG(Error, "Invalid position after upgrading skinned model header data.");
                return true;
            }
            if (context.AllocateChunk(0))
                return true;
            context.Output.Header.Chunks[0]->Data.Copy(output.GetHandle(), output.GetPosition());
        }

        // Rewrite meshes data chunks
        for (int32 lodIndex = 0; lodIndex < lodCount; lodIndex++)
        {
            const int32 chunkIndex = lodIndex + 1;
            const auto srcData = context.Input.Header.Chunks[chunkIndex];
            if (srcData == nullptr || srcData->IsMissing())
            {
                LOG(Warning, "Missing skinned model LOD meshes data chunk");
                return true;
            }
            MemoryReadStream stream(srcData->Get(), srcData->Size());
            MemoryWriteStream output(srcData->Size());

            // Mesh Data Version
            output.WriteByte(1);

            for (int32 meshIndex = 0; meshIndex < meshesCounts[lodIndex]; meshIndex++)
            {
                uint32 vertices;
                stream.ReadUint32(&vertices);
                output.WriteUint32(vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                output.WriteUint32(triangles);
                uint16 blendShapesCount;
                stream.ReadUint16(&blendShapesCount);
                output.WriteUint16(blendShapesCount);
                for (int32 blendShapeIndex = 0; blendShapeIndex < blendShapesCount; blendShapeIndex++)
                {
                    output.WriteBool(stream.ReadBool());
                    uint32 minVertexIndex, maxVertexIndex;
                    stream.ReadUint32(&minVertexIndex);
                    output.WriteUint32(minVertexIndex);
                    stream.ReadUint32(&maxVertexIndex);
                    output.WriteUint32(maxVertexIndex);
                    uint32 blendShapeVertices;
                    stream.ReadUint32(&blendShapeVertices);
                    output.WriteUint32(blendShapeVertices);
                    const uint32 blendShapeDataSize = blendShapeVertices * sizeof(BlendShapeVertex);
                    const auto blendShapeData = stream.Move<byte>(blendShapeDataSize);
                    output.WriteBytes(blendShapeData, blendShapeDataSize);
                }
                const uint32 indicesCount = triangles * 3;
                const bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
                const uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                if (vertices == 0 || triangles == 0)
                    return true;
                const auto vb0 = stream.Move<VB0SkinnedElementType>(vertices);
                output.WriteBytes(vb0, vertices * sizeof(VB0SkinnedElementType));
                const auto ib = stream.Move<byte>(indicesCount * ibStride);
                output.WriteBytes(ib, indicesCount * ibStride);
            }

            // Save new data
            if (stream.GetPosition() != stream.GetLength())
            {
                LOG(Error, "Invalid position after upgrading skinned model LOD meshes data.");
                return true;
            }
            if (context.AllocateChunk(chunkIndex))
                return true;
            context.Output.Header.Chunks[chunkIndex]->Data.Copy(output.GetHandle(), output.GetPosition());
        }

        return false;
    }
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
