// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Stream.h"
#include "Engine/Core/Templates.h"

extern FLAXENGINE_API class ScriptingObject* FindObject(const Guid& id, class MClass* type);
extern FLAXENGINE_API class Asset* LoadAsset(const Guid& id, const struct ScriptingTypeHandle& type);

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

public:
    byte ReadByte()
    {
        byte data;
        ReadBytes(&data, 1);
        return data;
    }

    bool ReadBool()
    {
        byte data;
        ReadBytes(&data, 1);
        return data != 0;
    }

    char ReadChar()
    {
        char data;
        ReadBytes(&data, 1);
        return data;
    }

    FORCE_INLINE void ReadByte(byte* data)
    {
        ReadBytes(data, sizeof(byte));
    }

    FORCE_INLINE void ReadChar(Char* data)
    {
        ReadBytes(data, sizeof(Char));
    }

    FORCE_INLINE void ReadUint8(uint8* data)
    {
        ReadBytes(data, sizeof(uint8));
    }

    FORCE_INLINE void ReadInt8(int8* data)
    {
        ReadBytes(data, sizeof(int8));
    }

    FORCE_INLINE void ReadUint16(uint16* data)
    {
        ReadBytes(data, sizeof(uint16));
    }

    FORCE_INLINE void ReadInt16(int16* data)
    {
        ReadBytes(data, sizeof(int16));
    }

    FORCE_INLINE void ReadUint32(uint32* data)
    {
        ReadBytes(data, sizeof(uint32));
    }

    FORCE_INLINE void ReadInt32(int32* data)
    {
        ReadBytes(data, sizeof(int32));
    }

    FORCE_INLINE void ReadUint64(uint64* data)
    {
        ReadBytes(data, sizeof(uint64));
    }

    FORCE_INLINE void ReadInt64(int64* data)
    {
        ReadBytes(data, sizeof(int64));
    }

    FORCE_INLINE void ReadFloat(float* data)
    {
        ReadBytes(data, sizeof(float));
    }

    FORCE_INLINE void ReadDouble(double* data)
    {
        ReadBytes(data, sizeof(double));
    }

public:
    void Read(String& data);
    void Read(String& data, int16 lock);
    void Read(StringAnsi& data);
    void Read(StringAnsi& data, int8 lock);
    void Read(CommonValue& data);
    void Read(VariantType& data);
    void Read(Variant& data);

    template<typename T>
    FORCE_INLINE typename TEnableIf<TIsPODType<T>::Value>::Type Read(T& data)
    {
        ReadBytes((void*)&data, sizeof(T));
    }

    template<typename T>
    typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Read(T*& data)
    {
        uint32 id[4];
        ReadBytes(id, sizeof(id));
        data = (T*)::FindObject(*(Guid*)id, T::GetStaticClass());
    }

    template<typename T>
    FORCE_INLINE void Read(ScriptingObjectReference<T>& v)
    {
        T* ptr;
        Read(ptr);
        v = ptr;
    }

    template<typename T>
    FORCE_INLINE void Read(SoftObjectReference<T>& v)
    {
        uint32 id[4];
        ReadBytes(id, sizeof(id));
        v.Set(*(Guid*)id);
    }

    template<typename T>
    FORCE_INLINE void Read(AssetReference<T>& v)
    {
        uint32 id[4];
        ReadBytes(id, sizeof(id));
        v = (T*)::LoadAsset(*(Guid*)id, T::TypeInitializer);
    }

    template<typename T>
    FORCE_INLINE void Read(WeakAssetReference<T>& v)
    {
        uint32 id[4];
        ReadBytes(id, sizeof(id));
        v = (T*)::LoadAsset(*(Guid*)id, T::TypeInitializer);
    }

    template<typename T>
    FORCE_INLINE void Read(SoftAssetReference<T>& v)
    {
        uint32 id[4];
        ReadBytes(id, sizeof(id));
        v.Set(*(Guid*)id);
    }

    /// <summary>
    /// Reads array.
    /// </summary>
    /// <param name="data">Array to read.</param>
    template<typename T, typename AllocationType = HeapAllocation>
    void Read(Array<T, AllocationType>& data)
    {
        int32 size;
        ReadInt32(&size);
        data.Resize(size, false);
        if (size > 0)
        {
            if (TIsPODType<T>::Value && !TIsPointer<T>::Value)
                ReadBytes(data.Get(), size * sizeof(T));
            else
            {
                for (int32 i = 0; i < size; i++)
                    Read(data[i]);
            }
        }
    }

    /// <summary>
    /// Reads dictionary.
    /// </summary>
    /// <param name="data">Dictionary to read.</param>
    template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
    void Read(Dictionary<KeyType, ValueType, AllocationType>& data)
    {
        int32 count;
        ReadInt32(&count);
        data.Clear();
        data.EnsureCapacity(count);
        for (int32 i = 0; i < count; i++)
        {
            KeyType key;
            Read(key);
            Read(data[key]);
        }
    }

    /// <summary>
    /// Deserializes object from Json by reading it as a raw data (ver+length+bytes).
    /// </summary>
    /// <remarks>Reads version number, data length and actual data bytes from the stream.</remarks>
    /// <param name="obj">The object to deserialize.</param>
    void ReadJson(ISerializable* obj);

    // Deserialization of math types with float or double depending on the context (must match serialization)
    // Set useDouble=true to explicitly use 64-bit precision for serialized data
    void Read(BoundingBox& box, bool useDouble = false);
    void Read(BoundingSphere& sphere, bool useDouble = false);
    void Read(Transform& transform, bool useDouble = false);
    void Read(Ray& ray, bool useDouble = false);

public:
    // Reads StringAnsi from the stream
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadStringAnsi(StringAnsi* data);

    // Reads StringAnsi from the stream with a key
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadStringAnsi(StringAnsi* data, int8 lock);

    // Reads String from the stream
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadString(String* data);

    // Reads String from the stream
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadString(String* data, int16 lock);

    // Reads CommonValue from the stream
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadCommonValue(CommonValue* data);

    // Reads VariantType from the stream
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadVariantType(VariantType* data);

    // Reads Variant from the stream
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    DEPRECATED("Use Read method") void ReadVariant(Variant* data);

    /// <summary>
    /// Read data array
    /// [Deprecated on 11.10.2022, expires on 11.10.2024]
    /// </summary>
    /// <param name="data">Array to read</param>
    template<typename T, typename AllocationType = HeapAllocation>
    DEPRECATED("Use Read method") void ReadArray(Array<T, AllocationType>* data)
    {
        Read(*data);
    }

    // Deserialization of math types with float or double depending on the context (must match serialization)
    // Set useDouble=true to explicitly use 64-bit precision for serialized data
    // [Deprecated in v1.10]
    DEPRECATED("Use Read method") void ReadBoundingBox(BoundingBox* box, bool useDouble = false);
    // [Deprecated in v1.10]
    DEPRECATED("Use Read method") void ReadBoundingSphere(BoundingSphere* sphere, bool useDouble = false);
    // [Deprecated in v1.10]
    DEPRECATED("Use Read method") void ReadTransform(Transform* transform, bool useDouble = false);
    // [Deprecated in v1.10]
    DEPRECATED("Use Read method") void ReadRay(Ray* ray, bool useDouble = false);

public:
    // [Stream]
    bool CanRead() override
    {
        return GetPosition() != GetLength();
    }
};
