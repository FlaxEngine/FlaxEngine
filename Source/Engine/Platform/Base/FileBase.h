// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Enums.h"
#include "Engine/Core/Encoding.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/Array.h"

class StringBuilder;
template<typename T>
class DataContainer;

/// <summary>
/// Specifies how the operating system should open a file.
/// </summary>
DECLARE_ENUM_FLAGS_5(FileMode, uint32, CreateAlways, 2, CreateNew, 1, OpenAlways, 4, OpenExisting, 3, TruncateExisting, 5);

/// <summary>
/// Defines constants for read, write, or read/write access to a file.
/// </summary>
DECLARE_ENUM_FLAGS_3(FileAccess, uint32, Read, 0x80000000, Write, 0x40000000, ReadWrite, (uint32)FileAccess::Read | (uint32)FileAccess::Write);

/// <summary>
/// Contains constants for controlling the kind of access other objects can have to the same file.
/// </summary>
DECLARE_ENUM_FLAGS_6(FileShare, uint32, Delete, 0x00000004, None, 0x00000000, Read, 0x00000001, Write, 0x00000002, ReadWrite, (uint32)FileShare::Read | (uint32)FileShare::Write, All, (uint32)FileShare::ReadWrite | (uint32)FileShare::Delete);

/// <summary>
/// The base class for file objects.
/// </summary>
class FLAXENGINE_API FileBase : public NonCopyable
{
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="FileBase"/> class.
    /// </summary>
    virtual ~FileBase()
    {
    }

public:

    /// <summary>
    /// Reads data from a file.
    /// </summary>
    /// <param name="buffer">Output buffer to read data to it.</param>
    /// <param name="bytesToRead">The maximum amount bytes to read.</param>
    /// <param name="bytesRead">A pointer to the variable that receives the number of bytes read.</param>
    /// <returns>True if cannot read data, otherwise false.</returns>
    virtual bool Read(void* buffer, uint32 bytesToRead, uint32* bytesRead = nullptr) = 0;

    /// <summary>
    /// Writes data to a file.
    /// </summary>
    /// <param name="buffer">Output buffer to read data to it.</param>
    /// <param name="bytesToWrite">The maximum amount bytes to be written.</param>
    /// <param name="bytesWritten">A pointer to the variable that receives the number of bytes written.</param>
    /// <returns>True if cannot write data, otherwise false.</returns>
    virtual bool Write(const void* buffer, uint32 bytesToWrite, uint32* bytesWritten = nullptr) = 0;

    /// <summary>
    /// Close file handle
    /// </summary>
    virtual void Close() = 0;

public:

    /// <summary>
    /// Gets size of the file (in bytes).
    /// </summary>
    /// <returns>File size in bytes.</returns>
    virtual uint32 GetSize() const = 0;

    /// <summary>
    /// Retrieves the date and time that a file was last modified (in UTC).
    /// </summary>
    /// <returns>Last file write time.</returns>
    virtual DateTime GetLastWriteTime() const = 0;

    /// <summary>
    /// Gets current position of the file pointer.
    /// </summary>
    /// <returns>File pointer position.</returns>
    virtual uint32 GetPosition() const = 0;

    /// <summary>
    /// Sets new position of the file pointer.
    /// </summary>
    /// <param name="seek">New file pointer position.</param>
    virtual void SetPosition(uint32 seek) = 0;

    /// <summary>
    /// Returns true if file is opened.
    /// </summary>
    /// <returns>True if file is opened, otherwise false.</returns>
    virtual bool IsOpened() const = 0;

public:

    static bool ReadAllBytes(const StringView& path, byte* data, int32 length);
    static bool ReadAllBytes(const StringView& path, Array<byte>& data);
    static bool ReadAllBytes(const StringView& path, DataContainer<byte>& data);
    static bool ReadAllText(const StringView& path, String& data);
    static bool ReadAllText(const StringView& path, StringAnsi& data);
    static bool WriteAllBytes(const StringView& path, const byte* data, int32 length);
    static bool WriteAllBytes(const StringView& path, const Array<byte>& data);
    static bool WriteAllText(const StringView& path, const String& data, Encoding encoding);
    static bool WriteAllText(const StringView& path, const StringBuilder& data, Encoding encoding);
    static bool WriteAllText(const StringView& path, const Char* data, int32 length, Encoding encoding);
};
