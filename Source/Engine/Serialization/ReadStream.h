// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Stream.h"
#include "Engine/Core/Templates.h"

struct CommonValue;
struct Variant;
struct VariantType;
class ISerializable;

/// <summary>
/// Base class for all data read streams
/// </summary>
class FLAXENGINE_API ReadStream : public Stream
{
public:
    /// <summary>
    /// Reads bytes from the stream
    /// </summary>
    /// <param name="data">Data to read</param>
    /// <param name="bytes">Amount of bytes to read</param>
    virtual void ReadBytes(void* data, uint32 bytes) = 0;

    template<typename T>
    FORCE_INLINE void Read(T* data)
    {
        ReadBytes((void*)data, sizeof(T));
    }

    template<typename T>
    FORCE_INLINE void Read(T* data, int32 count)
    {
        ReadBytes((void*)data, sizeof(T) * count);
    }

public:
    // Reads byte from the stream
    // @returns Single byte
    byte ReadByte()
    {
        byte data;
        ReadBytes(&data, 1);
        return data;
    }

    // Reads bool from the stream
    bool ReadBool()
    {
        byte data;
        ReadBytes(&data, 1);
        return data != 0;
    }

    // Reads char from the stream
    // @param data Data to read
    char ReadChar()
    {
        char data;
        ReadBytes(&data, 1);
        return data;
    }

    FORCE_INLINE void ReadByte(byte* data)
    {
        Read(data);
    }

    // Reads Char from the stream
    // @param data Data to read
    FORCE_INLINE void ReadChar(Char* data)
    {
        Read(data);
    }

    // Reads uint8 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadUint8(uint8* data)
    {
        Read(data);
    }

    // Reads int8 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadInt8(int8* data)
    {
        Read(data);
    }

    // Reads uint16 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadUint16(uint16* data)
    {
        Read(data);
    }

    // Reads int16 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadInt16(int16* data)
    {
        Read(data);
    }

    // Reads uint32 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadUint32(uint32* data)
    {
        Read(data);
    }

    // Reads int32 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadInt32(int32* data)
    {
        Read(data);
    }

    // Reads uint64 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadUint64(uint64* data)
    {
        Read(data);
    }

    // Reads int64 from the stream
    // @param data Data to read
    FORCE_INLINE void ReadInt64(int64* data)
    {
        Read(data);
    }

    // Reads float from the stream
    // @param data Data to read
    FORCE_INLINE void ReadFloat(float* data)
    {
        Read(data);
    }

    // Reads double from the stream
    // @param data Data to read
    FORCE_INLINE void ReadDouble(double* data)
    {
        Read(data);
    }

public:
    // Reads StringAnsi from the stream
    // @param data Data to read
    void ReadStringAnsi(StringAnsi* data);

    // Reads StringAnsi from the stream with a key
    // @param data Data to read
    void ReadStringAnsi(StringAnsi* data, int8 lock);

    // Reads String from the stream
    // @param data Data to read
    void ReadString(String* data);

    // Reads String from the stream
    // @param data Data to read
    // @param lock Characters pass in the stream
    void ReadString(String* data, int16 lock);

public:
    // Reads CommonValue from the stream
    // @param data Data to read
    void ReadCommonValue(CommonValue* data);

    // Reads VariantType from the stream
    // @param data Data to read
    void ReadVariantType(VariantType* data);

    // Reads Variant from the stream
    // @param data Data to read
    void ReadVariant(Variant* data);

    /// <summary>
    /// Read data array
    /// </summary>
    /// <param name="data">Array to read</param>
    template<typename T, typename AllocationType = HeapAllocation>
    void ReadArray(Array<T, AllocationType>* data)
    {
        static_assert(TIsPODType<T>::Value, "Only POD types are valid for ReadArray.");
        int32 size;
        ReadInt32(&size);
        data->Resize(size, false);
        if (size > 0)
            ReadBytes(data->Get(), size * sizeof(T));
    }

    /// <summary>
    /// Deserializes object from Json by reading it as a raw data (ver+length+bytes).
    /// </summary>
    /// <remarks>Reads version number, data length and actual data bytes from the stream.</remarks>
    /// <param name="obj">The object to deserialize.</param>
    void ReadJson(ISerializable* obj);

public:
    // Deserialization of math types with float or double depending on the context (must match serialization)
    // Set useDouble=true to explicitly use 64-bit precision for serialized data
    void ReadBoundingBox(BoundingBox* box, bool useDouble = false);
    void ReadBoundingSphere(BoundingSphere* sphere, bool useDouble = false);
    void ReadTransform(Transform* transform, bool useDouble = false);
    void ReadRay(Ray* ray, bool useDouble = false);

public:
    // [Stream]
    bool CanRead() override
    {
        return GetPosition() != GetLength();
    }
};
