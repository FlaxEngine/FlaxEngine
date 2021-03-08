// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "FlaxStorage.h"
#include "FlaxFile.h"
#include "FlaxPackage.h"
#include "ContentStorageManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/File.h"
#include "Engine/Serialization/FileWriteStream.h"
#include "Engine/Serialization/JsonWriter.h"
#include "Engine/Serialization/JsonWriters.h"
#include <ThirdParty/LZ4/lz4.h>

String AssetHeader::ToString() const
{
    return String::Format(TEXT("ID: {0}, TypeName: {1}, Chunks Count: {2}"), ID, TypeName, GetChunksCount());
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
    : _refCount(0)
    , _chunksLock(0)
    , _version(0)
    , _path(path)
{
}

void FlaxStorage::AddRef()
{
    _refCount++;
}

void FlaxStorage::RemoveRef()
{
    ASSERT(_refCount > 0);

    _refCount--;

    if (_refCount == 0)
    {
        _lastRefLostTime = DateTime::NowUTC();
    }
}

FlaxStorage::~FlaxStorage()
{
    // Validate if has been disposed
    ASSERT(IsDisposed());

    // Validate other fields
    // Note: disposed storage has no open files
    CHECK(_chunksLock == 0);
    CHECK(_refCount == 0);
    ASSERT(_chunks.IsEmpty());
}

FlaxStorage::LockData FlaxStorage::LockSafe()
{
    auto lock = LockData(this);
    ContentStorageManager::EnsureUnlocked();
    return lock;
}

bool FlaxStorage::ShouldDispose() const
{
    return _refCount == 0 && DateTime::NowUTC() - _lastRefLostTime >= TimeSpan::FromMilliseconds(500) && Platform::AtomicRead((int64*)&_chunksLock) == 0;
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
        stream->Read(&customData);

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
            stream->Read(&se);
            Entry e(se.ID, se.TypeName.Data, se.Address);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->Read(&e);
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            FlaxChunkFlags flags;
            stream->ReadInt32(reinterpret_cast<int32*>(&flags));
            AddChunk(New<FlaxChunk>(e, flags));
        }

        break;
    }
    case 8:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // Custom storage data
        CustomData customData;
        stream->Read(&customData);

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
            stream->Read(&se);
            Entry e(se.ID, se.TypeName.Data, se.Address);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->Read(&e);
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            FlaxChunkFlags flags;
            stream->ReadInt32(reinterpret_cast<int32*>(&flags));
            AddChunk(New<FlaxChunk>(e, flags));
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
            stream->Read(&se);
            Entry e(se.ID, se.TypeName.Data, se.Address);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->Read(&e);
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            AddChunk(New<FlaxChunk>(e));
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
            stream->Read(&ee);

            Entry e(ee.ID, TypeId2TypeName(ee.TypeID), ee.Adress);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->Read(&e);
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            AddChunk(New<FlaxChunk>(e));
        }

        break;
    }
    case 5:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // Package Time
        DateTime packTime;
        stream->Read(&packTime);

        // Asset Entries
        int32 assetsCount;
        stream->ReadInt32(&assetsCount);
        for (int32 i = 0; i < assetsCount; i++)
        {
            OldEntryV6 ee;
            stream->Read(&ee);

            Entry e(ee.ID, TypeId2TypeName(ee.TypeID), ee.Adress);
            AddEntry(e);
        }

        // Chunks
        int32 chunksCount;
        stream->ReadInt32(&chunksCount);
        for (int32 i = 0; i < chunksCount; i++)
        {
            FlaxChunk::Location e;
            stream->Read(&e);
            if (e.Size == 0)
            {
                LOG(Warning, "Empty chunk found.");
                return true;
            }
            AddChunk(New<FlaxChunk>(e));
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
        stream->Read(&e.ID);

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
            stream->Read(&tmpDate);
            stream->ReadString(&strTmp, -76);
            stream->ReadString(&strTmp, 1301);
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
                AddChunk(New<FlaxChunk>(oldChunk.Adress, oldChunk.Size));
            }
        }

        // Fake entry
        AddEntry(e);
    }
    break;
    default:
    {
        LOG(Warning, "Unsupported storage format version: {1}. {0}", ToString(), _version);
        return true;
    }
    }

    // Mark as loaded (version number describes 'isLoaded' state)
    _version = version;

    return false;
}

#if USE_EDITOR

bool FlaxStorage::Reload()
{
    // Check if wasn't already loaded
    if (!IsLoaded())
    {
        LOG(Warning, "{0} isn't loaded.", ToString());
        return false;
    }

    OnReloading(this);

    // Perform clean reloading
    Dispose();
    bool failed = Load();

    OnReloaded(this, failed);

    return failed;
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

        // Load data
        auto size = chunk->LocationInFile.Size;
        if (chunk->Flags & FlaxChunkFlags::CompressedLZ4)
        {
            // Compressed
            size -= sizeof(int32); // Don't count original size int
            int32 originalSize;
            stream->ReadInt32(&originalSize);
            Array<byte> tmpBuf;
            tmpBuf.Resize(size); // TODO: maybe use thread local or content loading pool with sharable temp buffers for the decompression?
            stream->ReadBytes(tmpBuf.Get(), size);

            // Decompress data
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
    CloseFileHandles();

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

bool FlaxStorage::Create(const StringView& path, const AssetInitData* data, int32 dataCount, bool silentMode, const CustomData* customData)
{
    LOG(Info, "Creating package at \'{0}\'. Silent Mode: {1}", path, silentMode);

    // Prepare to have access to the file
    auto storage = ContentStorageManager::EnsureAccess(path);

    // Open file
    auto stream = FileWriteStream::Open(path);
    if (stream == nullptr)
        return true;

    // Create package
    bool result = Create(stream, data, dataCount, customData);

    // Close file
    Delete(stream);

    // Reload storage container (only if not in silent mode)
    if (storage && !silentMode)
    {
        storage->Reload();
    }

    return result;
}

bool FlaxStorage::Create(WriteStream* stream, const AssetInitData* data, int32 dataCount, const CustomData* customData)
{
    // Validate inputs
    if (data == nullptr || dataCount <= 0)
    {
        LOG(Warning, "Cannot create new package. No assets to write.");
        return true;
    }

    // Prepare data
    Array<SerializedEntryV9, InlinedAllocation<1>> entries;
    entries.Resize(dataCount);
    Array<FlaxChunk*> chunks;

    // Get all chunks
    for (int32 i = 0; i < dataCount; i++)
        data[i].Header.GetLoadedChunks(chunks);
    int32 chunksCount = chunks.Count();

    // TODO: sort chunks by size? smaller ones first?
    // Calculate start address of the first asset header location
    // 0 -> Header -> Entries Count -> Entries -> Chunks Count -> Chunk Locations
    int32 currentAddress = sizeof(Header)
            + sizeof(int32)
            + sizeof(SerializedEntryV9) * dataCount
            + sizeof(int32)
            + (sizeof(FlaxChunk::Location) + sizeof(int32)) * chunksCount;

    // Initialize entries offsets in the file
    for (int32 i = 0; i < dataCount; i++)
    {
        auto& asset = data[i];
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
        if (chunk->Flags & FlaxChunkFlags::CompressedLZ4)
        {
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
    stream->Write(&mainHeader);

    // Write asset entries
    stream->WriteInt32(dataCount);
    stream->WriteBytes(entries.Get(), sizeof(SerializedEntryV9) * dataCount);

    // Write chunk locations and meta
    stream->WriteInt32(chunksCount);
    for (int32 i = 0; i < chunksCount; i++)
    {
        FlaxChunk* chunk = chunks[i];
        stream->Write(&chunk->LocationInFile);
        stream->WriteInt32((int32)chunk->Flags);
    }

#if ASSETS_LOADING_EXTRA_VERIFICATION

    // Check calculated position of first asset header
    if (dataCount > 0 && stream->GetPosition() != entries[0].Address)
    {
        LOG(Warning, "Error while asset header location computation.");
        return true;
    }

#endif

    // Write asset headers
    for (int32 i = 0; i < dataCount; i++)
    {
        auto& header = data[i];

        // ID
        stream->Write(&header.Header.ID);

        // Type Name
        SerializedTypeNameV9 typeName(header.Header.TypeName);
        stream->WriteBytes(typeName.Data, sizeof(SerializedTypeNameV9));

        // Serialized Version
        stream->WriteUint32(header.SerializedVersion);

        // Chunks mapping
        for (int32 chunkIndex = 0; chunkIndex < ARRAY_COUNT(header.Header.Chunks); chunkIndex++)
        {
            const int32 index = chunks.Find(header.Header.Chunks[chunkIndex]);
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
        stream->Write(header.Dependencies.Get(), header.Dependencies.Count());
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
            // Compressed chunk data (write additional size of the original data
            stream->WriteInt32(chunks[i]->Data.Length());
            stream->Write(compressedChunks[i].Get(), compressedChunks[i].Count());
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

bool FlaxStorage::Save(AssetInitData& data, bool silentMode)
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
        stream->Read(&data.Header.ID);

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
            if (chunkIndex >= _chunks.Count())
            {
                LOG(Warning, "Invalid chunks mapping.");
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
        stream->Read(data.Dependencies.Get(), dependencies);
#endif
        break;
    }
    case 8:
    case 7:
    {
        // [Deprecated on 4/17/2020, expires on 4/17/2022]

        // ID
        stream->Read(&data.Header.ID);

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
            if (chunkIndex >= _chunks.Count())
            {
                LOG(Warning, "Invalid chunks mapping.");
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
        stream->Read(&data.Header.ID);

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
            if (chunkIndex >= _chunks.Count())
            {
                LOG(Warning, "Invalid chunks mapping.");
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
        stream->Read(&data.Header.ID);
        uint32 typeId;
        stream->ReadUint32(&typeId);
        data.Header.TypeName = TypeId2TypeName(typeId);
        const bool useMetadata = stream->ReadBool();
        if (useMetadata)
        {
            DateTime importDate;
            String importPath, importUsername;
            stream->Read(&importDate);
            stream->ReadString(&importPath, -76);
            stream->ReadString(&importUsername, 1301);

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

        // Create file reading stream
        stream = New<FileReadStream>(file);
    }
    return stream;
}

void FlaxStorage::CloseFileHandles()
{
    // Note: this is usually called by the content manager when this file is not used or on exit
    // In those situations all the async tasks using this storage should be cancelled externally

    // Ensure that no one is using this resource
    int32 waitTime = 500;
    while (Platform::AtomicRead(&_chunksLock) != 0 && waitTime-- > 0)
        Platform::Sleep(10);
    ASSERT(_chunksLock == 0);

    _file.DeleteAll();
}

void FlaxStorage::Dispose()
{
    if (IsDisposed())
        return;

    // Close file
    CloseFileHandles();

    // Release data
    _chunks.ClearDelete();
    _version = 0;
}

void FlaxStorage::Tick()
{
    // Check if chunks are locked
    if (Platform::AtomicRead(&_chunksLock) != 0)
        return;

    const auto now = DateTime::NowUTC();
    bool wasAnyUsed = false;
    for (int32 i = 0; i < _chunks.Count(); i++)
    {
        auto chunk = _chunks[i];
        const bool wasUsed = (now - chunk->LastAccessTime) < ContentStorageManager::UnusedDataChunksLifetime;
        if (!wasUsed && chunk->IsLoaded())
        {
            chunk->Unload();
        }
        wasAnyUsed |= wasUsed;
    }

    // Release file handles in none of chunks was not used
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

void FlaxFile::GetEntry(int32 index, Entry& output) const
{
    ASSERT(index == 0);
    output = _asset;
}

void FlaxFile::GetEntries(Array<Entry>& output) const
{
    if (_asset.ID.IsValid())
        output.Add(_asset);
}

void FlaxFile::Dispose()
{
    // Base
    FlaxStorage::Dispose();

    // Clean
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
    Entry* e = _entries.TryGet(info.ID);
    return e && e->TypeName == info.TypeName;
}

int32 FlaxPackage::GetEntriesCount() const
{
    return _entries.Count();
}

void FlaxPackage::GetEntry(int32 index, Entry& output) const
{
    ASSERT(index >= 0 && index < _entries.Count());
    for (auto i = _entries.Begin(); i.IsNotEnd(); ++i)
    {
        if (index-- <= 0)
        {
            output = i->Value;
            return;
        }
    }
}

void FlaxPackage::GetEntries(Array<Entry>& output) const
{
    _entries.GetValues(output);
}

void FlaxPackage::Dispose()
{
    // Base
    FlaxStorage::Dispose();

    // Clean
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
