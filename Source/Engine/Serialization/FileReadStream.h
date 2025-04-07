// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Types.h"
#include "ReadStream.h"

/// <summary>
/// Implementation of the stream that has access to the file and is optimized for fast reading from it
/// </summary>
/// <seealso cref="ReadStream" />
class FLAXENGINE_API FileReadStream : public ReadStream
{
private:
    File* _file;
    uint32 _virtualPosInBuffer; // Current position in the buffer (index)
    uint32 _bufferSize; // Amount of loaded bytes from the file to the buffer
    uint32 _filePosition; // Cached position in the file (native)
    byte _buffer[FILESTREAM_BUFFER_SIZE];

public:
    NON_COPYABLE(FileReadStream);

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="file">File to read</param>
    FileReadStream(File* file);

    /// <summary>
    /// Destructor
    /// </summary>
    ~FileReadStream();

public:
    /// <summary>
    /// Gets the file handle.
    /// </summary>
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
    /// Open file to write data to it
    /// </summary>
    /// <param name="path">Path to the file</param>
    /// <returns>Created file reader stream or null if cannot perform that action</returns>
    static FileReadStream* Open(const StringView& path);

public:
    // [ReadStream]
    void Flush() final override;
    void Close() final override;
    uint32 GetLength() override;
    uint32 GetPosition() override;
    void SetPosition(uint32 seek) override;
    void ReadBytes(void* data, uint32 bytes) override;
};
