// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "SerializationFwd.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Scripting/SoftObjectReference.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/WeakAssetReference.h"

struct Version;
struct VariantType;

// @formatter:off

namespace Serialization
{
    // In-build types
    
    inline bool ShouldSerialize(const bool& v, const void* otherObj)
    {
        return !otherObj || v != *(bool*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const bool& v, const void* otherObj)
    {
        stream.Bool(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, bool& v, ISerializeModifier* modifier)
    {
        v = stream.GetBool();
    }
    
    inline bool ShouldSerialize(const int8& v, const void* otherObj)
    {
        return !otherObj || v != *(int8*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const int8& v, const void* otherObj)
    {
        stream.Int(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, int8& v, ISerializeModifier* modifier)
    {
        v = stream.GetInt();
    }
    
    inline bool ShouldSerialize(const char& v, const void* otherObj)
    {
        return !otherObj || v != *(char*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const char& v, const void* otherObj)
    {
        stream.Int(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, char& v, ISerializeModifier* modifier)
    {
        v = (char)stream.GetInt();
    }
    
    inline bool ShouldSerialize(const Char& v, const void* otherObj)
    {
        return !otherObj || v != *(Char*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const Char& v, const void* otherObj)
    {
        stream.Int(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, Char& v, ISerializeModifier* modifier)
    {
        v = (Char)stream.GetInt();
    }

    inline bool ShouldSerialize(const int16& v, const void* otherObj)
    {
        return !otherObj || v != *(int16*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const int16& v, const void* otherObj)
    {
        stream.Int(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, int16& v, ISerializeModifier* modifier)
    {
        v = stream.GetInt();
    }

    inline bool ShouldSerialize(const int32& v, const void* otherObj)
    {
        return !otherObj || v != *(int32*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const int32& v, const void* otherObj)
    {
        stream.Int(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, int32& v, ISerializeModifier* modifier)
    {
        v = stream.GetInt();
    }

    inline bool ShouldSerialize(const int64& v, const void* otherObj)
    {
        return !otherObj || v != *(int64*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const int64& v, const void* otherObj)
    {
        stream.Int64(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, int64& v, ISerializeModifier* modifier)
    {
        v = stream.GetInt64();
    }

    inline bool ShouldSerialize(const uint8& v, const void* otherObj)
    {
        return !otherObj || v != *(uint8*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const uint8& v, const void* otherObj)
    {
        stream.Uint(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, uint8& v, ISerializeModifier* modifier)
    {
        v = stream.GetUint();
    }

    inline bool ShouldSerialize(const uint16& v, const void* otherObj)
    {
        return !otherObj || v != *(uint16*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const uint16& v, const void* otherObj)
    {
        stream.Uint(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, uint16& v, ISerializeModifier* modifier)
    {
        v = stream.GetUint();
    }

    inline bool ShouldSerialize(const uint32& v, const void* otherObj)
    {
        return !otherObj || v != *(uint32*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const uint32& v, const void* otherObj)
    {
        stream.Uint(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, uint32& v, ISerializeModifier* modifier)
    {
        v = stream.GetUint();
    }

    inline bool ShouldSerialize(const uint64& v, const void* otherObj)
    {
        return !otherObj || v != *(uint64*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const uint64& v, const void* otherObj)
    {
        stream.Uint64(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, uint64& v, ISerializeModifier* modifier)
    {
        v = stream.GetUint64();
    }

    inline bool ShouldSerialize(const float& v, const void* otherObj)
    {
        return !otherObj || Math::Abs(v - *(float*)otherObj) > SERIALIZE_EPSILON;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const float& v, const void* otherObj)
    {
        stream.Float(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, float& v, ISerializeModifier* modifier)
    {
        v = stream.GetFloat();
    }

    inline bool ShouldSerialize(const double& v, const void* otherObj)
    {
        return !otherObj || v != *(double*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const double& v, const void* otherObj)
    {
        stream.Double(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, double& v, ISerializeModifier* modifier)
    {
        v = stream.GetDouble();
    }
    
    // Enum

    template<typename T>
    inline typename TEnableIf<TIsEnum<T>::Value, bool>::Type ShouldSerialize(const T& v, const void* otherObj)
    {
        return !otherObj || v != *(T*)otherObj;
    }
    template<typename T>
    inline typename TEnableIf<TIsEnum<T>::Value>::Type Serialize(ISerializable::SerializeStream& stream, const T& v, const void* otherObj)
    {
        stream.Uint((uint32)v);
    }
    template<typename T>
    inline typename TEnableIf<TIsEnum<T>::Value>::Type Deserialize(ISerializable::DeserializeStream& stream, T& v, ISerializeModifier* modifier)
    {
        v = (T)stream.GetInt();
    }

    // Common types

    FLAXENGINE_API bool ShouldSerialize(const VariantType& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const VariantType& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, VariantType& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Variant& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const Variant& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Variant& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Guid& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const Guid& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Guid& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const DateTime& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const DateTime& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, DateTime& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const TimeSpan& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const TimeSpan& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, TimeSpan& v, ISerializeModifier* modifier);

    inline bool ShouldSerialize(const String& v, const void* otherObj)
    {
        return !otherObj || v != *(String*)otherObj;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, const String& v, const void* otherObj)
    {
        stream.String(v);
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, String& v, ISerializeModifier* modifier)
    {
        v = stream.GetText();
    }

    FLAXENGINE_API bool ShouldSerialize(const Version& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const Version& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Version& v, ISerializeModifier* modifier);

    // Math types

    FLAXENGINE_API bool ShouldSerialize(const Vector2& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Vector2& v, const void* otherObj)
    {
        stream.Vector2(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Vector2& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Vector3& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Vector3& v, const void* otherObj)
    {
        stream.Vector3(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Vector3& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Vector4& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Vector4& v, const void* otherObj)
    {
        stream.Vector4(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Vector4& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Quaternion& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Quaternion& v, const void* otherObj)
    {
        stream.Quaternion(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Quaternion& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Color& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Color& v, const void* otherObj)
    {
        stream.Color(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Color& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Color32& v, const void* otherObj);
    FLAXENGINE_API void Serialize(ISerializable::SerializeStream& stream, const Color32& v, const void* otherObj);
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Color32& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const BoundingBox& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const BoundingBox& v, const void* otherObj)
    {
        stream.BoundingBox(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, BoundingBox& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const BoundingSphere& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const BoundingSphere& v, const void* otherObj)
    {
        stream.BoundingSphere(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, BoundingSphere& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Ray& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Ray& v, const void* otherObj)
    {
        stream.Ray(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Ray& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Rectangle& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Rectangle& v, const void* otherObj)
    {
        stream.Rectangle(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Rectangle& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Transform& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Transform& v, const void* otherObj)
    {
        stream.Transform(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Transform& v, ISerializeModifier* modifier);

    FLAXENGINE_API bool ShouldSerialize(const Matrix& v, const void* otherObj);
    inline void Serialize(ISerializable::SerializeStream& stream, const Matrix& v, const void* otherObj)
    {
        stream.Matrix(v);
    }
    FLAXENGINE_API void Deserialize(ISerializable::DeserializeStream& stream, Matrix& v, ISerializeModifier* modifier);

    // ISerializable

    inline bool ShouldSerialize(ISerializable& v, const void* otherObj)
    {
        return true;
    }
    inline void Serialize(ISerializable::SerializeStream& stream, ISerializable& v, const void* otherObj)
    {
        stream.StartObject();
        v.Serialize(stream, otherObj);
        stream.EndObject();
    }
    inline void Deserialize(ISerializable::DeserializeStream& stream, ISerializable& v, ISerializeModifier* modifier)
    {
        v.Deserialize(stream, modifier);
    }

    // Scripting Object

    template<typename T>
    inline typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value, bool>::Type ShouldSerialize(T*& v, const void* otherObj)
    {
        return !otherObj || v != *(T**)otherObj;
    }
    template<typename T>
    inline typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Serialize(ISerializable::SerializeStream& stream, T*& v, const void* otherObj)
    {
        stream.Guid(v ? v->GetID() : Guid::Empty);
    }
    template<typename T>
    inline typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Deserialize(ISerializable::DeserializeStream& stream, T*& v, ISerializeModifier* modifier)
    {
        Guid id;
        Deserialize(stream, id, modifier);
		modifier->IdsMapping.TryGet(id, id);
        v = (T*)FindObject(id, T::GetStaticClass());
    }

    // Scripting Object Reference

    template<typename T>
    inline bool ShouldSerialize(const ScriptingObjectReference<T>& v, const void* otherObj)
    {
        return !otherObj || v.Get() != ((ScriptingObjectReference<T>*)otherObj)->Get();
    }
    template<typename T>
    inline void Serialize(ISerializable::SerializeStream& stream, const ScriptingObjectReference<T>& v, const void* otherObj)
    {
        stream.Guid(v.GetID());
    }
    template<typename T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, ScriptingObjectReference<T>& v, ISerializeModifier* modifier)
    {
        Guid id;
        Deserialize(stream, id, modifier);
		modifier->IdsMapping.TryGet(id, id);
        v = id;
    }

    // Soft Object Reference

    template<typename T>
    inline bool ShouldSerialize(const SoftObjectReference<T>& v, const void* otherObj)
    {
        return !otherObj || v.Get() != ((SoftObjectReference<T>*)otherObj)->Get();
    }
    template<typename T>
    inline void Serialize(ISerializable::SerializeStream& stream, const SoftObjectReference<T>& v, const void* otherObj)
    {
        stream.Guid(v.GetID());
    }
    template<typename T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, SoftObjectReference<T>& v, ISerializeModifier* modifier)
    {
        Guid id;
        Deserialize(stream, id, modifier);
		modifier->IdsMapping.TryGet(id, id);
        v = id;
    }

    // Asset Reference

    template<typename T>
    inline bool ShouldSerialize(const AssetReference<T>& v, const void* otherObj)
    {
        return !otherObj || v.Get() != ((AssetReference<T>*)otherObj)->Get();
    }
    template<typename T>
    inline void Serialize(ISerializable::SerializeStream& stream, const AssetReference<T>& v, const void* otherObj)
    {
        stream.Guid(v.GetID());
    }
    template<typename T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, AssetReference<T>& v, ISerializeModifier* modifier)
    {
        Guid id;
        Deserialize(stream, id, modifier);
        v = id;
    }

    // Weak Asset Reference

    template<typename T>
    inline bool ShouldSerialize(const WeakAssetReference<T>& v, const void* otherObj)
    {
        return !otherObj || v.Get() != ((WeakAssetReference<T>*)otherObj)->Get();
    }
    template<typename T>
    inline void Serialize(ISerializable::SerializeStream& stream, const WeakAssetReference<T>& v, const void* otherObj)
    {
        stream.Guid(v.GetID());
    }
    template<typename T>
    inline void Deserialize(ISerializable::DeserializeStream& stream, WeakAssetReference<T>& v, ISerializeModifier* modifier)
    {
        Guid id;
        Deserialize(stream, id, modifier);
        v = id;
    }

    // Array

    template<typename T, typename AllocationType = HeapAllocation>
    inline bool ShouldSerialize(const Array<T, AllocationType>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (Array<T, AllocationType>*)otherObj;
        if (v.Count() != other->Count())
            return true;
        for (int32 i = 0; i < v.Count(); i++)
        {
            if (ShouldSerialize(v[i], &other->At(i)))
                return true;
        }
        return false;
    }
    template<typename T, typename AllocationType = HeapAllocation>
    inline void Serialize(ISerializable::SerializeStream& stream, const Array<T, AllocationType>& v, const void* otherObj)
    {
        stream.StartArray();
        for (int32 i = 0; i < v.Count(); i++)
            Serialize(stream, v[i], nullptr);
        stream.EndArray();
    }
    template<typename T, typename AllocationType = HeapAllocation>
    inline void Deserialize(ISerializable::DeserializeStream& stream, Array<T, AllocationType>& v, ISerializeModifier* modifier)
    {
        if (!stream.IsArray())
            return;
        const auto& streamArray = stream.GetArray();
        v.Resize(streamArray.Size());
        for (int32 i = 0; i < v.Count(); i++)
            Deserialize(streamArray[i], v[i], modifier);
    }

    // Dictionary

    template<typename KeyType, typename ValueType>
    inline bool ShouldSerialize(const Dictionary<KeyType, ValueType>& v, const void* otherObj)
    {
        if (!otherObj)
            return true;
        const auto other = (const Dictionary<KeyType, ValueType>*)otherObj;
        if (v.Count() != other->Count())
            return true;
        for (auto& i : v)
        {
            if (!other->ContainsKey(i.Key) || ShouldSerialize(i.Value, &other->At(i.Key)))
                return true;
        }
        return false;
    }
    template<typename KeyType, typename ValueType>
    inline void Serialize(ISerializable::SerializeStream& stream, const Dictionary<KeyType, ValueType>& v, const void* otherObj)
    {
        stream.StartArray();
        for (auto& i : v)
        {
            stream.StartObject();
            stream.JKEY("Key");
            Serialize(stream, i.Key, nullptr);
            stream.JKEY("Value");
            Serialize(stream, i.Value, nullptr);
            stream.EndObject();
        }
        stream.EndArray();
    }
    template<typename KeyType, typename ValueType>
    inline void Deserialize(ISerializable::DeserializeStream& stream, Dictionary<KeyType, ValueType>& v, ISerializeModifier* modifier)
    {
        if (!stream.IsArray())
            return;
        const auto& streamArray = stream.GetArray();
        v.Clear();
        v.EnsureCapacity(streamArray.Size() * 3);
        for (rapidjson::SizeType i = 0; i < streamArray.Size(); i++)
        {
            auto& streamItem = streamArray[i];
            const auto mKey = SERIALIZE_FIND_MEMBER(streamItem, "Key");
            const auto mValue = SERIALIZE_FIND_MEMBER(streamItem, "Value");
            if (mKey != streamItem.MemberEnd() && mValue != streamItem.MemberEnd())
            {
                KeyType key;
                Deserialize(mKey->value, key, modifier);
                Deserialize(mValue->value, v[key], modifier);
            }
        }
    }
}

// @formatter:on
