// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Types.h"
#include "WriteStream.h"

/// <summary>
/// Implementation of the stream that has access to the file and is optimized for fast writing to it.
/// </summary>
/// <seealso cref="WriteStream" />
class FLAXENGINE_API FileWriteStream : public WriteStream
{
private:

    File* _file;
    uint32 _virtualPosInBuffer;
    byte _buffer[FILESTREAM_BUFFER_SIZE];

public:
    NON_COPYABLE(FileWriteStream);

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="file">File to write</param>
    FileWriteStream(File* file);

    /// <summary>
    /// Destructor
    /// </summary>
    ~FileWriteStream();

public:

    /// <summary>
    /// Gets the file handle.
    /// </summary>
    /// <returns>File</returns>
    FORCE_INLINE const File* GetFile() const
    {
        return _file;
    }

    /// <summary>
    /// Unlink file object passed via constructor
    /// </summary>
    void Unlink();

public:

    /// <summary>
    /// Open file to read data from it
    /// </summary>
    /// <param name="path">Path to the file</param>
    /// <returns>Created file writer stream or null if cannot perform it</returns>
    static FileWriteStream* Open(const StringView& path);

public:

    // [WriteStream]
    void Flush() final override;
    void Close() final override;
    uint32 GetLength() override;
    uint32 GetPosition() override;
    void SetPosition(uint32 seek) override;
    void WriteBytes(const void* data, uint32 bytes) override;
};
