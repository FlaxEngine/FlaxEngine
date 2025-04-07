// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Serialization/ReadStream.h"
#include "Engine/Serialization/WriteStream.h"

class INetworkSerializable;

/// <summary>
/// Objects and values serialization stream for sending data over network. Uses memory buffer for both read and write operations.
/// </summary>
API_CLASS(sealed, Namespace="FlaxEngine.Networking") class FLAXENGINE_API NetworkStream final : public ScriptingObject, public ReadStream, public WriteStream
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
    /// The ClientId of the network client that is a data sender. Can be used to detect who send the incoming RPC or replication data. Set to the current client when writing data.
    /// </summary>
    API_FIELD(ReadOnly) uint32 SenderId = 0;

    /// <summary>
    /// Gets the pointer to the native stream memory buffer.
    /// </summary>
    API_PROPERTY() byte* GetBuffer() const
    {
        return _buffer;
    }

    /// <summary>
    /// Initializes the stream for writing. Allocates the memory or reuses already existing memory. Resets the current stream position to beginning.
    /// </summary>
    /// <param name="minCapacity">The minimum capacity (in bytes) for the memory buffer used for data storage.</param>
    API_FUNCTION() void Initialize(uint32 minCapacity = 1024);

    /// <summary>
    /// Initializes the stream for reading.
    /// </summary>
    /// <param name="buffer">The allocated memory.</param>
    /// <param name="length">The allocated memory length (bytes count).</param>
    API_FUNCTION() void Initialize(byte* buffer, uint32 length);

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

    using ReadStream::Read;
    void Read(INetworkSerializable& obj);
    void Read(INetworkSerializable* obj);
    void Read(Quaternion& data);
    void Read(Transform& data, bool useDouble = false);

    using WriteStream::Write;
    void Write(INetworkSerializable& obj);
    void Write(INetworkSerializable* obj);
    void Write(const Quaternion& data);
    void Write(const Transform& data, bool useDouble = false);

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
