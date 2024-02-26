// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Threading/Threading.h"

/// <summary>
/// Skeleton Mask asset upgrader.
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class SkeletonMaskUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="SkeletonMaskUpgrader"/> class.
    /// </summary>
    SkeletonMaskUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 2, &Upgrade_1_To_2 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    static bool Upgrade_1_To_2(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 2);

        // Load data
        AssetReference<SkinnedModel> skeleton;
        Array<bool> bonesMask;
        {
            auto dataChunk = context.Input.Header.Chunks[0];
            if (dataChunk == nullptr || dataChunk->IsMissing())
            {
                LOG(Warning, "Missing data chunk.");
                return true;
            }
            MemoryReadStream stream(dataChunk->Get(), dataChunk->Size());

            Guid skeletonId;
            stream.Read(skeletonId);
            int32 maskCount;
            stream.ReadInt32(&maskCount);
            bonesMask.Resize(maskCount, false);
            if (maskCount > 0)
                stream.ReadBytes(bonesMask.Get(), maskCount * sizeof(bool));

            skeleton = skeletonId;
            if (skeleton && skeleton->WaitForLoaded())
            {
                LOG(Warning, "Failed to load skeleton to update skeleton mask.");
                return true;
            }
        }

        // Build nodes mask from bones
        Array<String> nodesMask;
        if (skeleton)
        {
            ScopeLock lock(skeleton->Locker);

            for (int32 i = 0; i < bonesMask.Count(); i++)
            {
                if (bonesMask[i])
                {
                    int32 nodeIndex = skeleton->Skeleton.Bones[i].NodeIndex;
                    nodesMask.Add(skeleton->Skeleton.Nodes[nodeIndex].Name);
                }
            }
        }

        // Save data
        {
            if (context.AllocateChunk(0))
                return true;
            MemoryWriteStream stream(4096);

            const Guid skeletonId = skeleton.GetID();
            stream.Write(skeletonId);
            stream.WriteInt32(nodesMask.Count());
            for (auto& e : nodesMask)
            {
                stream.WriteString(e, -13);
            }

            context.Output.Header.Chunks[0]->Data.Copy(stream.GetHandle(), stream.GetPosition());
        }

        return false;
    }
};

#endif
