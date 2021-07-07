// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"

class Asset;
class ScriptingObject;
struct ScriptingType;
struct Transform;
struct CommonValue;

/// <summary>
/// Represents an object type that can be interpreted as more than one type.
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API VariantType
{
    enum Types
    {
        Null = 0,
        Void,

        Bool,
        Int,
        Uint,
        Int64,
        Uint64,
        Float,
        Double,
        Pointer,

        String,
        Object,
        Structure,
        Asset,
        Blob,
        Enum,

        Vector2,
        Vector3,
        Vector4,
        Color,
        Guid,
        BoundingBox,
        BoundingSphere,
        Quaternion,
        Transform,
        Rectangle,
        Ray,
        Matrix,

        Array,
        Dictionary,
        ManagedObject,
        Typename,

        Int2,
        Int3,
        Int4,

        Int16,
        Uint16,

        MAX
    };

public:

    /// <summary>
    /// The type of the variant.
    /// </summary>
    Types Type;

    /// <summary>
    /// The optional additional full name of the scripting type. Used for Asset, Object, Enum, Structure types to describe type precisely.
    /// </summary>
    char* TypeName;

public:

    FORCE_INLINE VariantType()
    {
        Type = Null;
        TypeName = nullptr;
    }

    FORCE_INLINE explicit VariantType(Types type)
    {
        Type = type;
        TypeName = nullptr;
    }

    explicit VariantType(Types type, const StringView& typeName);
    explicit VariantType(Types type, const StringAnsiView& typeName);
    VariantType(const VariantType& other);
    VariantType(VariantType&& other) noexcept;

    FORCE_INLINE ~VariantType()
    {
        Allocator::Free(TypeName);
    }

public:

    VariantType& operator=(const Types& type);
    VariantType& operator=(VariantType&& other);
    VariantType& operator=(const VariantType& other);
    bool operator==(const Types& type) const;
    bool operator==(const VariantType& other) const;

    FORCE_INLINE bool operator!=(const VariantType& other) const
    {
        return !operator==(other);
    }

public:

    void SetTypeName(const StringView& typeName);
    void SetTypeName(const StringAnsiView& typeName);
    const char* GetTypeName() const;
    ::String ToString() const;
};

FLAXENGINE_API uint32 GetHash(const VariantType& key);

DEFINE_DEFAULT_FORMATTING_VIA_TO_STRING(VariantType);

/// <summary>
/// Represents an object that can be interpreted as more than one type.
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API Variant
{
    /// <summary>
    /// Thee value type.
    /// </summary>
    VariantType Type;

    union
    {
        bool AsBool;
        int16 AsInt16;
        uint16 AsUint16;
        int32 AsInt;
        uint32 AsUint;
        int64 AsInt64;
        uint64 AsUint64;
        float AsFloat;
        double AsDouble;
        void* AsPointer;

        ScriptingObject* AsObject;
        Asset* AsAsset;

        struct
        {
            void* Data;
            int32 Length;
        } AsBlob;

        Dictionary<Variant, Variant, HeapAllocation>* AsDictionary;

        byte AsData[16];
    };

public:

    // 0.0f (floating-point value type)
    static const Variant Zero;

    // 1.0f (floating-point value type)
    static const Variant One;

    // nullptr (pointer value type)
    static const Variant Null;

    // false (boolean value type)
    static const Variant False;

    // true (boolean value type)
    static const Variant True;

public:

    FORCE_INLINE Variant()
    {
    }

    Variant(const Variant& other);
    Variant(Variant&& other) noexcept;

    Variant(bool v);
    Variant(int16 v);
    Variant(uint16 v);
    Variant(int32 v);
    Variant(uint32 v);
    Variant(int64 v);
    Variant(uint64 v);
    Variant(float v);
    Variant(double v);
    Variant(void* v);
    Variant(ScriptingObject* v);
    Variant(Asset* v);
    Variant(struct _MonoObject* v);
    Variant(const StringView& v);
    Variant(const StringAnsiView& v);
    Variant(const Char* v);
    Variant(const char* v);
    Variant(const Guid& v);
    Variant(const Vector2& v);
    Variant(const Vector3& v);
    Variant(const Vector4& v);
    Variant(const Int2& v);
    Variant(const Int3& v);
    Variant(const Int4& v);
    Variant(const Color& v);
    Variant(const Quaternion& v);
    Variant(const BoundingSphere& v);
    Variant(const Rectangle& v);
    explicit Variant(const BoundingBox& v);
    explicit Variant(const Transform& v);
    explicit Variant(const Ray& v);
    explicit Variant(const Matrix& v);
    Variant(Array<Variant, HeapAllocation>&& v);
    Variant(const Array<Variant, HeapAllocation>& v);
    explicit Variant(const Dictionary<Variant, Variant, HeapAllocation>& v);
    explicit Variant(const CommonValue& v);

    ~Variant();

public:

    Variant& operator=(Variant&& other);
    Variant& operator=(const Variant& other);
    bool operator==(const Variant& other) const;
    bool operator<(const Variant& other) const;

    FORCE_INLINE bool operator>(const Variant& other) const
    {
        return !operator==(other) && !operator<(other);
    }

    FORCE_INLINE bool operator>=(const Variant& other) const
    {
        return !operator<(other);
    }

    FORCE_INLINE bool operator<=(const Variant& other) const
    {
        return !operator>(other);
    }

    FORCE_INLINE bool operator!=(const Variant& other) const
    {
        return !operator==(other);
    }

public:

    explicit operator bool() const;
    explicit operator Char() const;
    explicit operator int8() const;
    explicit operator int16() const;
    explicit operator int32() const;
    explicit operator int64() const;
    explicit operator uint8() const;
    explicit operator uint16() const;
    explicit operator uint32() const;
    explicit operator uint64() const;
    explicit operator float() const;
    explicit operator double() const;
    explicit operator void*() const;
    explicit operator StringView() const;
    explicit operator StringAnsiView() const;
    explicit operator ScriptingObject*() const;
    explicit operator Asset*() const;
    explicit operator Vector2() const;
    explicit operator Vector3() const;
    explicit operator Vector4() const;
    explicit operator Int2() const;
    explicit operator Int3() const;
    explicit operator Int4() const;
    explicit operator Color() const;
    explicit operator Quaternion() const;
    explicit operator Guid() const;
    explicit operator BoundingSphere() const;
    explicit operator BoundingBox() const;
    explicit operator Transform() const;
    explicit operator Matrix() const;
    explicit operator Ray() const;
    explicit operator Rectangle() const;

    const Vector2& AsVector2() const;
    const Vector3& AsVector3() const;
    const Vector4& AsVector4() const;
    const Int2& AsInt2() const;
    const Int3& AsInt3() const;
    const Int4& AsInt4() const;
    const Color& AsColor() const;
    const Quaternion& AsQuaternion() const;

public:

    void SetType(const VariantType& type);
    void SetType(VariantType&& type);
    void SetString(const StringView& str);
    void SetString(const StringAnsiView& str);
    void SetTypename(const StringView& typeName);
    void SetTypename(const StringAnsiView& typeName);
    void SetBlob(int32 length);
    void SetBlob(const void* data, int32 length);
    void SetObject(ScriptingObject* object);
    void SetAsset(Asset* asset);
    String ToString() const;

    FORCE_INLINE Variant Cast(const VariantType& to) const
    {
        return Cast(*this, to);
    }

public:

    template<typename T>
    static typename TEnableIf<TIsEnum<T>::Value, Variant>::Type Enum(VariantType&& type, const T value)
    {
        ASSERT_LOW_LAYER(type.Type == VariantType::Enum);
        Variant v;
        v.SetType(MoveTemp(type));
        v.AsUint64 = (uint64)value;
        return MoveTemp(v);
    }

    template<typename T>
    static typename TEnableIf<!TIsEnum<T>::Value && !TIsPointer<T>::Value, Variant>::Type Structure(VariantType&& type, const T& value)
    {
        ASSERT_LOW_LAYER(type.Type == VariantType::Structure);
        Variant v;
        v.SetType(MoveTemp(type));
        v.CopyStructure((void*)&value);
        return MoveTemp(v);
    }

    static bool CanCast(const Variant& v, const VariantType& to);
    static Variant Cast(const Variant& v, const VariantType& to);
    static bool NearEqual(const Variant& a, const Variant& b, float epsilon = 1e-6f);
    static Variant Lerp(const Variant& a, const Variant& b, float alpha);

private:

    void OnObjectDeleted(ScriptingObject* obj)
    {
        AsObject = nullptr;
    }

    void OnAssetUnloaded(Asset* obj)
    {
        AsAsset = nullptr;
    }

    void AllocStructure();
    void CopyStructure(void* src);
    void FreeStructure();
};

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Variant& a, const Variant& b)
    {
        return Variant::NearEqual(a, b);
    }
}

FLAXENGINE_API uint32 GetHash(const Variant& key);

DEFINE_DEFAULT_FORMATTING_VIA_TO_STRING(Variant);
