// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/File.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"

bool FileBase::ReadAllBytes(const StringView& path, byte* data, int32 length)
{
    PROFILE_CPU_NAMED("File::ReadAllBytes");
    ZoneText(*path, path.Length());
    bool result = true;
    auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::All);
    if (file)
    {
        const uint32 size = Math::Min<uint32>(file->GetSize(), length);
        if (size > 0)
        {
            result = file->Read(data, size) != 0;
        }
        else
        {
            result = false;
        }
        Delete(file);
    }
    return result;
}

bool FileBase::ReadAllBytes(const StringView& path, Array<byte>& data)
{
    PROFILE_CPU_NAMED("File::ReadAllBytes");
    ZoneText(*path, path.Length());
    bool result = true;
    auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::All);
    if (file)
    {
        const uint32 size = file->GetSize();
        if (size < MAX_int32)
        {
            data.Resize(size, false);
            if (size > 0)
            {
                result = file->Read(data.Get(), size) != 0;
            }
            else
            {
                data.Resize(0, false);
                result = false;
            }
        }
        else
        {
            LOG(Error, "Failed to load file {0}. It's too big. Size: {1} MB", path, size / (1024 * 1024));
        }
        Delete(file);
    }
    return result;
}

bool FileBase::ReadAllBytes(const StringView& path, DataContainer<byte>& data)
{
    PROFILE_CPU_NAMED("File::ReadAllBytes");
    ZoneText(*path, path.Length());
    bool result = true;
    auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::All);
    if (file)
    {
        const uint32 size = file->GetSize();
        if (size < MAX_int32)
        {
            data.Allocate(size);
            if (size > 0)
            {
                result = file->Read(data.Get(), size) != 0;
            }
            else
            {
                data.Release();
                result = false;
            }
        }
        else
        {
            LOG(Error, "Failed to load file {0}. It's too big. Size: {1} MB", path, size / (1024 * 1024));
        }
        Delete(file);
    }
    return result;
}

bool FileBase::ReadAllText(const StringView& path, String& data)
{
    PROFILE_CPU_NAMED("File::ReadAllText");
    ZoneText(*path, path.Length());
    data.Clear();

    // Read data
    Array<byte> bytes;
    if (ReadAllBytes(path, bytes))
        return true;

    // Check for empty data
    if (bytes.IsEmpty())
        return false;

    // Add null terminator char
    int32 count = bytes.Count();
    bytes.Add('\0');

    // Check BOM char
    // TODO: maybe let's check whole BOM????
    switch (bytes[0])
    {
    case 0xEF: // UTF-8
    {
        // BOM: EF BB BF

        // Eat BOM
        count -= 3;
        if (count < 0)
        {
            // Invalid data
            return true;
        }

        // Convert to UTF-16
        int32 utf16Length;
        Char* utf16Data = StringUtils::ConvertUTF82UTF16(reinterpret_cast<char*>(bytes.Get()), count, utf16Length);
        data.Set(utf16Data, utf16Length);
        Allocator::Free(utf16Data);
    }
    break;
    case 0xFE: // UTF-16 (BE)
    {
        // BOM: FE FF

        // Eat BOM
        count -= 2;
        if (count < 0)
        {
            // Invalid data
            return true;
        }

        data = (Char*)bytes.Get();
    }
    break;
    case 0xFF: // UTF-16 (LE)
    {
        // BOM: FF FE

        // Eat BOM
        count -= 2;
        if (count < 0)
        {
            // Invalid data
            return true;
        }

        data = (Char*)bytes.Get();
    }
    break;
    default: // ANSI
    {
        // Convert to UTF-16
        data = (char*)bytes.Get();
    }
    break;
    }

    return false;
}

bool FileBase::ReadAllText(const StringView& path, StringAnsi& data)
{
    PROFILE_CPU_NAMED("File::ReadAllText");
    ZoneText(*path, path.Length());
    data.Clear();

    // Read data
    Array<byte> bytes;
    if (ReadAllBytes(path, bytes))
        return true;

    // Check for empty data
    if (bytes.IsEmpty())
        return false;

    // Check BOM char
    // TODO: maybe let's check whole BOM????
    switch (bytes[0])
    {
    case 0xEF: // UTF-8
    {
        // BOM: EF BB BF
        if (bytes.Count() < 3)
            return true;
        data.Set((char*)(bytes.Get() + 3), bytes.Count() - 3);
    }
    break;
    case 0xFE: // UTF-16 (BE)
    {
        // BOM: FE FF
        if (bytes.Count() < 2)
            return true;
        const int32 length = bytes.Count() / 2 - 1;
        data.ReserveSpace(length);
        StringUtils::ConvertUTF162ANSI((Char*)(bytes.Get() + 2), data.Get(), length);
    }
    break;
    case 0xFF: // UTF-16 (LE)
    {
        // BOM: FF FE
        if (bytes.Count() < 2)
            return true;
        const int32 length = bytes.Count() / 2 - 1;
        data.ReserveSpace(length);
        StringUtils::ConvertUTF162ANSI((Char*)(bytes.Get() + 2), data.Get(), length);
    }
    break;
    default: // ANSI
    {
        data.Set((char*)bytes.Get(), bytes.Count());
    }
    break;
    }

    return false;
}

bool FileBase::WriteAllBytes(const StringView& path, const byte* data, int32 length)
{
    PROFILE_CPU_NAMED("File::WriteAllBytes");
    ZoneText(*path, path.Length());
    bool result = true;
    auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::All);
    if (file)
    {
        if (length > 0)
            result = file->Write(data, length) != 0;
        else
            result = false;

        Delete(file);
    }
    return result;
}

bool FileBase::WriteAllBytes(const StringView& path, const Array<byte>& data)
{
    return WriteAllBytes(path, data.Get(), data.Count());
}

bool FileBase::WriteAllText(const StringView& path, const String& data, Encoding encoding)
{
    return WriteAllText(path, *data, data.Length(), encoding);
}

bool FileBase::WriteAllText(const StringView& path, const StringBuilder& data, Encoding encoding)
{
    return WriteAllText(path, *data, data.Length(), encoding);
}

bool FileBase::WriteAllText(const StringView& path, const Char* data, int32 length, Encoding encoding)
{
    PROFILE_CPU_NAMED("File::WriteAllText");
    ZoneText(*path, path.Length());
    switch (encoding)
    {
    case Encoding::ANSI:
    {
        Array<char> ansiBytes(length);
        StringUtils::ConvertUTF162ANSI(data, ansiBytes.Get(), length);
        return WriteAllBytes(path, (byte*)ansiBytes.Get(), length * sizeof(char));
    }
    case Encoding::Unicode:
    {
        // UTF-16 (LE)
        // BOM: FF FE

        // TODO: make it faster without additional array

        int32 size = length * sizeof(Char);
        Array<byte> tmp;
        tmp.Resize(size + 2);
        tmp[0] = 0xFF;
        tmp[1] = 0xFE;
        Platform::MemoryCopy(tmp.Get() + 2, data, size);

        return WriteAllBytes(path, tmp.Get(), tmp.Count());
    }
    case Encoding::UnicodeBigEndian:
    {
        // UTF-16 (BE)
        // BOM: FE FF

        // TODO: make it faster without additional array

        int32 size = length * sizeof(Char);
        Array<byte> tmp;
        tmp.Resize(size + 2);
        tmp[0] = 0xFE;
        tmp[1] = 0xFF;
        Platform::MemoryCopy(tmp.Get() + 2, data, size);

        return WriteAllBytes(path, tmp.Get(), tmp.Count());
    }
    case Encoding::UTF8:
    {
        Array<byte> tmp;
        tmp.SetCapacity(length);

        for (int32 i = 0; i < length; i++)
        {
            Char c = data[i];
            if (c < 0x0080)
            {
                tmp.Add((byte)c);
            }
            else if (c < 0x0800)
            {
                tmp.Add(0xC0 | (c >> 6));
                tmp.Add(0x80 | (c & 0x3F));
            }
            else if (c < 0xD800 || c >= 0xE000)
            {
                tmp.Add(0xE0 | (c >> 12));
                tmp.Add(0x80 | ((c >> 6) & 0x3F));
                tmp.Add(0x80 | (c & 0x3F));
            }
            else // Surrogate pair
            {
                ++i;
                if (i >= length)
                    return true;

                uint32 p = 0x10000 + (((c & 0x3FF) << 10) | (data[i] & 0x3FF));
                tmp.Add(0xF0 | (p >> 18));
                tmp.Add(0x80 | ((p >> 12) & 0x3F));
                tmp.Add(0x80 | ((p >> 6) & 0x3F));
                tmp.Add(0x80 | (p & 0x3F));
            }
        }

        return WriteAllBytes(path, tmp.Get(), tmp.Count());
    }
    default:
        return true;
    }
}
