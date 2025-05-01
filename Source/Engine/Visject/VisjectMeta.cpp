// Copyright (c) Wojciech Figat. All rights reserved.

#include "VisjectMeta.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Serialization/ReadStream.h"
#include "Engine/Serialization/WriteStream.h"

bool VisjectMeta::Load(ReadStream* stream, bool loadData)
{
    Release();

    int32 entries;
    stream->ReadInt32(&entries);
    Entries.Resize(entries);

    for (int32 i = 0; i < entries; i++)
    {
        Entry& e = Entries[i];

        stream->ReadInt32(&e.TypeID);
        DateTime creationTime;
        stream->Read(creationTime);

        uint32 dataSize;
        stream->ReadUint32(&dataSize);
        e.IsLoaded = loadData;
        if (loadData)
        {
            e.Data.Resize(dataSize, false);
            if (dataSize > 0)
            {
                stream->ReadBytes(e.Data.Get(), dataSize);
            }
        }
        else
        {
            e.Data.SetCapacity(0);
            stream->SetPosition(stream->GetPosition() + dataSize);
        }
    }

    return false;
}

bool VisjectMeta::Save(WriteStream* stream, bool saveData) const
{
    stream->WriteInt32(Entries.Count());

    for (int32 i = 0; i < Entries.Count(); i++)
    {
        const Entry& e = Entries[i];

        stream->WriteInt32(e.TypeID);
        stream->WriteInt64(0); // unused creation time

        const uint32 dataSize = e.IsLoaded && saveData ? e.Data.Count() : 0;
        stream->WriteUint32(dataSize);
        if (dataSize > 0)
        {
            stream->WriteBytes(e.Data.Get(), dataSize);
        }
    }

    return false;
}

void VisjectMeta::Release()
{
    Entries.Clear();
}

const VisjectMeta::Entry* VisjectMeta::GetEntry(int32 typeID) const
{
    const Entry* result = nullptr;
    for (const Entry& e : Entries)
    {
        if (e.TypeID == typeID)
        {
            result = &e;
            break;
        }
    }
    return result;
}

VisjectMeta::Entry* VisjectMeta::GetEntry(int32 typeID)
{
    Entry* result = nullptr;
    for (Entry& e : Entries)
    {
        if (e.TypeID == typeID)
        {
            result = &e;
            break;
        }
    }
    return result;
}

void VisjectMeta::AddEntry(int32 typeID, byte* data, int32 size)
{
    auto& e = Entries.AddOne();
    e.IsLoaded = true;
    e.TypeID = typeID;
    e.Data.Set(data, size);
}
