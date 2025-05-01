// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Core/Types/BaseTypes.h"

#define FILESTREAM_BUFFER_SIZE 4096
#define STREAM_MAX_STRING_LENGTH (4*1024) // 4 kB

class ReadStream;
class WriteStream;
struct CommonValue;
struct Variant;
struct VariantType;
class ISerializable;
class ScriptingObject;
template<typename T>
class ScriptingObjectReference;
template<typename T>
class SoftObjectReference;
template<typename T>
class AssetReference;
template<typename T>
class WeakAssetReference;
template<typename T>
class SoftAssetReference;

/// <summary>
/// Base class for all data streams (memory streams, file streams etc.)
/// </summary>
class FLAXENGINE_API Stream
{
protected:

    bool _hasError;

    Stream()
        : _hasError(false)
    {
    }

public:

    /// <summary>
    /// Virtual destructor
    /// </summary>
    virtual ~Stream()
    {
    }

public:

    /// <summary>
    /// Returns true if error occurred during reading/writing to the stream
    /// </summary>
    /// <returns>True if error occurred during reading/writing to the stream</returns>
    virtual bool HasError() const
    {
        return _hasError;
    }

    /// <summary>
    /// Returns true if can read data from that stream.
    /// </summary>
    /// <returns>True if can read, otherwise false.</returns>
    virtual bool CanRead()
    {
        return false;
    }

    /// <summary>
    /// Returns true if can write data to that stream.
    /// </summary>
    /// <returns>True if can write, otherwise false.</returns>
    virtual bool CanWrite()
    {
        return false;
    }

public:

    /// <summary>
    /// Flush the stream buffers
    /// </summary>
    virtual void Flush() = 0;

    /// <summary>
    /// Close the stream
    /// </summary>
    virtual void Close() = 0;

    /// <summary>
    /// Gets length of the stream
    /// </summary>
    /// <returns>Length of the stream</returns>
    virtual uint32 GetLength() = 0;

    /// <summary>
    /// Gets current position in the stream
    /// </summary>
    /// <returns>Current position in the stream</returns>
    virtual uint32 GetPosition() = 0;

    /// <summary>
    /// Set new position in the stream
    /// </summary>
    /// <param name="seek">New position in the stream</param>
    virtual void SetPosition(uint32 seek) = 0;
};
