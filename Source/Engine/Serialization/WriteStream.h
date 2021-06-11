// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Formatting.h"
#include "Stream.h"

struct CommonValue;
struct Variant;
struct VariantType;

/// <summary>
/// Base class for all data write streams
/// </summary>
class FLAXENGINE_API WriteStream : public Stream
{
public:

    /// <summary>
    /// Virtual destructor
    /// </summary>
    virtual ~WriteStream()
    {
    }

public:

    /// <summary>
    /// Writes bytes to the stream
    /// </summary>
    /// <param name="data">Data to write</param>
    /// <param name="bytes">Amount of bytes to write</param>
    virtual void WriteBytes(const void* data, uint32 bytes) = 0;

public:

    template<typename T>
    FORCE_INLINE void Write(const T* data)
    {
        WriteBytes((const void*)data, sizeof(T));
    }

    template<typename T>
    FORCE_INLINE void Write(const T* data, int32 count)
    {
        WriteBytes((const void*)data, sizeof(T) * count);
    }

public:

    // Writes byte to the stream
    // @param data Data to write
    FORCE_INLINE void WriteByte(byte data)
    {
        Write(&data);
    }

    // Writes bool to the stream
    // @param data Data to write
    FORCE_INLINE void WriteBool(bool data)
    {
        Write(&data);
    }

    // Writes char to the stream
    // @param data Data to write
    FORCE_INLINE void WriteChar(char data)
    {
        Write(&data);
    }

    // Writes Char to the stream
    // @param data Data to write
    FORCE_INLINE void WriteChar(Char data)
    {
        Write(&data);
    }

    // Writes uint8 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteUint8(uint8 data)
    {
        Write(&data);
    }

    // Writes int8 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteInt8(int8 data)
    {
        Write(&data);
    }

    // Writes uint16 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteUint16(uint16 data)
    {
        Write(&data);
    }

    // Writes int16 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteInt16(int16 data)
    {
        Write(&data);
    }

    // Writes uint32 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteUint32(uint32 data)
    {
        Write(&data);
    }

    // Writes int32 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteInt32(int32 data)
    {
        Write(&data);
    }

    // Writes int64 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteInt64(int64 data)
    {
        Write(&data);
    }

    // Writes uint64 to the stream
    // @param data Data to write
    FORCE_INLINE void WriteUint64(uint64 data)
    {
        Write(&data);
    }

    // Writes float to the stream
    // @param data Data to write
    FORCE_INLINE void WriteFloat(float data)
    {
        Write(&data);
    }

    // Writes double to the stream
    // @param data Data to write
    FORCE_INLINE void WriteDouble(double data)
    {
        Write(&data);
    }

public:

    // Writes text to the stream
    // @param data Text to write
    // @param length Text length
    void WriteText(const char* text, int32 length)
    {
        WriteBytes((const void*)text, sizeof(char) * length);
    }

    // Writes text to the stream
    // @param data Text to write
    // @param length Text length
    void WriteText(const Char* text, int32 length)
    {
        WriteBytes((const void*)text, sizeof(Char) * length);
    }

    template<typename... Args>
    void WriteTextFormatted(const char* format, const Args& ... args)
    {
        fmt_flax::allocator_ansi allocator;
        fmt_flax::memory_buffer_ansi buffer(allocator);
        fmt_flax::format(buffer, format, args...);
        WriteText(buffer.data(), (int32)buffer.size());
    }

    template<typename... Args>
    void WriteTextFormatted(const Char* format, const Args& ... args)
    {
        fmt_flax::allocator allocator;
        fmt_flax::memory_buffer buffer(allocator);
        fmt_flax::format(buffer, format, args...);
        WriteText(buffer.data(), (int32)buffer.size());
    }

    // Write UTF BOM character sequence
    void WriteBOM()
    {
        WriteByte(0xEF);
        WriteByte(0xBB);
        WriteByte(0xBF);
    }

    // Writes text to the stream
    // @param data Text to write
    void WriteText(const StringView& text);

    // Writes String to the stream
    // @param data Data to write
    void WriteString(const StringView& data);

    // Writes String to the stream
    // @param data Data to write
    // @param lock Characters pass in the stream
    void WriteString(const StringView& data, int16 lock);

    // Writes Ansi String to the stream
    // @param data Data to write
    void WriteStringAnsi(const StringAnsiView& data);

    // Writes Ansi String to the stream
    // @param data Data to write
    // @param lock Characters pass in the stream
    void WriteStringAnsi(const StringAnsiView& data, int16 lock);

public:

    // Writes CommonValue to the stream
    // @param data Data to write
    void WriteCommonValue(const CommonValue& data);

    // Writes VariantType to the stream
    // @param data Data to write
    void WriteVariantType(const VariantType& data);

    // Writes Variant to the stream
    // @param data Data to write
    void WriteVariant(const Variant& data);

    /// <summary>
    /// Write data array
    /// </summary>
    /// <param name="data">Array to write</param>
    template<typename T>
    void WriteArray(const Array<T>& data)
    {
        static_assert(TIsPODType<T>::Value, "Only POD types are valid for WriteArray.");
        const int32 size = data.Count();
        WriteInt32(size);
        if (size > 0)
            WriteBytes(data.Get(), size * sizeof(T));
    }

public:

    // [Stream]
    bool CanWrite() override
    {
        return true;
    }
};
