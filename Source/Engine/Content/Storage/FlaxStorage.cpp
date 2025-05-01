// Copyright (c) Wojciech Figat. All rights reserved.

#include "FlaxStorage.h"
#include "FlaxFile.h"
#include "FlaxPackage.h"
#include "ContentStorageManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Platform/File.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Content/Asset.h"
#include "Engine/Content/Content.h"
#include "Engine/Threading/Threading.h"
#if USE_EDITOR
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Serialization/JsonWriters.h"
#else
#include "Engine/Engine/Globals.h"
#endif
#include <ThirdParty/LZ4/lz4.h>

int32 AssetHeader::GetChunksCount() const
{
    int32 result = 0;
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
    {
        if (Chunks[i] != nullptr)
            result++;
    }
    return result;
}

void AssetHeader::DeleteChunks()
{
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
    {
        SAFE_DELETE(Chunks[i]);
    }
}

void AssetHeader::UnlinkChunks()
{
    Platform::MemoryClear(Chunks, sizeof(Chunks));
}

String AssetHeader::ToString() const
{
    return String::Format(TEXT("ID: {0}, TypeName: {1}, Chunks Count: {2}"), ID, TypeName, GetChunksCount());
}

uint32 AssetInitData::GetHashCode() const
{
    // Do not use Metadata/Dependencies because it may not be loaded (it's optional)
    uint32 hashCode = GetHash(Header.ID);
    hashCode = (hashCode * 397) ^ SerializedVersion;
    hashCode = (hashCode * 397) ^ CustomData.Length();
    return hashCode;
}

void FlaxChunk::RegisterUsage()
{
    LastAccessTime = Platform::GetTimeSeconds();
}

const int32 FlaxStorage::MagicCode = 1180124739;

FlaxStorage::LockData FlaxStorage::LockData::Invalid(nullptr);

struct Header
{
    int32 MagicCode;
    uint32 Version;
    FlaxStorage::CustomData CustomData;
};

struct SerializedTypeNameV9
{
    Char Data[64];

    SerializedTypeNameV9()
    {
        Platform::MemoryClear(Data, sizeof(Data));
    }

    SerializedTypeNameV9(const String& data)
    {
        const int32 length = data.Length();
        ASSERT(length < ARRAY_COUNT(Data));
        Platform::MemoryCopy(Data, *data, length * sizeof(Char));
        Platform::MemoryClear(Data + length, (ARRAY_COUNT(Data) - length) * sizeof(Char));
    }
};

// [Deprecated on 4/17/2020, expires on 4/17/2022]
struct OldSerializedTypeNameV7
{
    Char Data[40];

    OldSerializedTypeNameV7()
    {
        Platform::MemoryClear(Data, sizeof(Data));
    }

    OldSerializedTypeNameV7(const String& data)
    {
        const int32 length = data.Length();
        ASSERT(length < ARRAY_COUNT(Data));
        Platform::MemoryClear(Data, sizeof(Data));
        Platform::MemoryCopy(Data, *data, (length + 1) * sizeof(Char));
    }
};

struct SerializedEntryV9
{
    Guid ID;
    SerializedTypeNameV9 TypeName;
    uint32 Address;

    SerializedEntryV9()
        : ID(Guid::Empty)
        , Address(0)
    {
    }

    SerializedEntryV9(const Guid& id, const String& typeName, uint32 address)
        : ID(id)
        , TypeName(typeName)
        , Address(address)
    {
    }
};

// [Deprecated on 4/17/2020, expires on 4/17/2022]
struct OldSerializedEntryV7
{
    Guid ID;
    OldSerializedTypeNameV7 TypeName;
    uint32 Address;

    OldSerializedEntryV7()
        : ID(Guid::Empty)
        , Address(0)
    {
    }

    OldSerializedEntryV7(const Guid& id, const String& typeName, uint32 address)
        : ID(id)
        , TypeName(typeName)
        , Address(address)
    {
    }
};

// [Deprecated on 1/20/2017, expires on 7/20/2020]
struct OldChunkEntry
{
    uint32 Adress;
    uint32 Size;

    OldChunkEntry()
        : Adress(0)
        , Size(0)
    {
    }
};

static_assert(sizeof(OldChunkEntry) == sizeof(uint32) * 2, "Wonrg size of the Flax File header chunk entry. Cannot use direct read.");

// [Deprecated on 4/17/2020, expires on 4/17/2022]
struct OldEntryV6
{
    Guid ID;
    uint32 TypeID;
    uint32 Adress;

    OldEntryV6()
        : ID(Guid::Empty)
        , TypeID(0)
        , Adress(0)
    {
    }

    OldEntryV6(const Guid& id, const uint32 typeId, uint32 address)
        : ID(id)
        , TypeID(typeId)
        , Adress(address)
    {
    }
};

// Converts old typeId to typeName of the asset
// [Deprecated on 16/10/2017, expires on 16/10/2020]
const Char* TypeId2TypeName(const uint32 typeId)
{
    switch (typeId)
    {
    case 1:
        return TEXT("FlaxEngine.Texture");
    case 2:
        return TEXT("FlaxEngine.Material");
    case 3:
        return TEXT("FlaxEngine.Model");
    case 4:
        return TEXT("FlaxEngine.MaterialInstance");
    case 6:
        return TEXT("FlaxEngine.FontAsset");
    case 7:
        return TEXT("FlaxEngine.Shader");
    case 8:
        return TEXT("FlaxEngine.CubeTexture");
    case 10:
        return TEXT("FlaxEngine.SpriteAtlas");
#if USE_EDITOR
    case 11:
        return TEXT("FlaxEditor.PreviewsCache");
#endif
    case 12:
        return TEXT("FlaxEngine.IESProfile");
    case 13:
        return TEXT("FlaxEngine.MaterialBase");
    case 14:
        return TEXT("FlaxEngine.RawDataAsset");
    }
    CRASH;
    return TEXT("");
}

FlaxStorage::FlaxStorage(const StringView& path)
    : _path(path)
{
}

FlaxStorage::~FlaxStorage()
{
    // Validate if has been disposed
    ASSERT(IsDisposed());
    CHECK(_chunksLock == 0);
    CHECK(_refCount == 0);
    ASSERT(_chunks.IsEmpty());

#if USE_EDITOR
    // Ensure to close any outstanding file handles to prevent file locking in case it failed to load
    Array<FileReadStream*> streams;
    _file.GetValues(streams);
    for (FileReadStream* stream : streams)
    {
        if (stream)
            Delete(stream);
    }
    Platform::AtomicStore(&_files, 0);
#endif
}

FlaxStorage::LockData FlaxStorage::LockSafe()
{
    auto lock = LockData(this);
    ContentStorageManager::EnsureUnlocked();
    return lock;
}

uint32 FlaxStorage::GetRefCount() const
{
    return (uint32)Platform::AtomicRead((int64*)&_refCount);
}

bool FlaxStorage::ShouldDispose() const
{
    return Platform::AtomicRead((int64*)&_refCount) == 0 &&
            Platform::AtomicRead((int64*)&_chunksLock) == 0 &&
            Platform::GetTimeSeconds() - _lastRefLostTime >= 0.5; // TTL in seconds
}

uint32 FlaxStorage::GetMemoryUsage() const
{
    uint32 result = sizeof(FlaxStorage);
    for (int32 i = 0; i < _chunks.Count(); i++)
        result += _chunks[i]->Data.Length();
    return result;
}

bool FlaxStorage::Load()
{
    // Check if was already loaded
    if (IsLoaded())
    {
        return false;
    }

    // Prevent loading by more than one thread
    ScopeLock lock(_loadLocker);
    if (IsLoaded())
    {
        // Other thread loaded it
        return false;
    }
    ASSERT(GetEntriesCount() == 0);

    // Open file
    auto stream = OpenFile();
    if (stream == nullptr)
        return true;

    // Magic Code
    uint32 magicCode;
    stream->ReadUint32(&magicCode);
    if (magicCode != MagicCode)
    {
        LOG(Warning, "Invalid asset magic code in {0}", ToString());
        return true;
    }

    // Version
    uint32 version;
    stream->ReadUint32(&version);
    switch (version)
    {
    case 9:
    {
        // Custom storage data
        CustomData customData;
        stream->Read(customData);

#if USE_EDITOR
        // Block loading packaged games content
        if (customData.ContentKey != 0)
#else
        // Block load content from unknown sources
        if (customData.ContentKey != Globals::ContentKey)
#endif
        {
            LOG(Warning, "Invalid asset {0}.", ToString());
            return true;
        }

        // Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        for (int32 i = 0; i < assetsCount; i++)
        {
            SerializedEntryV9 se;
            stream->ReadBytes(&se, sizeof(se));
            Entry e(se.ID, se.TypeName.Data, se.Address);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->ReadBytes(&e, sizeof(e));
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            auto chunk = New<FlaxChunk>();
            chunk->LocationInFile = e;
            stream->ReadInt32(reinterpret_cast<int32*>(&chunk->Flags));
            AddChunk(chunk);
        }

        break;
    }
    case 8:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // Custom storage data
        CustomData customData;
        stream->Read(customData);

#if USE_EDITOR
        // Block loading packaged games content
        if (customData.ContentKey != 0)
#else
        // Block load content from unknown sources
        if (customData.ContentKey != Globals::ContentKey)
#endif
        {
            LOG(Warning, "Invalid asset {0}.", ToString());
            return true;
        }

        // Asset Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        for (int32 i = 0; i < assetsCount; i++)
        {
            OldSerializedEntryV7 se;
            stream->ReadBytes(&se, sizeof(se));
            Entry e(se.ID, se.TypeName.Data, se.Address);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->ReadBytes(&e, sizeof(e));
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            auto chunk = New<FlaxChunk>();
            chunk->LocationInFile = e;
            stream->ReadInt32(reinterpret_cast<int32*>(&chunk->Flags));
            AddChunk(chunk);
        }

        break;
    }
    case 7:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // Asset Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        for (int32 i = 0; i < assetsCount; i++)
        {
            OldSerializedEntryV7 se;
            stream->ReadBytes(&se, sizeof(se));
            Entry e(se.ID, se.TypeName.Data, se.Address);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->ReadBytes(&e, sizeof(e));
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            auto chunk = New<FlaxChunk>();
            chunk->LocationInFile = e;
            AddChunk(chunk);
        }

        break;
    }
    case 6:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // Asset Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        for (int32 i = 0; i < assetsCount; i++)
        {
            OldEntryV6 ee;
            stream->ReadBytes(&ee, sizeof(ee));

            Entry e(ee.ID, TypeId2TypeName(ee.TypeID), ee.Adress);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->ReadBytes(&e, sizeof(e));
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            auto chunk = New<FlaxChunk>();
            chunk->LocationInFile = e;
            AddChunk(chunk);
        }

        break;
    }
    case 5:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // Package Time
        DateTime packTime;
        stream->Read(packTime);

        // Asset Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        for (int32 i = 0; i < assetsCount; i++)
        {
            OldEntryV6 ee;
            stream->ReadBytes(&ee, sizeof(ee));

            Entry e(ee.ID, TypeId2TypeName(ee.TypeID), ee.Adress);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->ReadBytes(&e, sizeof(e));
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            auto chunk = New<FlaxChunk>();
            chunk->LocationInFile = e;
            AddChunk(chunk);
        }

        break;
    }
    case 4:
    {
        // [Deprecated on 1/20/2017, expires on 7/20/2017]

        // Validate password
        uint32 tmp;
        stream->ReadUint32(&tmp);
        if (tmp > 0)
        {
            CRASH;
        }

        Entry e;
        e.Address = stream->GetPosition();

        // Asset ID
        stream->Read(e.ID);

        // Type ID
        uint32 typeId;
        stream->ReadUint32(&typeId);
        e.TypeName = TypeId2TypeName(typeId);

        // Metadata
        bool useMetadata = stream->ReadBool();
        if (useMetadata)
        {
            DateTime tmpDate;
            String strTmp;
            stream->Read(tmpDate);
            stream->Read(strTmp, -76);
            stream->Read(strTmp, 1301);
        }

        // Check char
        if (stream->ReadChar() != '.')
        {
            LOG(Warning, "Invalid Flax file ({0})", 2);
            return true;
        }

        // Load data chunks info
        OldChunkEntry chunks[16];
        stream->ReadBytes(chunks, sizeof(chunks));

        // Create chunk objects
        for (int32 i = 0; i < ARRAY_COUNT(chunks); i++)
        {
            auto& oldChunk = chunks[i];
            if (oldChunk.Size > 0)
            {
                auto chunk = New<FlaxChunk>();
                chunk->LocationInFile = FlaxChunk::Location(oldChunk.Adress, oldChunk.Size);
                AddChunk(chunk);
            }
        }

        // Fake entry
        AddEntry(e);
    }
    break;
    default:
        LOG(Warning, "Unsupported storage format version: {1}. {0}", ToString(), _version);
        return true;
    }

    // Mark as loaded (version number describes 'isLoaded' state)
    _version = version;

    return false;
}

#if USE_EDITOR

bool FlaxStorage::Reload()
{
    if (!IsLoaded())
        return false;
    PROFILE_CPU();

    OnReloading(this);

    // Perform clean reloading
    Dispose();
    const bool failed = Load();

    OnReloaded(this, failed);

    return failed;
}

bool FlaxStorage::ReloadSilent()
{
    ScopeLock lock(_loadLocker);
    if (!IsLoaded())
        return false;

    // Open file
    auto stream = OpenFile();
    if (stream == nullptr)
        return true;

    // Magic Code
    uint32 magicCode;
    stream->ReadUint32(&magicCode);
    if (magicCode != MagicCode)
    {
        LOG(Warning, "Invalid asset magic code in {0}", ToString());
        return true;
    }

    // Version
    uint32 version;
    stream->ReadUint32(&version);
    if (version == 9)
    {
        _version = version;

        // Custom storage data
        CustomData customData;
        stream->Read(customData);

        // Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        if (assetsCount != GetEntriesCount())
        {
            LOG(Warning, "Incorrect amount of assets ({} instead of {}) saved in {}", assetsCount, GetEntriesCount(), ToString());
            return true;
        }
        for (int32 i = 0; i < assetsCount; i++)
        {
            SerializedEntryV9 se;
            stream->ReadBytes(&se, sizeof(se));
            Entry e(se.ID, se.TypeName.Data, se.Address);
            SetEntry(i, e);
        }

        // TODO: load chunks metadata? compare against loaded chunks?
    }
    else
    {
        // Fallback to full reload
        LOG(Warning, "Fallback to full reload of {0}", ToString());
        Reload();
    }

    return false;
}

#endif

bool FlaxStorage::LoadAssetHeader(const Guid& id, AssetInitData& data)
{
    ASSERT(IsLoaded());

    // Get asset location in file
    Entry e;
    if (GetEntry(id, e))
    {
        LOG(Error, "Cannot find asset \'{0}\' within {1}", id, ToString());
        return true;
    }

    // Load header
    return LoadAssetHeader(e, data);
}

bool FlaxStorage::LoadAssetChunk(FlaxChunk* chunk)
{
    ASSERT(IsLoaded());
    ASSERT(chunk != nullptr && _chunks.Contains(chunk));

    // Check if already loaded
    if (chunk->IsLoaded())
        return false;

    // Ensure that asset is in a file
    if (chunk->ExistsInFile() == false)
    {
        LOG(Warning, "Cannot load chunk from {0}. It doesn't exist in storage.", ToString());
        return true;
    }

    LockChunks();

    // Open file
    auto stream = OpenFile();
    bool failed = stream == nullptr;
    if (!failed)
    {
        // Seek
        stream->SetPosition(chunk->LocationInFile.Address);

        if (stream->HasError())
        {
            // Sometimes stream->HasError() from setposition. result in a crash or missing media in release (stream _file._handle = nullptr).
            // When retrying, it looks like it works and we can continue. We need this to success.

            for (int retry = 0; retry < 5; retry++)
            {
                Platform::Sleep(50);
                stream = OpenFile();
                failed = stream == nullptr;
                if (!failed)
                {
                    stream->SetPosition(chunk->LocationInFile.Address);
                    if (!stream->HasError())
                        break;
                }
            }
        }

        if (!stream || stream->HasError())
        {
            failed = true;
            UnlockChunks();
            LOG(Warning, "SetPosition failed on chunk {0}.", ToString());
            return failed;
        }

        // Load data
        auto size = chunk->LocationInFile.Size;
        if (EnumHasAnyFlags(chunk->Flags, FlaxChunkFlags::CompressedLZ4))
        {
            // Compressed
            size -= sizeof(int32); // Don't count original size int
            int32 originalSize;
            stream->ReadInt32(&originalSize);
            Array<byte> tmpBuf;
            tmpBuf.Resize(size); // TODO: maybe use thread local or content loading pool with sharable temp buffers for the decompression?
            stream->ReadBytes(tmpBuf.Get(), size);

            // Decompress data
            PROFILE_CPU_NAMED("DecompressLZ4");
            chunk->Data.Allocate(originalSize);
            const int32 res = LZ4_decompress_safe((const char*)tmpBuf.Get(), chunk->Data.Get<char>(), size, originalSize);
            if (res <= 0)
            {
                UnlockChunks();
                LOG(Warning, "Cannot load chunk from {0}. Failed to decompress it data. Result: {1}.", ToString(), res);
                return true;
            }
            chunk->Data.SetLength(res);
        }
        else
        {
            // Raw data
            chunk->Data.Read(stream, size);
        }
        ASSERT(chunk->IsLoaded());
        chunk->RegisterUsage();
    }

    UnlockChunks();

    return failed;
}

#if USE_EDITOR

bool FlaxStorage::ChangeAssetID(Entry& e, const Guid& newId)
{
    ASSERT(IsLoaded());

    // TODO: validate entry
    ASSERT(newId.IsValid());
    ASSERT(AllowDataModifications());

    LOG(Info, "Changing asset \'{0}\' id to \'{1}\' (storage: \'{2}\')", e.ID, newId, _path);

    // Ensure to be loaded
    if (!IsLoaded())
    {
        if (Load())
        {
            return true;
        }
    }

    // Load asset entries data
    int32 entriesCount = GetEntriesCount();
    Array<AssetInitData> data;
    data.Resize(entriesCount);
    for (int32 i = 0; i < entriesCount; i++)
    {
        if (LoadAssetHeader(i, data[i]))
        {
            LOG(Warning, "Cannot load asset data.");
            return true;
        }
    }

    // Load all chunks
    for (int32 i = 0; i < _chunks.Count(); i++)
    {
        if (LoadAssetChunk(_chunks[i]))
        {
            LOG(Warning, "Cannot load asset chunk.");
            return true;
        }
    }

    // Close file
    if (CloseFileHandles())
    {
        LOG(Error, "Cannot close file access for '{}'", _path);
        return true;
    }

    // Change ID
    // TODO: here we could extend it and load assets from the storage and call asset ID change event to change references
    Array<Entry> entries;
    GetEntries(entries);
    for (int32 i = 0; i < entriesCount; i++)
    {
        Entry* myE = &entries[i];
        if (myE->ID == e.ID)
        {
            // Change ID
            e.ID = newId;
            myE->ID = newId;
            data[i].Header.ID = newId;
            break;
        }
    }

    // Repack container
    if (Create(_path, data))
    {
        LOG(Warning, "Cannot repack storage.");
        return true;
    }

    return false;
}

#endif

FlaxChunk* FlaxStorage::AllocateChunk()
{
    if (AllowDataModifications())
    {
        auto chunk = New<FlaxChunk>();
        _chunks.Add(chunk);
        return chunk;
    }

    LOG(Warning, "Cannot allocate chunk in {0}", ToString());
    return nullptr;
}

#if USE_EDITOR

bool FlaxStorage::Create(const StringView& path, Span<AssetInitData> assets, bool silentMode, const CustomData* customData)
{
    PROFILE_CPU();
    ZoneText(*path, path.Length());
    LOG(Info, "Creating package at \'{0}\'. Silent Mode: {1}", path, silentMode);

    // Prepare to have access to the file
    auto storage = ContentStorageManager::EnsureAccess(path);

    // Open file
    auto stream = FileWriteStream::Open(path);
    if (stream == nullptr)
        return true;

    // Create package
    bool result = Create(stream, assets, customData);

    // Close file
    Delete(stream);

    // Reload storage container
    if (storage && storage->IsLoaded())
    {
        if (silentMode)
        {
            // Silent data-only reload (loaded chunks location in file has been updated)
            storage->ReloadSilent();
        }
        else
        {
            // Full reload
            storage->Reload();
        }
    }

    return result;
}

bool FlaxStorage::Create(WriteStream* stream, Span<AssetInitData> assets, const CustomData* customData)
{
    if (assets.Get() == nullptr || assets.Length() <= 0)
    {
        LOG(Warning, "Cannot create new package. No assets to write.");
        return true;
    }

    // Prepare data
    Array<SerializedEntryV9, InlinedAllocation<1>> entries;
    entries.Resize(assets.Length());
    Array<FlaxChunk*> chunks;
    for (const AssetInitData& asset : assets)
    {
        for (FlaxChunk* chunk : asset.Header.Chunks)
        {
            if (chunk && chunk->IsLoaded())
                chunks.Add(chunk);
        }
    }
    const int32 chunksCount = chunks.Count();

    // TODO: sort chunks by size? smaller ones first?
    // Calculate start address of the first asset header location
    // 0 -> Header -> Entries Count -> Entries -> Chunks Count -> Chunk Locations
    int32 currentAddress = sizeof(Header)
            + sizeof(int32)
            + sizeof(SerializedEntryV9) * assets.Length()
            + sizeof(int32)
            + (sizeof(FlaxChunk::Location) + sizeof(int32)) * chunksCount;

    // Initialize entries offsets in the file
    for (int32 i = 0; i < assets.Length(); i++)
    {
        const AssetInitData& asset = assets[i];
        entries[i] = SerializedEntryV9(asset.Header.ID, asset.Header.TypeName, currentAddress);

        // Move forward by asset header data size
        currentAddress += sizeof(Guid) // ID
                + sizeof(SerializedTypeNameV9) // Type Name
                + sizeof(uint32) // Serialized Version
                + sizeof(int32) * 16 // Chunks mapping
                + sizeof(int32) + asset.CustomData.Length()
#if USE_EDITOR
                + sizeof(int32) + asset.Metadata.Length()
                + sizeof(int32) + asset.Dependencies.Count() * sizeof(Pair<Guid, DateTime>)
#else
                + sizeof(int32)
                + sizeof(int32)
#endif
                + sizeof(int32); // Header Hash Code
    }

    // Compress chunks
    Array<Array<byte>> compressedChunks;
    compressedChunks.Resize(chunksCount);
    for (int32 i = 0; i < chunksCount; i++)
    {
        const FlaxChunk* chunk = chunks[i];
        if (EnumHasAnyFlags(chunk->Flags, FlaxChunkFlags::CompressedLZ4))
        {
            PROFILE_CPU_NAMED("CompressLZ4");
            const int32 srcSize = chunk->Data.Length();
            const int32 maxSize = LZ4_compressBound(srcSize);
            auto& chunkCompressed = compressedChunks[i];
            chunkCompressed.Resize(maxSize);
            const int32 dstSize = LZ4_compress_default(chunk->Data.Get<char>(), (char*)chunkCompressed.Get(), srcSize, maxSize);
            if (dstSize <= 0)
            {
                chunkCompressed.Resize(0);
                LOG(Warning, "Chunk data LZ4 compression failed.");
                return true;
            }
            chunkCompressed.Resize(dstSize);
        }
    }

    // Initialize chunks locations in file
    for (int32 i = 0; i < chunksCount; i++)
    {
        int32 size = chunks[i]->Size();
        if (compressedChunks[i].HasItems())
            size = compressedChunks[i].Count() + sizeof(int32); // Add original data size
        ASSERT(size > 0);
        chunks[i]->LocationInFile = FlaxChunk::Location(currentAddress, size);
        currentAddress += size;
    }

    // Write header
    Header mainHeader;
    mainHeader.MagicCode = MagicCode;
    mainHeader.Version = 9;
    if (customData)
        mainHeader.CustomData = *customData;
    else
        Platform::MemoryClear(&mainHeader.CustomData, sizeof(CustomData));
    stream->Write(mainHeader);

    // Write asset entries
    stream->WriteInt32(assets.Length());
    stream->WriteBytes(entries.Get(), sizeof(SerializedEntryV9) * assets.Length());

    // Write chunk locations and meta
    stream->WriteInt32(chunksCount);
    for (int32 i = 0; i < chunksCount; i++)
    {
        FlaxChunk* chunk = chunks[i];
        stream->WriteBytes(&chunk->LocationInFile, sizeof(chunk->LocationInFile));
        FlaxChunkFlags flags = chunk->Flags & ~(FlaxChunkFlags::KeepInMemory); // Skip saving runtime-only flags
        stream->WriteInt32((int32)flags);
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION
    // Check calculated position of first asset header
    if (assets.Length() > 0 && stream->GetPosition() != entries[0].Address)
    {
        LOG(Warning, "Error while asset header location computation.");
        return true;
    }
#endif

    // Write asset headers
    for (int32 i = 0; i < assets.Length(); i++)
    {
        const AssetInitData& header = assets[i];

        // ID
        stream->Write(header.Header.ID);

        // Type Name
        SerializedTypeNameV9 typeName(header.Header.TypeName);
        stream->WriteBytes(typeName.Data, sizeof(SerializedTypeNameV9));

        // Serialized Version
        stream->WriteUint32(header.SerializedVersion);

        // Chunks mapping
        for (const FlaxChunk* chunk : header.Header.Chunks)
        {
            const int32 index = chunks.Find(chunk);
            ASSERT_LOW_LAYER(index >= -1 && index <= ALL_ASSET_CHUNKS);
            stream->WriteInt32(index);
        }

        // Custom Data
        stream->WriteInt32(header.CustomData.Length());
        header.CustomData.Write(stream);

        // Header Hash Code
        stream->WriteUint32(header.GetHashCode());

        // Json Metadata
        stream->WriteInt32(header.Metadata.Length());
        header.Metadata.Write(stream);

        // Asset Dependencies
        stream->WriteInt32(header.Dependencies.Count());
        stream->WriteBytes(header.Dependencies.Get(), header.Dependencies.Count() * sizeof(Pair<Guid, DateTime>));
        static_assert(sizeof(Pair<Guid, DateTime>) == sizeof(Guid) + sizeof(DateTime), "Invalid data size.");
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION
    // Check calculated position of first asset chunk
    if (chunksCount > 0 && stream->GetPosition() != chunks[0]->LocationInFile.Address)
    {
        LOG(Warning, "Error while asset data chunk location computation.");
        return true;
    }
#endif

    // Write chunks data
    for (int32 i = 0; i < chunksCount; i++)
    {
        if (compressedChunks[i].HasItems())
        {
            // Compressed chunk data (write additional size of the original data)
            stream->WriteInt32(chunks[i]->Data.Length());
            stream->WriteBytes(compressedChunks[i].Get(), compressedChunks[i].Count());
        }
        else
        {
            // Raw chunk data
            chunks[i]->Data.Write(stream);
        }
    }

    if (stream->HasError())
    {
        LOG(Warning, "Stream has error.");
        return true;
    }

    return false;
}

bool FlaxStorage::Save(const AssetInitData& data, bool silentMode)
{
    // Check if can modify the storage
    if (!AllowDataModifications())
        return true;

    // Note: we support saving only single asset, to save more assets in single package use FlaxStorage::Create(..)

    return Create(_path, data, silentMode);
}

#endif

bool FlaxStorage::LoadAssetHeader(const Entry& e, AssetInitData& data)
{
    ASSERT(IsLoaded());

    auto lock = Lock();

    // Open file
    auto stream = OpenFile();
    if (stream == nullptr)
    {
        return true;
    }

    // Seek
    stream->SetPosition(e.Address);

    // Switch version
    switch (_version)
    {
    case 9:
    {
        // ID
        stream->Read(data.Header.ID);

        // Type name
        SerializedTypeNameV9 typeName;
        stream->ReadBytes(typeName.Data, sizeof(SerializedTypeNameV9));
        data.Header.TypeName = typeName.Data;
        if (stream->HasError())
        {
            LOG(Warning, "Data stream error.");
            return true;
        }

        // Serialized Version
        stream->ReadUint32(&data.SerializedVersion);

        // Chunks mapping
        for (int32 i = 0; i < 16; i++)
        {
            int32 chunkIndex;
            stream->ReadInt32(&chunkIndex);
            if (chunkIndex < -1 || chunkIndex >= _chunks.Count())
            {
                LOG(Warning, "Invalid chunks mapping ({} -> {}).", i, chunkIndex);
                return true;
            }
            data.Header.Chunks[i] = chunkIndex == INVALID_INDEX ? nullptr : _chunks[chunkIndex];
        }

        // Custom data
        int32 customDataSize;
        stream->ReadInt32(&customDataSize);
        data.CustomData.Read(stream, customDataSize);

        // Header hash code
        uint32 headerHashCode;
        stream->ReadUint32(&headerHashCode);
        if (headerHashCode != data.GetHashCode())
        {
            LOG(Warning, "Asset header data is corrupted.");
            return true;
        }

#if USE_EDITOR
        // Metadata
        int32 metadataSize;
        stream->ReadInt32(&metadataSize);
        data.Metadata.Read(stream, metadataSize);

        // Asset Dependencies
        int32 dependencies;
        stream->ReadInt32(&dependencies);
        data.Dependencies.Resize(dependencies);
        stream->ReadBytes(data.Dependencies.Get(), dependencies * sizeof(Pair<Guid, DateTime>));
#endif
        break;
    }
    case 8:
    case 7:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // ID
        stream->Read(data.Header.ID);

        // Type Name
        OldSerializedTypeNameV7 typeName;
        stream->ReadBytes(typeName.Data, sizeof(OldSerializedTypeNameV7));
        data.Header.TypeName = typeName.Data;

        // Serialized Version
        stream->ReadUint32(&data.SerializedVersion);

        // Chunks mapping
        for (int32 i = 0; i < 16; i++)
        {
            int32 chunkIndex;
            stream->ReadInt32(&chunkIndex);
            if (chunkIndex < -1 || chunkIndex >= _chunks.Count())
            {
                LOG(Warning, "Invalid chunks mapping ({} -> {}).", i, chunkIndex);
                return true;
            }
            data.Header.Chunks[i] = chunkIndex == INVALID_INDEX ? nullptr : _chunks[chunkIndex];
        }

        // Custom Data
        int32 customDataSize;
        stream->ReadInt32(&customDataSize);
        data.CustomData.Read(stream, customDataSize);

        // Header Hash Code
        uint32 headerHashCode;
        stream->ReadUint32(&headerHashCode);
        if (headerHashCode != data.GetHashCode())
        {
            LOG(Warning, "Asset header data is corrupted.");
            return true;
        }

#if USE_EDITOR
        // Metadata
        int32 metadataSize;
        stream->ReadInt32(&metadataSize);
        data.Metadata.Read(stream, metadataSize);

        // Asset Dependencies
        data.Dependencies.Clear();
#endif

        break;
    }
    case 6:
    case 5:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // ID
        stream->Read(data.Header.ID);

        // Type ID
        uint32 typeId;
        stream->ReadUint32(&typeId);
        data.Header.TypeName = TypeId2TypeName(typeId);

        // Serialized Version
        stream->ReadUint32(&data.SerializedVersion);

        // Chunks mapping
        for (int32 i = 0; i < 16; i++)
        {
            int32 chunkIndex;
            stream->ReadInt32(&chunkIndex);
            if (chunkIndex < -1 || chunkIndex >= _chunks.Count())
            {
                LOG(Warning, "Invalid chunks mapping ({} -> {}).", i, chunkIndex);
                return true;
            }
            data.Header.Chunks[i] = chunkIndex == INVALID_INDEX ? nullptr : _chunks[chunkIndex];
        }

        // Custom Data
        int32 customDataSize;
        stream->ReadInt32(&customDataSize);
        data.CustomData.Read(stream, customDataSize);

        // Header Hash Code
        uint32 headerHashCode;
        stream->ReadUint32(&headerHashCode);
        if (headerHashCode != data.GetHashCode())
        {
            LOG(Warning, "Asset header data is corrupted.");
            return true;
        }

#if USE_EDITOR
        // Metadata
        int32 metadataSize;
        stream->ReadInt32(&metadataSize);
        data.Metadata.Read(stream, metadataSize);

        // Asset Dependencies
        data.Dependencies.Clear();
#endif

        break;
    }
    case 4:
    {
        // [Deprecated on 1/20/2017, expires on 7/20/2017]

        // Load data
        data.SerializedVersion = 1;
        stream->Read(data.Header.ID);
        uint32 typeId;
        stream->ReadUint32(&typeId);
        data.Header.TypeName = TypeId2TypeName(typeId);
        const bool useMetadata = stream->ReadBool();
        if (useMetadata)
        {
            DateTime importDate;
            String importPath, importUsername;
            stream->Read(importDate);
            stream->Read(importPath, -76);
            stream->Read(importUsername, 1301);

#if USE_EDITOR
            // Convert old metadata into the new format
            rapidjson_flax::StringBuffer buffer;
            CompactJsonWriter writerObj(buffer);
            JsonWriter& writer = writerObj;
            writer.StartObject();
            {
                writer.JKEY("ImportPath");
                writer.String(importPath);
                writer.JKEY("ImportUsername");
                writer.String(importUsername);
            }
            writer.EndObject();
            data.Metadata.Copy((const byte*)buffer.GetString(), (uint32)buffer.GetSize());
#endif
        }
        if (stream->ReadChar() != '.')
            return true;
        OldChunkEntry chunks[16];
        stream->ReadBytes(chunks, sizeof(chunks));
        int32 chunkIndex = 0;
        for (uint32 i = 0; i < ARRAY_COUNT(chunks); i++)
        {
            auto& oldChunk = chunks[i];
            if (oldChunk.Size > 0)
            {
                data.Header.Chunks[i] = _chunks[chunkIndex++];
            }
        }
        ASSERT(chunkIndex == _chunks.Count());
        data.CustomData.Release();
#if USE_EDITOR
        data.Dependencies.Clear();
#endif

        break;
    }
    default:
        return true;
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION
    // Validate loaded header (asset ID and type ID must be the same)
    if (e.ID != data.Header.ID)
    {
        LOG(Error, "Loading asset header data mismatch! Expected ID: {0}, loaded header: {1}.\nSource: {2}", e.ID, data.Header.ToString(), ToString());
    }
    if (e.TypeName != data.Header.TypeName)
    {
        LOG(Error, "Loading asset header data mismatch! Expected Type Name: {0}, loaded header: {1}.\nSource: {2}", e.TypeName, data.Header.ToString(), ToString());
    }
#endif

    return false;
}

void FlaxStorage::AddChunk(FlaxChunk* chunk)
{
    _chunks.Add(chunk);
}

FileReadStream* FlaxStorage::OpenFile()
{
    auto& stream = _file.Get();
    if (stream == nullptr)
    {
        // Open file
        auto file = File::Open(_path, FileMode::OpenExisting, FileAccess::Read, FileShare::Read);
        if (file == nullptr)
        {
            LOG(Error, "Cannot open Flax Storage file \'{0}\'.", _path);
            return nullptr;
        }
        Platform::InterlockedIncrement(&_files);

        // Create file reading stream
        stream = New<FileReadStream>(file);
    }
    return stream;
}

bool FlaxStorage::CloseFileHandles()
{
    if (Platform::AtomicRead(&_chunksLock) == 0 && Platform::AtomicRead(&_files) == 0)
    {
        return false;
    }
    PROFILE_CPU();

    // Note: this is usually called by the content manager when this file is not used or on exit
    // In those situations all the async tasks using this storage should be cancelled externally

    // Ensure that no one is using this resource
    int32 waitTime = 100;
    while (Platform::AtomicRead(&_chunksLock) != 0 && waitTime-- > 0)
        Platform::Sleep(1);
    if (Platform::AtomicRead(&_chunksLock) != 0)
    {
        // File can be locked by some streaming tasks (eg. AudioClip::StreamingTask or StreamModelLODTask)
        Entry e;
        for (int32 i = 0; i < GetEntriesCount(); i++)
        {
            GetEntry(i, e);
            Asset* asset = Content::GetAsset(e.ID);
            if (asset)
            {
                LOG(Info, "Canceling streaming for asset {0}", asset->ToString());
                asset->CancelStreaming();
            }
        }
    }
    waitTime = 100;
    while (Platform::AtomicRead(&_chunksLock) != 0 && waitTime-- > 0)
        Platform::Sleep(1);
    if (Platform::AtomicRead(&_chunksLock) != 0)
        return true; // Failed, someone is still accessing the file

    // Close file handles (from all threads)
    Array<FileReadStream*, InlinedAllocation<8>> streams;
    _file.GetValues(streams);
    for (FileReadStream* stream : streams)
    {
        if (stream)
            Delete(stream);
    }
    _file.Clear();
    Platform::AtomicStore(&_files, 0);
    return false;
}

void FlaxStorage::Dispose()
{
    if (IsDisposed())
        return;

    // Close file
    if (CloseFileHandles())
    {
        LOG(Error, "Cannot close file access for '{}'", _path);
    }

    // Release data
    _chunks.ClearDelete();
    _version = 0;
}

void FlaxStorage::Tick(double time)
{
    // Skip if file is in use
    if (Platform::AtomicRead(&_chunksLock) != 0)
        return;

    bool wasAnyUsed = false;
    const float unusedDataChunksLifetime = ContentStorageManager::UnusedDataChunksLifetime.GetTotalSeconds();
    for (int32 i = 0; i < _chunks.Count(); i++)
    {
        auto chunk = _chunks.Get()[i];
        const bool wasUsed = (time - chunk->LastAccessTime) < unusedDataChunksLifetime;
        if (!wasUsed && chunk->IsLoaded() && EnumHasNoneFlags(chunk->Flags, FlaxChunkFlags::KeepInMemory))
        {
            chunk->Unload();
        }
        wasAnyUsed |= wasUsed;
    }

    // Release file handles in none of chunks is in use
    if (!wasAnyUsed && Platform::AtomicRead(&_chunksLock) == 0)
    {
        CloseFileHandles();
    }
}

#if USE_EDITOR

void FlaxStorage::OnRename(const StringView& newPath)
{
    ASSERT(AllowDataModifications());
    _path = newPath;
}

#endif

FlaxFile::FlaxFile(const StringView& path)
    : FlaxStorage(path)
{
    _asset.ID = Guid::Empty;
}

String FlaxFile::ToString() const
{
    return String::Format(TEXT("Asset \'{0}\'"), _path);
}

bool FlaxFile::IsPackage() const
{
    return false;
}

bool FlaxFile::AllowDataModifications() const
{
    return true;
}

bool FlaxFile::HasAsset(const Guid& id) const
{
    return _asset.ID == id;
}

bool FlaxFile::HasAsset(const AssetInfo& info) const
{
#if USE_EDITOR
    if (_path != info.Path)
        return false;
#endif
    return _asset.ID == info.ID && _asset.TypeName == info.TypeName;
}

int32 FlaxFile::GetEntriesCount() const
{
    return _asset.ID.IsValid() ? 1 : 0;
}

void FlaxFile::GetEntry(int32 index, Entry& value) const
{
    ASSERT(index == 0);
    value = _asset;
}

#if USE_EDITOR

void FlaxFile::SetEntry(int32 index, const Entry& value)
{
    ASSERT(index == 0);
    _asset = value;
}
#endif

void FlaxFile::GetEntries(Array<Entry>& output) const
{
    if (_asset.ID.IsValid())
        output.Add(_asset);
}

void FlaxFile::Dispose()
{
    FlaxStorage::Dispose();

    _asset.ID = Guid::Empty;
}

bool FlaxFile::GetEntry(const Guid& id, Entry& e)
{
    e = _asset;
    return id != _asset.ID;
}

void FlaxFile::AddEntry(Entry& e)
{
    ASSERT(_asset.ID.IsValid() == false);
    _asset = e;
}

FlaxPackage::FlaxPackage(const StringView& path)
    : FlaxStorage(path)
    , _entries(256)
{
}

String FlaxPackage::ToString() const
{
    return String::Format(TEXT("Package \'{0}\'"), _path);
}

bool FlaxPackage::IsPackage() const
{
    return true;
}

bool FlaxPackage::AllowDataModifications() const
{
    return false;
}

bool FlaxPackage::HasAsset(const Guid& id) const
{
    return _entries.ContainsKey(id);
}

bool FlaxPackage::HasAsset(const AssetInfo& info) const
{
    ASSERT(_path == info.Path);
    const Entry* e = _entries.TryGet(info.ID);
    return e && e->TypeName == info.TypeName;
}

int32 FlaxPackage::GetEntriesCount() const
{
    return _entries.Count();
}

void FlaxPackage::GetEntry(int32 index, Entry& value) const
{
    ASSERT(index >= 0 && index < _entries.Count());
    for (auto i = _entries.Begin(); i.IsNotEnd(); ++i)
    {
        if (index-- <= 0)
        {
            value = i->Value;
            return;
        }
    }
}

#if USE_EDITOR

void FlaxPackage::SetEntry(int32 index, const Entry& value)
{
    ASSERT(index >= 0 && index < _entries.Count());
    for (auto i = _entries.Begin(); i.IsNotEnd(); ++i)
    {
        if (index-- <= 0)
        {
            i->Value = value;
            return;
        }
    }
}

#endif

void FlaxPackage::GetEntries(Array<Entry>& output) const
{
    _entries.GetValues(output);
}

void FlaxPackage::Dispose()
{
    FlaxStorage::Dispose();

    _entries.Clear();
}

bool FlaxPackage::GetEntry(const Guid& id, Entry& e)
{
    return !_entries.TryGet(id, e);
}

void FlaxPackage::AddEntry(Entry& e)
{
    ASSERT(HasAsset(e.ID) == false);
    _entries.Add(e.ID, e);
}
