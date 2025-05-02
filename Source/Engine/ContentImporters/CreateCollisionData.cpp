// Copyright (c) Wojciech Figat. All rights reserved.

#include "CreateCollisionData.h"

#if COMPILE_WITH_ASSETS_IMPORTER

#include "AssetsImportingManager.h"
#include "Engine/Physics/CollisionData.h"

CreateAssetResult CreateCollisionData::Create(CreateAssetContext& context)
{
    // Base
    IMPORT_SETUP(CollisionData, 1);

    CollisionData::SerializedOptions options;

    // Check if has cooking module and input argument has been specified
#if COMPILE_WITH_PHYSICS_COOKING
    if (context.CustomArg)
    {
        // Cook
        const auto arg = (CollisionCooking::Argument*)context.CustomArg;
        BytesContainer outputData;
        if (CollisionCooking::CookCollision(*arg, options, outputData))
        {
            return CreateAssetResult::Error;
        }

        // Save data to asset
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Allocate(outputData.Length() + sizeof(options));
        const auto resultChunkPtr = context.Data.Header.Chunks[0]->Data.Get();
        Platform::MemoryCopy(resultChunkPtr, &options, sizeof(options));
        Platform::MemoryCopy(resultChunkPtr + sizeof(options), outputData.Get(), outputData.Length());
    }
    else
#endif
    {
        // Use empty options
        options.Type = CollisionDataType::None;
        options.Model = Guid::Empty;
        options.ModelLodIndex = 0;
        options.ConvexFlags = ConvexMeshGenerationFlags::None;
        options.ConvexVertexLimit = 0;
        if (context.AllocateChunk(0))
            return CreateAssetResult::CannotAllocateChunk;
        context.Data.Header.Chunks[0]->Data.Copy(&options);
    }

    return CreateAssetResult::Ok;
}

#if COMPILE_WITH_PHYSICS_COOKING

bool CreateCollisionData::CookMeshCollision(const String& outputPath, CollisionCooking::Argument& arg)
{
    // Use in-build assets importing/creating pipeline to generate asset
    return AssetsImportingManager::Create(AssetsImportingManager::CreateCollisionDataTag, outputPath, &arg);
}

#endif

#endif
