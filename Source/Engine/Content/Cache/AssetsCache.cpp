// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "AssetsCache.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/DeleteMe.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/FileReadStream.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Storage/ContentStorageManager.h"
#include "Engine/Content/Storage/JsonStorageProxy.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Globals.h"
#include "FlaxEngine.Gen.h"

void AssetsCache::Init()
{
    Entry e;
    int32 count;
    Stopwatch stopwatch;
#if USE_EDITOR
    _path = Globals::ProjectCacheFolder / TEXT("AssetsCache.dat");
#else
    _path = Globals::ProjectContentFolder / TEXT("AssetsCache.dat");
#endif
    LOG(Info, "Loading Asset Cache {0}...", _path);

    // Check if assets registry exists
    if (!FileSystem::FileExists(_path))
    {
        _isDirty = true;
        LOG(Warning, "Cannot find assets cache file");
        return;
    }

    // Open file
    FileReadStream* stream = FileReadStream::Open(_path);
    DeleteMe<FileReadStream> deleteStream(stream);

    // Load version
    int32 version;
    stream->ReadInt32(&version);
    if (version != FLAXENGINE_VERSION_BUILD)
    {
        LOG(Warning, "Corrupted or not supported Asset Cache file. Version: {0}", version);
        return;
    }

    // Load paths
    String enginePath, projectPath;
    stream->ReadString(&enginePath, -410);
    stream->ReadString(&projectPath, -410);

    // Flags
    AssetsCacheFlags flags;
    stream->ReadInt32((int32*)&flags);

    // Check if other workspace instance used this cache
    if (EnumHasNoneFlags(flags, AssetsCacheFlags::RelativePaths) && enginePath != Globals::StartupFolder)
    {
        LOG(Warning, "Assets cache generated by the different {1} installation in \'{0}\'", enginePath, TEXT("engine"));
        return;
    }
    if (EnumHasNoneFlags(flags, AssetsCacheFlags::RelativePaths) && projectPath != Globals::ProjectFolder)
    {
        LOG(Warning, "Assets cache generated by the different {1} installation in \'{0}\'", projectPath, TEXT("project"));
        return;
    }

    ScopeLock lock(_locker);
    _isDirty = false;

    // Load elements count
    stream->ReadInt32(&count);
    _registry.Clear();
    _registry.EnsureCapacity(count);

    // Load data
    int32 rejectedCount = 0;
    for (int32 i = 0; i < count; i++)
    {
        stream->Read(e.Info.ID);
        stream->ReadString(&e.Info.TypeName, i - 13);
        stream->ReadString(&e.Info.Path, i);
#if ENABLE_ASSETS_DISCOVERY
        stream->Read(e.FileModified);
#else
        DateTime tmp1;
        stream->Read(tmp1);
#endif

        if (EnumHasAnyFlags(flags, AssetsCacheFlags::RelativePaths) && e.Info.Path.HasChars())
        {
            // Convert to absolute path
            e.Info.Path = Globals::StartupFolder / e.Info.Path;
        }

        // Use only valid entries
        if (IsEntryValid(e))
            _registry.Add(e.Info.ID, e);
        else
            rejectedCount++;
    }

    // Paths mapping
    stream->ReadInt32(&count);
    _pathsMapping.Clear();
    _pathsMapping.EnsureCapacity(count);
    for (int32 i = 0; i < count; i++)
    {
        Guid id;
        stream->Read(id);
        String mappedPath;
        stream->ReadString(&mappedPath, i + 73);

        if (EnumHasAnyFlags(flags, AssetsCacheFlags::RelativePaths) && mappedPath.HasChars())
        {
            // Convert to absolute path
            mappedPath = Globals::StartupFolder / mappedPath;
        }

        _pathsMapping.Add(mappedPath, id);
    }

    // Check errors
    const bool hasError = stream->HasError();
    deleteStream.Delete();
    if (hasError)
    {
        _isDirty = true;
        _registry.Clear();
        LOG(Warning, "Asset Cache file has an error. Removing it.");
        if (FileSystem::DeleteFile(_path))
        {
            LOG(Error, "Cannot delete registry file after reading error.");
        }
    }

    stopwatch.Stop();
    LOG(Info, "Asset Cache loaded {0} entries in {1}ms ({2} rejected)", _registry.Count(), stopwatch.GetMilliseconds(), rejectedCount);
}

bool AssetsCache::Save()
{
    // Registry can be saved only in editor
#if USE_EDITOR

    // Check if registry hasn't been edited
    if (!_isDirty && FileSystem::FileExists(_path))
        return false;

    ScopeLock lock(_locker);

    if (Save(_path, _registry, _pathsMapping))
        return true;

    _isDirty = false;

#endif

    return false;
}

bool AssetsCache::Save(const StringView& path, const Registry& entries, const PathsMapping& pathsMapping, const AssetsCacheFlags flags)
{
    PROFILE_CPU();

    LOG(Info, "Saving assets cache to \'{0}\', entries: {1}", path, entries.Count());

    // Open file
    auto stream = FileWriteStream::Open(path);
    if (stream == nullptr)
        return true;

    // Version
    stream->WriteInt32(FLAXENGINE_VERSION_BUILD);

    // Paths
    stream->WriteString(Globals::StartupFolder, -410);
    stream->WriteString(Globals::ProjectFolder, -410);

    // Flags
    stream->WriteInt32((int32)flags);

    // Items count
    stream->WriteInt32(entries.Count());

    // Data
    int32 index = 0;
    for (auto i = entries.Begin(); i.IsNotEnd(); ++i)
    {
        auto& e = i->Value();
        stream->Write(e.Info.ID);
        stream->WriteString(e.Info.TypeName, index - 13);
        stream->WriteString(e.Info.Path, index);
#if ENABLE_ASSETS_DISCOVERY
        stream->Write(e.FileModified);
#else
        stream->WriteInt64(0);
#endif
        index++;
    }

    // Paths mapping
    index = 0;
    stream->WriteInt32(pathsMapping.Count());
    for (auto i = pathsMapping.Begin(); i.IsNotEnd(); ++i)
    {
        stream->Write(i->Value());
        stream->WriteString(i->Key(), index + 73);
        index++;
    }

    // Cleanup
    stream->Flush();
    Delete(stream);

    return false;
}

const String& AssetsCache::GetEditorAssetPath(const Guid& id) const
{
    ScopeLock lock(_locker);
#if USE_EDITOR
    auto e = _registry.TryGet(id);
    return e ? e->Info.Path : String::Empty;
#else
    for (auto& e : _pathsMapping)
    {
        if (e.Value == id)
            return e.Key;
    }
    return String::Empty;
#endif
}

bool AssetsCache::FindAsset(const StringView& path, AssetInfo& info)
{
    PROFILE_CPU();

    bool result = false;

    ScopeLock lock(_locker);

    // Check if asset has direct mapping to id (used for some cooked assets)
    Guid id;
    if (_pathsMapping.TryGet(path, id))
    {
        return FindAsset(id, info);
    }
#if !USE_EDITOR
    if (FileSystem::IsRelative(path))
    {
        // Additional check if user provides path relative to the project folder (eg. Content/SomeAssets/MyFile.json)
        const String absolutePath = Globals::ProjectFolder / *path;
        if (_pathsMapping.TryGet(absolutePath, id))
        {
            return FindAsset(id, info);
        }
    }
#endif

    // Find asset in registry
    for (auto i = _registry.Begin(); i.IsNotEnd(); ++i)
    {
        auto& e = i->Value();
        if (e.Info.Path == path)
        {
            if (!IsEntryValid(e))
            {
                LOG(Warning, "Missing file from registry: \'{0}\':{1}:{2}", e.Info.Path, e.Info.ID, e.Info.TypeName);
                _registry.Remove(i);
            }
            else
            {
                // Found
                result = true;
                info = e.Info;
            }

            break;
        }
    }

    return result;
}

bool AssetsCache::FindAsset(const Guid& id, AssetInfo& info)
{
    PROFILE_CPU();
    bool result = false;
    ScopeLock lock(_locker);
    auto e = _registry.TryGet(id);
    if (e != nullptr)
    {
        if (!IsEntryValid(*e))
        {
            LOG(Warning, "Missing file from registry: \'{0}\':{1}:{2}", e->Info.Path, e->Info.ID, e->Info.TypeName);
            _registry.Remove(id);
        }
        else
        {
            // Found
            result = true;
            info = e->Info;
        }
    }
    return result;
}

void AssetsCache::GetAll(Array<Guid>& result) const
{
    PROFILE_CPU();
    ScopeLock lock(_locker);
    _registry.GetKeys(result);
}

void AssetsCache::GetAllByTypeName(const StringView& typeName, Array<Guid>& result) const
{
    PROFILE_CPU();
    ScopeLock lock(_locker);
    for (auto i = _registry.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value().Info.TypeName == typeName)
            result.Add(i->Key());
    }
}

void AssetsCache::RegisterAssets(FlaxStorage* storage)
{
    PROFILE_CPU();

    ASSERT(storage);

    // Get all entries
    Array<FlaxStorage::Entry> entries;
    storage->GetEntries(entries);
    ASSERT(entries.HasItems());

    ScopeLock lock(_locker);
    auto storagePath = storage->GetPath();

    // Remove all old entries from that location
    for (auto i = _registry.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value().Info.Path == storagePath)
            _registry.Remove(i);
    }

    // Find asset IDs collisions
    AssetInfo info;
    Array<int32> duplicatedEntries;
    for (int32 i = 0; i < entries.Count(); i++)
    {
        auto& e = entries[i];
        ASSERT(e.ID.IsValid());

        // Check if storage contains ID which has been already registered
        if (FindAsset(e.ID, info))
        {
#if PLATFORM_WINDOWS
            // On Windows - if you start your project using a shortcut/VS commandline -project, and using a upper/lower drive letter, it could the cache (case doesn't matter on OS)
            if (StringUtils::CompareIgnoreCase(storagePath.GetText(), info.Path.GetText()) != 0)
            {
                LOG(Warning, "Founded duplicated asset \'{0}\'. Locations: \'{1}\' and \'{2}\'", e.ID, storagePath, info.Path);
                duplicatedEntries.Add(i);
            }
            else
            {
                // Remove from registry so we can add it again later with the original ID, so we don't loose relations
                for (auto j = _registry.Begin(); j.IsNotEnd(); ++j)
                {
                    if (StringUtils::CompareIgnoreCase(j->Value().Info.Path.GetText(), storagePath.GetText()) == 0)
                        _registry.Remove(j);
                }
            }
#else
            LOG(Warning, "Founded duplicated asset \'{0}\'. Locations: \'{1}\' and \'{2}\'", e.ID, storagePath, info.Path);
            duplicatedEntries.Add(i);
#endif
        }
    }

    // Check if need to resolve any collisions
    if (duplicatedEntries.HasItems())
    {
        // Check if cannot resolve collision for that container (it must allow to write to)
        // TODO: we could support packages as well but don't have to do it now, maybe in future
        if (storage->AllowDataModifications() == false)
        {
            LOG(Error, "Cannot register \'{0}\'. Founded duplicated asset at \'{1}\' but storage container doesn't allow data modifications.", storagePath, info.Path);
            return;
        }

        // Process all duplicated entries
        for (int32 i = 0; i < duplicatedEntries.Count(); i++)
        {
            auto& e = entries[duplicatedEntries[i]];
#if USE_EDITOR
            if (storage->ChangeAssetID(e, Guid::New()))
#endif
            {
                LOG(Error, "Cannot modify duplicated asset ID {2} from \'{0}\'. Founded duplicated asset at \'{1}\'.", storagePath, info.Path, e.ID);
                return;
            }
        }
    }

    // Register all entries
    for (int32 i = 0; i < entries.Count(); i++)
    {
        auto& e = entries[i];

        // Send info
        LOG(Info, "Register asset {0}:{1} \'{2}\'", e.ID, e.TypeName, storagePath);

        // Add new asset entry
        _registry.Add(e.ID, Entry(e.ID, e.TypeName, storagePath));
    }

    // Mark registry as draft
    _isDirty = true;
}

void AssetsCache::RegisterAsset(const AssetHeader& header, const StringView& path)
{
    RegisterAsset(header.ID, header.TypeName, path);
}

void AssetsCache::RegisterAssets(const FlaxStorageReference& storage)
{
    RegisterAssets(storage.Get());
}

void AssetsCache::RegisterAsset(const Guid& id, const String& typeName, const StringView& path)
{
    PROFILE_CPU();
    ScopeLock lock(_locker);

    // Check if asset has been already added to the registry
    bool isMissing = true;
    for (auto i = _registry.Begin(); i.IsNotEnd(); ++i)
    {
        auto& e = i->Value();

        if (e.Info.ID == id)
        {
            if (e.Info.Path != path)
            {
                e.Info.Path = path;
                _isDirty = true;
            }
            if (e.Info.TypeName != typeName)
            {
                e.Info.TypeName = typeName;
                _isDirty = true;
            }
            isMissing = false;
            break;
        }

        if (e.Info.Path == path)
        {
            if (e.Info.ID != id)
            {
                e.Info.Path = path;
                _isDirty = true;
            }
            if (e.Info.TypeName != typeName)
            {
                e.Info.TypeName = typeName;
                _isDirty = true;
            }
            isMissing = false;
            break;
        }
    }

    if (isMissing)
    {
        LOG(Info, "Register asset {0}:{1} \'{2}\'", id, typeName, path);
        _registry.Add(id, Entry(id, typeName, path));
        _isDirty = true;
    }
}

bool AssetsCache::DeleteAsset(const StringView& path, AssetInfo* info)
{
    bool result = false;
    _locker.Lock();

    for (auto i = _registry.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value().Info.Path == path)
        {
            if (info)
                *info = i->Value().Info;
            _registry.Remove(i);
            _isDirty = true;
            result = true;
            break;
        }
    }

    _locker.Unlock();
    return result;
}

bool AssetsCache::DeleteAsset(const Guid& id, AssetInfo* info)
{
    bool result = false;
    _locker.Lock();

    const auto e = _registry.TryGet(id);
    if (e != nullptr)
    {
        if (info)
            *info = e->Info;
        _registry.Remove(id);
        _isDirty = true;
        result = true;
    }

    _locker.Unlock();
    return result;
}

bool AssetsCache::RenameAsset(const StringView& oldPath, const StringView& newPath)
{
    bool result = false;
    _locker.Lock();

    for (auto i = _registry.Begin(); i.IsNotEnd(); ++i)
    {
        if (i->Value().Info.Path == oldPath)
        {
            i->Value().Info.Path = newPath;
            _isDirty = true;
            result = true;
            break;
        }
    }

    _locker.Unlock();
    return result;
}

bool AssetsCache::IsEntryValid(Entry& e)
{
#if ENABLE_ASSETS_DISCOVERY
    // Check if file exists
    if (FileSystem::FileExists(e.Info.Path))
    {
        // Check if file hasn't been modified
        const auto fileModified = FileSystem::GetFileLastEditTime(e.Info.Path);
        if (fileModified == e.FileModified)
            return true;

        const auto extension = FileSystem::GetExtension(e.Info.Path).ToLower();

        // Check if it's a binary asset
        if (ContentStorageManager::IsFlaxStorageExtension(extension))
        {
            // Validate ID within storage container
            const auto storage = ContentStorageManager::GetStorage(e.Info.Path);
            if (storage)
            {
                // Check if storage at given location contains that asset
                const bool isValid = storage->HasAsset(e.Info);

                // Update entry and mark cache as dirty
                e.FileModified = fileModified;
                _isDirty = true;

                return isValid;
            }
        }
        // Check for json resource
        else if (JsonStorageProxy::IsValidExtension(extension))
        {
            // Check Json storage layer
            Guid jsonId;
            String jsonTypeName;
            if (JsonStorageProxy::GetAssetInfo(e.Info.Path, jsonId, jsonTypeName))
            {
                const bool isValid = e.Info.ID == jsonId && e.Info.TypeName == jsonTypeName;

                // Update entry and mark cache as dirty
                e.FileModified = fileModified;
                _isDirty = true;

                return isValid;
            }
        }
    }

    return false;

#else
    // In game we don't care about it because all cached asset entries are valid (precached)
    // Skip only entries with missing file
    return e.Info.Path.HasChars();
#endif
}
