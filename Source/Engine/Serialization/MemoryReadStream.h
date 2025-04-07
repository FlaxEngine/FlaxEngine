// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "ReadStream.h"

/// <summary>
/// Direct memory reading stream  that uses a single allocation buffer.
/// </summary>
class FLAXENGINE_API MemoryReadStream : public ReadStream
{
private:
    const byte* _buffer;
    const byte* _position;
    uint32 _length;

public:
    /// <summary>
    /// Init (empty, cannot access before Init())
    /// </summary>
    /// <summary>
    /// Initializes a new instance of the <see cref="MemoryReadStream"/> class.
    /// </summary>
    MemoryReadStream();
    
    /// <summary>
    /// Initializes a new instance of the <see cref="MemoryReadStream"/> class.
    /// </summary>
    /// <param name="bytes">Bytes with data to read from it (no memory cloned, using input buffer).</param>
    /// <param name="length">Amount of bytes</param>
    MemoryReadStream(const byte* bytes, uint32 length);
    
    /// <summary>
    /// Initializes a new instance of the <see cref="MemoryReadStream"/> class.
    /// </summary>
    /// <param name="data">Array with data to read from.</param>
    template<typename T, typename AllocationType = HeapAllocation>
    MemoryReadStream(const Array<T, AllocationType>& data)
        : MemoryReadStream(data.Get(), data.Count() * sizeof(T))
    {
    }
    
    /// <summary>
    /// Initializes a new instance of the <see cref="MemoryReadStream"/> class.
    /// </summary>
    /// <param name="data">Span with data to read from.</param>
    template<typename T>
    MemoryReadStream(const Span<T>& data)
        : MemoryReadStream(data.Get(), data.Length() * sizeof(T))
    {
    }

public:
    /// <summary>
    /// Init stream to the custom buffer location.
    /// </summary>
    /// <param name="bytes">Bytes with data to read from it (no memory cloned, using input buffer).</param>
    /// <param name="length">Amount of bytes.</param>
    void Init(const byte* bytes, uint32 length);

    /// <summary>
    /// Init stream to the custom buffer location.
    /// </summary>
    /// <param name="data">Array with data to read from.</param>
    template<typename T, typename AllocationType = HeapAllocation>
    FORCE_INLINE void Init(const Array<T, AllocationType>& data)
    {
        Init(data.Get(), data.Count() * sizeof(T));
    }

    /// <summary>
    /// Gets the current handle to position in buffer.
    /// </summary>
    const byte* GetPositionHandle() const
    {
        return _position;
    }

public:
    /// <summary>
    /// Skips the data from the target buffer without reading from it. Moves the read pointer in the buffer forward.
    /// </summary>
    /// <param name="bytes">The amount of bytes to read.</param>
    /// <returns>The pointer to the data in memory.</returns>
    void* Move(uint32 bytes);

    /// <summary>
    /// Skips the data from the target buffer without reading from it. Moves the read pointer in the buffer forward.
    /// </summary>
    /// <returns>The pointer to the data in memory.</returns>
    template<typename T>
    FORCE_INLINE T* Move()
    {
        return static_cast<T*>(Move(sizeof(T)));
    }

    /// <summary>
    /// Skips the data from the target buffer without reading from it. Moves the read pointer in the buffer forward.
    /// </summary>
    /// <param name="count">The amount of items to read.</param>
    /// <returns>The pointer to the data in memory.</returns>
    template<typename T>
    FORCE_INLINE T* Move(uint32 count)
    {
        return static_cast<T*>(Move(sizeof(T) * count));
    }

public:
    // [ReadStream]
    void Flush() override;
    void Close() override;
    uint32 GetLength() override;
    uint32 GetPosition() override;
    void SetPosition(uint32 seek) override;
    void ReadBytes(void* data, uint32 bytes) override;
};
