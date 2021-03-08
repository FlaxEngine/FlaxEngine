// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "BinaryAssetFactory.h"
#include "../BinaryAsset.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#if USE_EDITOR
#include "Engine/Content/Storage/FlaxFile.h"
#endif
#include "Engine/Content/Upgraders/BinaryAssetUpgrader.h"

bool BinaryAssetFactoryBase::Init(BinaryAsset* asset)
{
    ASSERT(asset && asset->Storage);

    // Prepare
    auto storage = asset->Storage;
    AssetInfo info;
    info.ID = asset->GetID();
    info.TypeName = asset->GetTypeName();
    info.Path = storage->GetPath();

    // Load serialized asset data
    AssetInitData initData;
    if (storage->LoadAssetHeader(info.ID, initData))
    {
        LOG(Error, "Cannot load asset header.\nInfo: {0}", info.ToString());
        return true;
    }

#if COMPILE_WITH_ASSET_UPGRADERS
    // Check if need to perform data conversion to the newer version (only in Editor)
    const auto upgrader = GetUpgrader();
    if (storage->AllowDataModifications() && upgrader && upgrader->ShouldUpgrade(initData.SerializedVersion))
    {
        const auto startTime = DateTime::NowUTC();
        LOG(Info, "Starting asset \'{0}\' conversion", info.Path);

        // Backup source file (in case of conversion failure)
        String backupPath;
        FileSystem::GetTempFilePath(backupPath);
        if (FileSystem::CopyFile(backupPath, info.Path))
        {
            LOG(Warning, "Failed to create backup file \'{0}\'. Cannot copy file.", backupPath);
        }

        // Create asset version migration context
        AssetMigrationContext context;
        context.Input = initData;

        // Perform conversion
        bool conversionFailed = UpgradeAsset(info, storage, context);

        // Process result
        if (conversionFailed)
        {
            LOG(Error, "Asset \'{0}\' conversion failed! Restoring backup file.", info.ToString());
        }
        else
        {
            // Peek the results
            initData = context.Output;

            auto msg = String::Format(TEXT("Asset \'{0}\' upgraded to version {1} successfully ({2} ms)"),
                                      info.ToString(),
                                      initData.SerializedVersion,
                                      Math::FloorToInt((float)(DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
            LOG_STR(Info, msg);
        }

        // Remove or restore backup file
        if (FileSystem::FileExists(backupPath))
        {
            if (conversionFailed)
            {
                storage->CloseFileHandles();
                if (FileSystem::MoveFile(info.Path, backupPath, true))
                {
                    LOG(Warning, "Failed to restore backup file \'{0}\'. Cannot move file.", backupPath);
                }
            }
            else
            {
                if (FileSystem::DeleteFile(backupPath))
                {
                    LOG(Warning, "Failed to remove backup file \'{0}\'.", backupPath);
                }
            }
        }
        else if (conversionFailed)
        {
            LOG(Warning, "Failed to restore backup file \'{0}\'. It's missing.", backupPath);
        }
    }
#endif

    // Check if serialized asset version is supported
    if (!IsVersionSupported(initData.SerializedVersion))
    {
        LOG(Warning, "Asset version {1} is not supported.\nInfo: {0}", info.ToString(), initData.SerializedVersion);
        return true;
    }

    // Initialize asset
    if (asset->Init(initData))
    {
        LOG(Error, "Cannot initialize asset.\nInfo: {0}", info.ToString());
        return true;
    }

    return false;
}

#if COMPILE_WITH_ASSET_UPGRADERS

bool BinaryAssetFactoryBase::UpgradeAsset(const AssetInfo& info, FlaxStorage* storage, AssetMigrationContext& context)
{
    // Load all asset chunks
    for (int32 chunkIndex = 0; chunkIndex < ASSET_FILE_DATA_CHUNKS; chunkIndex++)
    {
        auto chunk = context.Input.Header.Chunks[chunkIndex];
        if (chunk && storage->LoadAssetChunk(chunk))
        {
            LOG(Warning, "Failed to load asset chunk {0}.", chunkIndex);
            return true;
        }
    }

    // Init output header
    context.Output.Header = context.Input.Header;

    // Start upgrading chain
    const auto upgrader = static_cast<BinaryAssetUpgrader*>(GetUpgrader());
    int32 step = 0;
    do
    {
        // Unlink chunks in output (won't be used)
        context.Output.Header.UnlinkChunks();

        // Perform conversion
        // Note: on failed conversion there may be some memory leaks but we don't care in that case very much
        if (upgrader->Upgrade(context.Input.SerializedVersion, context))
            return true;

        // Swap input with output (remember to delete old input chunks if they were allocated by upgrader not by the storage)
        if (step++ > 1)
            context.Input.Header.DeleteChunks();
        context.Input = context.Output;
    } while (upgrader->ShouldUpgrade(context.Input.SerializedVersion));

    // Release storage internal data (should also close file handles)
    {
        // HACK: file is locked by some tasks: the current one that called asset data upgrade (LoadAssetTask)
        // and by asset data loading tasks which are waiting for the start after init task
        // Let's hide these locks just for the upgrade
        const auto locks = storage->_chunksLock;
        storage->_chunksLock = 0;
        storage->Dispose();
        storage->_chunksLock = locks;
    }

    // Serialize conversion result data
#if USE_EDITOR
    if (FlaxFile::Create(storage->GetPath(), context.Output))
#endif
    {
        LOG(Warning, "Cannot serialize converted data.");
        return true;
    }

    // Release output data
#if USE_EDITOR
    context.Output.Dependencies.Clear();
    context.Output.Metadata.Release();
#endif
    context.Output.CustomData.Release();
    context.Output.Header.DeleteChunks();

    // Reload storage
    if (storage->Load())
    {
        LOG(Warning, "Cannot reload asset storage file after the conversion.");
        return true;
    }

    // Load asset header
    if (storage->LoadAssetHeader(info.ID, context.Output))
    {
        LOG(Warning, "Cannot load asset header after the conversion.");
        return true;
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION

    // Validate output asset info
    if (context.Output.Header.ID != info.ID || context.Output.Header.TypeName != info.TypeName)
    {
        LOG(Warning, "Data leak! After asset conversion output TypeName or Id is different.");
        return true;
    }

    // Check if converted version is supported
    if (!IsVersionSupported(context.Output.SerializedVersion))
    {
        LOG(Warning, "Asset has been converted but the result version is not supported!");
        return true;
    }

#endif

    return false;
}

#endif

Asset* BinaryAssetFactoryBase::New(const AssetInfo& info)
{
    // Get the asset storage container but don't load it now
    const auto storage = ContentStorageManager::GetStorage(info.Path, false);
    if (!storage)
    {
        // Note: missing file situation should be handled before asset creation
        LOG(Warning, "Missing asset storage container at \'{0}\'!\nInfo: ", info.Path, info.ToString());
        return nullptr;
    }

    // Create asset object
    auto result = Create(info);

    // Perform fast init, we assume that given AssetInfo is valid 
    // and we can create asset object now without further verification
    // which will be done during asset loading on content pool thread.
    // Then we will perform asset storage upgrading and loading.
    AssetHeader header;
    header.ID = info.ID;
    header.TypeName = info.TypeName;
    if (result->Init(storage, header))
    {
        LOG(Warning, "Cannot initialize asset.\nInfo: {0}", info.ToString());
        Delete(result);
        result = nullptr;
    }

    return result;
}

Asset* BinaryAssetFactoryBase::NewVirtual(const AssetInfo& info)
{
    // Create asset object
    auto result = Create(info);

    // Initialize with virtual data
    AssetInitData initData;
    initData.Header.ID = info.ID;
    initData.Header.TypeName = info.TypeName;
    initData.SerializedVersion = result->GetSerializedVersion();
    if (result->InitVirtual(initData))
    {
        LOG(Warning, "Cannot initialize asset.\nInfo: {0}", info.ToString());
        Delete(result);
        result = nullptr;
    }

    return result;
}
