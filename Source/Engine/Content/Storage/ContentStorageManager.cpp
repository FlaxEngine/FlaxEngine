// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ContentStorageManager.h"
#include "FlaxFile.h"
#include "FlaxPackage.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Profiler/ProfilerCPU.h"

namespace
{
    CriticalSection Locker;
#if USE_EDITOR
    Array<FlaxFile*> Files(1024);
    Array<FlaxPackage*> Packages;
#else
    Array<FlaxFile*> Files;
    Array<FlaxPackage*> Packages(64);
#endif
    Dictionary<String, FlaxStorage*> StorageMap(2048);
}

class ContentStorageManagerService : public EngineService
{
public:

    ContentStorageManagerService()
        : EngineService(TEXT("ContentStorageManager"), -800)
    {
    }

    void Update() override;
    void Dispose() override;
};

ContentStorageManagerService ContentStorageManagerServiceInstance;

TimeSpan ContentStorageManager::UnusedDataChunksLifetime = TimeSpan::FromSeconds(10);

FlaxStorageReference ContentStorageManager::GetStorage(const StringView& path, bool loadIt)
{
    Locker.Lock();

    // Try fast lookup
    FlaxStorage* result;
    if (!StorageMap.TryGet(path, result))
    {
        // Detect storage type and create object
        const bool isPackage = path.EndsWith(StringView(PACKAGE_FILES_EXTENSION));
        if (isPackage)
        {
            auto package = New<FlaxPackage>(path);
            Packages.Add(package);
            result = package;
        }
        else
        {
            auto file = New<FlaxFile>(path);
            Files.Add(file);
            result = file;
        }

        // Register storage container
        StorageMap.Add(path, result);
    }

    Locker.Unlock();

    if (loadIt)
    {
        // Initialize storage container
        result->LockChunks();
        const bool loadFailed = result->Load();
        result->UnlockChunks();
        if (loadFailed)
        {
            LOG(Error, "Failed to load {0}.", path);
            Locker.Lock();
            StorageMap.Remove(path);
            if (result->IsPackage())
                Packages.Remove((FlaxPackage*)result);
            else
                Files.Remove((FlaxFile*)result);
            Locker.Unlock();
            Delete(result);
            return nullptr;
        }
    }

    return result;
}

FlaxStorageReference ContentStorageManager::TryGetStorage(const StringView& path)
{
    ScopeLock lock(Locker);
    FlaxStorage* result = nullptr;
    StorageMap.TryGet(path, result);
    return result;
}

FlaxStorageReference ContentStorageManager::EnsureAccess(const StringView& path)
{
    // Note: because we want to create new storage package it may exists.
    // So let's check if any storage container is referencing that location and try to close it.
    auto storage = TryGetStorage(path);
    if (storage && storage->IsLoaded())
    {
        LOG(Info, "File \'{0}\' is in use. Trying to release handle to it.", path);
        storage->CloseFileHandles();
    }
    return storage;
}

uint32 ContentStorageManager::GetMemoryUsage()
{
    ScopeLock lock(Locker);
    uint32 result = 0;
    for (auto i = StorageMap.Begin(); i.IsNotEnd(); ++i)
    {
        result += i->Value->GetMemoryUsage();
    }
    return result;
}

bool ContentStorageManager::HasAsset(const Guid& id)
{
    ScopeLock lock(Locker);
    for (auto i = StorageMap.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value->HasAsset(id))
            return true;
    }
    return false;
}

bool ContentStorageManager::GetAssetEntry(const String& path, FlaxStorage::Entry& output)
{
    // Load storage container
    auto storage = GetStorage(path);
    if (!storage)
        return true;

    // Check entries
    if (storage->GetEntriesCount() != 1)
        return true;

    // Pick up the first entry
    storage->GetEntry(0, output);
    return false;
}

void ContentStorageManager::OnRenamed(const StringView& oldPath, const StringView& newPath)
{
    ScopeLock lock(Locker);

    // Update cached path key
    auto i = StorageMap.Find(oldPath);
    if (i != StorageMap.End())
    {
        ASSERT(!StorageMap.ContainsKey(newPath));
        const auto value = i->Value;
        StorageMap.Remove(i);
        StorageMap.Add(newPath, value);
    }
}

void ContentStorageManager::EnsureUnlocked()
{
    Locker.Lock();
    Locker.Unlock();
}

bool ContentStorageManager::IsFlaxStoragePath(const String& path)
{
    auto extension = FileSystem::GetExtension(path).ToLower();
    return IsFlaxStorageExtension(extension);
}

bool ContentStorageManager::IsFlaxStorageExtension(const String& extension)
{
    return extension == ASSET_FILES_EXTENSION || extension == PACKAGE_FILES_EXTENSION;
}

void ContentStorageManager::GetPackages(Array<FlaxPackage*>& result)
{
    result.Add(Packages);
}

void ContentStorageManager::GetFiles(Array<FlaxFile*>& result)
{
    result.Add(Files);
}

void ContentStorageManager::GetStorage(Array<FlaxStorage*>& result)
{
    result.EnsureCapacity(Packages.Count() + Files.Count());
    for (int32 i = 0; i < Packages.Count(); i++)
        result.Add(Packages[i]);
    for (int32 i = 0; i < Files.Count(); i++)
        result.Add(Files[i]);
}

void ContentStorageManagerService::Update()
{
    PROFILE_CPU();

    ScopeLock lock(Locker);

    for (auto i = StorageMap.Begin(); i.IsNotEnd(); ++i)
    {
        auto storage = i->Value;
        if (storage->ShouldDispose())
        {
            // Remove from collections
            if (storage->IsPackage())
                Packages.Remove((FlaxPackage*)storage);
            else
                Files.Remove((FlaxFile*)storage);
            StorageMap.Remove(i);

            // Delete
            storage->Dispose();
            Delete(storage);
        }
        else
        {
            storage->Tick();
        }
    }
}

void ContentStorageManagerService::Dispose()
{
    ScopeLock lock(Locker);

    // Cleanup
    for (auto i = StorageMap.Begin(); i.IsNotEnd(); ++i)
        i->Value->Dispose();
    Files.ClearDelete();
    Packages.ClearDelete();
    StorageMap.Clear();

    // Ensure that data has been disposed
    ASSERT(Files.IsEmpty() && Packages.IsEmpty());
}
