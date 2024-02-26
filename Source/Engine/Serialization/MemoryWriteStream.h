// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "WriteStream.h"

/// <summary>
/// Implementation of of the stream that can be used for fast data writing to the memory.
/// </summary>
class FLAXENGINE_API MemoryWriteStream : public WriteStream
{
private:

    byte* _buffer;
    byte* _position;
    uint32 _capacity;

public:

    MemoryWriteStream();

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="capacity">Initial write buffer capacity (in bytes).</param>
    MemoryWriteStream(uint32 capacity);

    /// <summary>
    /// Destructor
    /// </summary>
    ~MemoryWriteStream();

public:

    /// <summary>
    /// Gets buffer handle
    /// </summary>
    /// <returns>Pointer to the buffer in memory</returns>
    FORCE_INLINE byte* GetHandle() const
    {
        return _buffer;
    }

    /// <summary>
    /// Gets current capacity of the memory stream
    /// </summary>
    /// <returns>Stream capacity in bytes</returns>
    FORCE_INLINE uint32 GetCapacity() const
    {
        return _capacity;
    }

    /// <summary>
    /// Gets current stream length (capacity in bytes)
    /// </summary>
    /// <returns>Stream length in bytes</returns>
    FORCE_INLINE uint32 GetLength() const
    {
        return _capacity;
    }

    /// <summary>
    /// Gets current position in the stream (in bytes)
    /// </summary>
    /// <returns>Stream position in bytes</returns>
    FORCE_INLINE uint32 GetPosition() const
    {
        return static_cast<uint32>(_position - _buffer);
    }

    /// <summary>
    /// Skips bytes from the target buffer without writing to it. Moves the write pointer in the buffer forward.
    /// </summary>
    /// <param name="bytes">The amount of bytes to skip.</param>
    /// <returns>The pointer to the target bytes in memory.</returns>
    void* Move(uint32 bytes);

    /// <summary>
    /// Skips the data from the target buffer without writing to it. Moves the write pointer in the buffer forward.
    /// </summary>
    /// <returns>The pointer to the data in memory.</returns>
    template<typename T>
    FORCE_INLINE T* Move()
    {
        return static_cast<T*>(Move(sizeof(T)));
    }

    /// <summary>
    /// Skips the data from the target buffer without writing to it. Moves the write pointer in the buffer forward.
    /// </summary>
    /// <param name="count">The amount of items to read.</param>
    /// <returns>The pointer to the data in memory.</returns>
    template<typename T>
    FORCE_INLINE T* Move(uint32 count)
    {
        return static_cast<T*>(Move(sizeof(T) * count));
    }

public:

    /// <summary>
    /// Cleanups the buffers, resets the position and allocated the new memory chunk.
    /// </summary>
    /// <param name="capacity">Initial write buffer capacity (in bytes).</param>
    void Reset(uint32 capacity);

    /// <summary>
    /// Saves current buffer contents to the file
    /// </summary>
    /// <param name="path">Filepath</param>
    /// <returns>True if cannot save data, otherwise false</returns>
    bool SaveToFile(const StringView& path) const;

public:

    // [WriteStream]
    void Flush() override;
    void Close() override;
    uint32 GetLength() override;
    uint32 GetPosition() override;
    void SetPosition(uint32 seek) override;
    void WriteBytes(const void* data, uint32 bytes) override;
};
