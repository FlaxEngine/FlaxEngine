// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Serialization/ReadStream.h"
#include "Engine/Serialization/WriteStream.h"

/// <summary>
/// Objects and values serialization stream for sending data over network. Uses memory buffer for both read and write operations.
/// </summary>
API_CLASS(sealed, Namespace = "FlaxEngine.Networking") class FLAXENGINE_API NetworkStream final : public ScriptingObject, public ReadStream, public WriteStream
{
    DECLARE_SCRIPTING_TYPE(NetworkStream);
private:
    byte* _buffer = nullptr;
    byte* _position = nullptr;
    uint32 _length = 0;
    bool _allocated = false;

public:
    ~NetworkStream();

    /// <summary>
    /// Initializes the stream for writing. Allocates the memory or reuses already existing memory. Resets the current stream position to beginning.
    /// </summary>
    API_FUNCTION() void Initialize(uint32 minCapacity);

    /// <summary>
    /// Writes bytes to the stream
    /// </summary>
    /// <param name="data">Data to write</param>
    /// <param name="bytes">Amount of bytes to write</param>
    API_FUNCTION() FORCE_INLINE void WriteData(const void* data, int32 bytes)
    {
        WriteBytes(data, bytes);
    }

    /// <summary>
    /// Reads bytes from the stream
    /// </summary>
    /// <param name="data">Data to write</param>
    /// <param name="bytes">Amount of bytes to write</param>
    API_FUNCTION() FORCE_INLINE void ReadData(void* data, int32 bytes)
    {
        ReadBytes(data, bytes);
    }

public:
    // [Stream]
    void Flush() override;
    void Close() override;
    uint32 GetLength() override;
    uint32 GetPosition() override;
    void SetPosition(uint32 seek) override;

    // [ReadStream]
    void ReadBytes(void* data, uint32 bytes) override;

    // [WriteStream]
    void WriteBytes(const void* data, uint32 bytes) override;
};
