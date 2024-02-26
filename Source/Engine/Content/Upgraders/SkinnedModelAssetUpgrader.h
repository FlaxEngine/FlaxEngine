// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Graphics/Models/Types.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Matrix.h"
#include "Engine/Core/Math/Transform.h"

/// <summary>
/// Skinned Model Asset Upgrader
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class SkinnedModelAssetUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SkinnedModelAssetUpgrader"/> class.
    /// </summary>
    SkinnedModelAssetUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 2, &Upgrade_1_To_2 }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
            { 2, 3, &Upgrade_2_To_3 }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
            { 3, 4, &Upgrade_3_To_4 }, // [Deprecated on 28.04.2023, expires on 01.01.2024]
            { 4, 5, &Upgrade_4_To_5 }, // [Deprecated on 28.04.2023, expires on 28.04.2026]
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    static bool Upgrade_1_To_2(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 2);

        // Copy header chunk (header format is the same)
        if (CopyChunk(context, 0))
            return true;

        // Get mesh data chunk
        const auto srcData = context.Input.Header.Chunks[1];
        if (srcData == nullptr || srcData->IsMissing())
        {
            LOG(Warning, "Missing model data chunk");
            return true;
        }

        // Upgrade meshes data
        MemoryReadStream stream(srcData->Get(), srcData->Size());
        MemoryWriteStream output(srcData->Size());
        do
        {
            uint32 vertices;
            stream.ReadUint32(&vertices);
            uint32 triangles;
            stream.ReadUint32(&triangles);
            const uint32 indicesCount = triangles * 3;
            const bool use16BitIndexBuffer = indicesCount <= MAX_uint16;
            const uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
            if (vertices == 0 || triangles == 0)
                return true;
            const auto vb0 = stream.Move<VB0SkinnedElementType1>(vertices);
            const auto ib = stream.Move<byte>(indicesCount * ibStride);

            // Write back
            output.WriteUint32(vertices);
            output.WriteUint32(triangles);
            for (uint32 i = 0; i < vertices; i++)
            {
                const VB0SkinnedElementType1 oldVertex = vb0[i];
                VB0SkinnedElementType2 newVertex;
                newVertex.Position = oldVertex.Position;
                newVertex.TexCoord = oldVertex.TexCoord;
                newVertex.Normal = oldVertex.Normal;
                newVertex.Tangent = oldVertex.Tangent;
                newVertex.BlendIndices = oldVertex.BlendIndices;
                Float4 blendWeights(oldVertex.BlendWeights.R / 255.0f, oldVertex.BlendWeights.G / 255.0f, oldVertex.BlendWeights.B / 255.0f, oldVertex.BlendWeights.A / 255.0f);
                const float sum = blendWeights.SumValues();
                const float invSum = sum > ZeroTolerance ? 1.0f / sum : 0.0f;
                blendWeights *= invSum;
                newVertex.BlendWeights = Half4(blendWeights);
                output.WriteBytes(&newVertex, sizeof(newVertex));
            }
            output.WriteBytes(ib, indicesCount * ibStride);
        } while (stream.CanRead());

        // Save new data
        if (context.AllocateChunk(1))
            return true;
        context.Output.Header.Chunks[1]->Data.Copy(output.GetHandle(), output.GetPosition());

        return false;
    }

    static bool Upgrade_2_To_3(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 2 && context.Output.SerializedVersion == 3);

        // Copy meshes data chunk (format is the same)
        if (CopyChunk(context, 1))
            return true;

        // Rewrite header chunk (added LOD count)
        const auto srcData = context.Input.Header.Chunks[0];
        if (srcData == nullptr || srcData->IsMissing())
        {
            LOG(Warning, "Missing model header chunk");
            return true;
        }
        MemoryReadStream stream(srcData->Get(), srcData->Size());
        MemoryWriteStream output(srcData->Size());
        {
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
            output.WriteByte(1);

            // Screen Size
            output.WriteFloat(1.0f);

            // Amount of meshes
            uint16 meshesCount;
            stream.ReadUint16(&meshesCount);
            output.WriteUint16(meshesCount);

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
        }

        // Save new header data
        if (context.AllocateChunk(0))
            return true;
        context.Output.Header.Chunks[0]->Data.Copy(output.GetHandle(), output.GetPosition());

        return false;
    }

    static bool Upgrade_3_To_4(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 3 && context.Output.SerializedVersion == 4);

        // Rewrite header chunk (added blend shapes count)
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
                    output.WriteUint16(0);
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

        // Rewrite meshes data chunks (blend shapes added)
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

            for (int32 meshIndex = 0; meshIndex < meshesCounts[lodIndex]; meshIndex++)
            {
                uint32 vertices;
                stream.ReadUint32(&vertices);
                output.WriteUint32(vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                output.WriteUint32(triangles);
                uint16 blendShapesCount = 0;
                output.WriteUint16(blendShapesCount);
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
};

#endif
