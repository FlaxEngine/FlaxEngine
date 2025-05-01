// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Ray.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Core/Math/Matrix.h"

/// <summary>
/// Common values types.
/// [Deprecated on 31.07.2020, expires on 31.07.2022]
/// </summary>
enum class CommonType
{
    Bool,
    Integer,
    Float,
    Vector2,
    Vector3,
    Vector4,
    Color,
    Guid,
    String,
    Box,
    Rotation,
    Transform,
    Sphere,
    Rectangle,
    Pointer,
    Matrix,
    Blob,
    Object,
    Ray,
};

const Char* ToString(CommonType value);

class ScriptingObject;

/// <summary>
/// Container for value that can have different types
/// [Deprecated on 31.07.2020, expires on 31.07.2022]
/// </summary>
struct DEPRECATED("Use Variant.") FLAXENGINE_API CommonValue
{
public:
    /// <summary>
    /// Type
    /// </summary>
    CommonType Type;

    union
    {
        bool AsBool;
        int32 AsInteger;
        float AsFloat;
        Float2 AsVector2;
        Float3 AsVector3;
        Float4 AsVector4;
        Color AsColor;
        Guid AsGuid;
        Char* AsString;
        BoundingBox AsBox;
        Transform AsTransform;
        Quaternion AsRotation;
        BoundingSphere AsSphere;
        Rectangle AsRectangle;
        Ray AsRay;
        void* AsPointer;
        Matrix AsMatrix;
        ScriptingObject* AsObject;

        struct
        {
            byte* Data;
            int32 Length;
        } AsBlob;
    };

public:
    // 0.0f (floating-point value type)
    static const CommonValue Zero;

    // 1.0f (floating-point value type)
    static const CommonValue One;

    // nullptr (pointer value type)
    static const CommonValue Null;

    // false (bool value type)
    static const CommonValue False;

    // true (bool value type)
    static const CommonValue True;

public:
    /// <summary>
    /// Default constructor (bool)
    /// </summary>
    CommonValue()
        : Type(CommonType::Bool)
        , AsBool(false)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(bool value)
        : Type(CommonType::Bool)
        , AsBool(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(int32 value)
        : Type(CommonType::Integer)
        , AsInteger(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(float value)
        : Type(CommonType::Float)
        , AsFloat(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Float2& value)
        : Type(CommonType::Vector2)
        , AsVector2(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Float3& value)
        : Type(CommonType::Vector3)
        , AsVector3(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Float4& value)
        : Type(CommonType::Vector4)
        , AsVector4(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Color& value)
        : Type(CommonType::Color)
        , AsColor(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Matrix& value)
        : Type(CommonType::Matrix)
        , AsMatrix(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Guid& value)
        : Type(CommonType::Guid)
        , AsGuid(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const String& value)
        : Type(CommonType::Bool)
        , AsString(nullptr)
    {
        Set(value);
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const BoundingBox& value)
        : Type(CommonType::Box)
        , AsBox(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Transform& value)
        : Type(CommonType::Transform)
        , AsTransform(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Quaternion& value)
        : Type(CommonType::Rotation)
        , AsRotation(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const BoundingSphere& value)
        : Type(CommonType::Sphere)
        , AsSphere(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Rectangle& value)
        : Type(CommonType::Rectangle)
        , AsRectangle(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(const Ray& value)
        : Type(CommonType::Ray)
        , AsRay(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(void* value)
        : Type(CommonType::Pointer)
        , AsPointer(value)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="value">Value</param>
    CommonValue(ScriptingObject* value)
        : Type(CommonType::Object)
        , AsObject(value)
    {
        if (value)
            LinkObject();
    }

    CommonValue(byte* data, int32 length)
        : Type(CommonType::Blob)
    {
        AsBlob.Length = length;
        if (length)
        {
            AsBlob.Data = (byte*)Allocator::Allocate(length);
            Platform::MemoryCopy(AsBlob.Data, data, length);
        }
        else
        {
            AsBlob.Data = nullptr;
        }
    }

    /// <summary>
    /// Copy constructor
    /// </summary>
    /// <param name="other">The other value</param>
    CommonValue(const CommonValue& other)
    {
        if (other.Type == CommonType::String)
        {
            Type = CommonType::String;

            const int32 length = StringUtils::Length(other.AsString);
            if (length > 0)
            {
                AsString = (Char*)Allocator::Allocate((length + 1) * sizeof(Char));
                Platform::MemoryCopy(AsString, other.AsString, sizeof(Char) * length);
                AsString[length] = 0;
            }
            else
            {
                AsString = nullptr;
            }
        }
        else if (other.Type == CommonType::Blob)
        {
            Type = CommonType::Blob;

            const int32 length = other.AsBlob.Length;
            if (length > 0)
            {
                AsBlob.Data = (byte*)Allocator::Allocate(length);
                Platform::MemoryCopy(AsBlob.Data, other.AsBlob.Data, length);
                AsBlob.Length = length;
            }
            else
            {
                AsBlob.Data = nullptr;
                AsBlob.Length = 0;
            }
        }
        else
        {
            Platform::MemoryCopy(this, &other, sizeof(CommonValue));
        }
    }

    /// <summary>
    /// Destructor
    /// </summary>
    ~CommonValue()
    {
        SetType(CommonType::Bool);
    }

public:
    /// <summary>
    /// Assignment operator
    /// </summary>
    /// <param name="other">Other value</param>
    CommonValue& operator=(const CommonValue& other)
    {
        if (Type == CommonType::String && AsString)
        {
            Allocator::Free(AsString);
            AsString = nullptr;
        }
        else if (Type == CommonType::Blob && AsBlob.Data)
        {
            Allocator::Free(AsBlob.Data);
            AsBlob.Data = nullptr;
            AsBlob.Length = 0;
        }

        if (other.Type == CommonType::String)
        {
            Type = CommonType::String;

            const int32 length = StringUtils::Length(other.AsString);
            if (length > 0)
            {
                AsString = (Char*)Allocator::Allocate((length + 1) * sizeof(Char));
                Platform::MemoryCopy(AsString, other.AsString, sizeof(Char) * length);
                AsString[length] = 0;
            }
            else
            {
                AsString = nullptr;
            }
        }
        else if (other.Type == CommonType::Blob)
        {
            Type = CommonType::Blob;

            const int32 length = other.AsBlob.Length;
            if (length > 0)
            {
                AsBlob.Data = (byte*)Allocator::Allocate(length);
                Platform::MemoryCopy(AsBlob.Data, other.AsBlob.Data, length);
                AsBlob.Length = length;
            }
            else
            {
                AsBlob.Data = nullptr;
                AsBlob.Length = 0;
            }
        }
        else
        {
            Platform::MemoryCopy(this, &other, sizeof(CommonValue));
        }

        return *this;
    }

public:
    /// <summary>
    /// Gets value as boolean (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE bool GetBool() const
    {
        bool isValid;
        return GetBool(isValid);
    }

    /// <summary>
    /// Gets value as boolean (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    bool GetBool(bool& isValid) const
    {
        isValid = true;

        switch (Type)
        {
        case CommonType::Bool:
            return AsBool;
        case CommonType::Integer:
            return AsInteger != 0;
        case CommonType::Float:
            return !Math::IsZero(AsFloat);
        case CommonType::Vector2:
            return !Math::IsZero(AsVector2.X);
        case CommonType::Vector3:
            return !Math::IsZero(AsVector3.X);
        case CommonType::Vector4:
            return !Math::IsZero(AsVector4.X);
        case CommonType::Color:
            return !Math::IsZero(AsColor.R);
        }

        isValid = false;
        return false;
    }

    /// <summary>
    /// Gets value as integer (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE int32 GetInteger() const
    {
        bool isValid;
        return GetInteger(isValid);
    }

    /// <summary>
    /// Gets value as integer (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    int32 GetInteger(bool& isValid) const
    {
        isValid = true;

        switch (Type)
        {
        case CommonType::Bool:
            return AsBool ? 1 : 0;
        case CommonType::Integer:
            return AsInteger;
        case CommonType::Float:
            return static_cast<int32>(AsFloat);
        case CommonType::Vector2:
            return static_cast<int32>(AsVector2.X);
        case CommonType::Vector3:
            return static_cast<int32>(AsVector3.X);
        case CommonType::Vector4:
            return static_cast<int32>(AsVector4.X);
        case CommonType::Color:
            return static_cast<int32>(AsColor.R);
        }

        isValid = false;
        return 0;
    }

    /// <summary>
    /// Gets value as float (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE float GetFloat() const
    {
        bool isValid;
        return GetFloat(isValid);
    }

    /// <summary>
    /// Gets value as float (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    float GetFloat(bool& isValid) const
    {
        isValid = true;

        switch (Type)
        {
        case CommonType::Bool:
            return AsBool ? 1.0f : 0.0f;
        case CommonType::Integer:
            return static_cast<float>(AsInteger);
        case CommonType::Float:
            return AsFloat;
        case CommonType::Vector2:
            return AsVector2.X;
        case CommonType::Vector3:
            return AsVector3.X;
        case CommonType::Vector4:
            return AsVector4.X;
        case CommonType::Color:
            return AsColor.R;
        }

        isValid = false;
        return 0.0f;
    }

    /// <summary>
    /// Gets value as Vector2 (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE Float2 GetVector2() const
    {
        bool isValid;
        return GetVector2(isValid);
    }

    /// <summary>
    /// Gets value as Vector2 (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    Float2 GetVector2(bool& isValid) const
    {
        isValid = true;
        switch (Type)
        {
        case CommonType::Bool:
            return Float2(AsBool ? 1.0f : 0.0f);
        case CommonType::Integer:
            return Float2(static_cast<float>(AsInteger));
        case CommonType::Float:
            return Float2(AsFloat);
        case CommonType::Vector2:
            return AsVector2;
        case CommonType::Vector3:
            return Float2(AsVector3);
        case CommonType::Vector4:
            return Float2(AsVector4);
        case CommonType::Color:
            return Float2(AsColor);
        }
        isValid = false;
        return Float2::Zero;
    }

    /// <summary>
    /// Gets value as Vector3 (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE Float3 GetVector3() const
    {
        bool isValid;
        return GetVector3(isValid);
    }

    /// <summary>
    /// Gets value as Vector3 (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    Float3 GetVector3(bool& isValid) const
    {
        isValid = true;
        switch (Type)
        {
        case CommonType::Bool:
            return Float3(AsBool ? 1.0f : 0.0f);
        case CommonType::Integer:
            return Float3(static_cast<float>(AsInteger));
        case CommonType::Float:
            return Float3(AsFloat);
        case CommonType::Vector2:
            return Float3(AsVector2, 0);
        case CommonType::Vector3:
            return AsVector3;
        case CommonType::Vector4:
            return Vector3(AsVector4);
        case CommonType::Color:
            return Vector3(AsColor);
        }
        isValid = false;
        return Float3::Zero;
    }

    /// <summary>
    /// Gets value as Vector4 (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE Float4 GetVector4() const
    {
        bool isValid;
        return GetVector4(isValid);
    }

    /// <summary>
    /// Gets value as Vector4 (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    Float4 GetVector4(bool& isValid) const
    {
        isValid = true;
        switch (Type)
        {
        case CommonType::Bool:
            return Float4(AsBool ? 1.0f : 0.0f);
        case CommonType::Integer:
            return Float4(static_cast<float>(AsInteger));
        case CommonType::Float:
            return Float4(AsFloat);
        case CommonType::Vector2:
            return Float4(AsVector2, 0, 0);
        case CommonType::Vector3:
            return Float4(AsVector3, 0);
        case CommonType::Vector4:
            return AsVector4;
        case CommonType::Color:
            return Float4(AsColor);
        }
        isValid = false;
        return Float4::Zero;
    }

    /// <summary>
    /// Gets value as Quaternion (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE Quaternion GetRotation() const
    {
        bool isValid;
        return GetRotation(isValid);
    }

    /// <summary>
    /// Gets value as Quaternion (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    Quaternion GetRotation(bool& isValid) const
    {
        isValid = true;

        switch (Type)
        {
        case CommonType::Vector3:
            return Quaternion::Euler(AsVector3);
        case CommonType::Vector4:
            return Quaternion(AsVector4);
        case CommonType::Rotation:
            return AsRotation;
        }

        isValid = false;
        return Quaternion::Identity;
    }

    /// <summary>
    /// Gets value as Color (if can convert it)
    /// </summary>
    /// <returns>Value</returns>
    FORCE_INLINE Color GetColor() const
    {
        bool isValid;
        return GetColor(isValid);
    }

    /// <summary>
    /// Gets value as Color (if can convert it)
    /// </summary>
    /// <param name="isValid">Contains true if value has been converted, otherwise false</param>
    /// <returns>Value</returns>
    Color GetColor(bool& isValid) const
    {
        isValid = true;

        switch (Type)
        {
        case CommonType::Bool:
            return Color(AsBool ? 1.0f : 0.0f);
        case CommonType::Integer:
            return Color(static_cast<float>(AsInteger));
        case CommonType::Float:
            return Color(AsFloat);
        case CommonType::Vector2:
            return Color(AsVector2.X, AsVector2.Y, 0, 1);
        case CommonType::Vector3:
            return Color(AsVector3, 1);
        case CommonType::Vector4:
            return Color(AsVector4);
        case CommonType::Color:
            return AsColor;
        }

        isValid = false;
        return Color::Black;
    }

public:
    /// <summary>
    /// Set new value and change type to Bool
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(bool value)
    {
        SetType(CommonType::Bool);
        AsBool = value;
    }

    /// <summary>
    /// Set new value and change type to Integer
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(int32 value)
    {
        SetType(CommonType::Integer);
        AsInteger = value;
    }

    /// <summary>
    /// Set new value and change type to Float
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(float value)
    {
        SetType(CommonType::Float);
        AsFloat = value;
    }

    /// <summary>
    /// Set new value and change type to Vector2
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Float2& value)
    {
        SetType(CommonType::Vector2);
        AsVector2 = value;
    }

    /// <summary>
    /// Set new value and change type to Vector3
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Float3& value)
    {
        SetType(CommonType::Vector3);
        AsVector3 = value;
    }

    /// <summary>
    /// Set new value and change type to Vector4
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Float4& value)
    {
        SetType(CommonType::Vector4);
        AsVector4 = value;
    }

    /// <summary>
    /// Set new value and change type to Color
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Color& value)
    {
        SetType(CommonType::Color);
        AsColor = value;
    }

    /// <summary>
    /// Set new value and change type to Matrix
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Matrix& value)
    {
        SetType(CommonType::Matrix);
        AsMatrix = value;
    }

    /// <summary>
    /// Set new value and change type to Guid
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Guid& value)
    {
        SetType(CommonType::Guid);
        AsGuid = value;
    }

    /// <summary>
    /// Set new value and change type to String
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const StringView& value)
    {
        SetType(CommonType::String);
        int32 length = value.Length();
        if (length > 0)
        {
            AsString = (Char*)Allocator::Allocate((length + 1) * sizeof(Char));
            Platform::MemoryCopy(AsString, *value, sizeof(Char) * length);
            AsString[length] = 0;
        }
        else
        {
            AsString = nullptr;
        }
    }

    /// <summary>
    /// Set new value and change type to Box
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const BoundingBox& value)
    {
        SetType(CommonType::Box);
        AsBox = value;
    }

    /// <summary>
    /// Set new value and change type to Rotation
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Quaternion& value)
    {
        SetType(CommonType::Rotation);
        AsRotation = value;
    }

    /// <summary>
    /// Set new value and change type to Transform
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Transform& value)
    {
        SetType(CommonType::Transform);
        AsTransform = value;
    }

    /// <summary>
    /// Set new value and change type to Sphere
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const BoundingSphere& value)
    {
        SetType(CommonType::Sphere);
        AsSphere = value;
    }

    /// <summary>
    /// Set new value and change type to Rectangle
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Rectangle& value)
    {
        SetType(CommonType::Rectangle);
        AsRectangle = value;
    }

    /// <summary>
    /// Set new value and change type to Rectangle
    /// </summary>
    /// <param name="value">Value to assign</param>
    void Set(const Ray& value)
    {
        SetType(CommonType::Ray);
        AsRay = value;
    }

    /// <summary>
    /// Sets the common value type to binary blob. Allocates the bloc with the given data capacity.
    /// </summary>
    /// <param name="length">The length (in bytes).</param>
    void SetBlob(int32 length)
    {
        SetType(CommonType::Blob);
        AsBlob.Length = length;
        if (length)
        {
            AsBlob.Data = (byte*)Allocator::Allocate(length);
        }
        else
        {
            AsBlob.Data = nullptr;
        }
    }

    /// <summary>
    /// Sets the value to object reference.
    /// </summary>
    /// <param name="obj">The object to reference.</param>
    void SetObject(ScriptingObject* obj)
    {
        SetType(CommonType::Object);
        AsObject = obj;
        if (obj)
            LinkObject();
    }

public:
    /// <summary>
    /// Change common value type
    /// </summary>
    /// <param name="type">New type</param>
    void SetType(CommonType type)
    {
        if (Type != type)
        {
            if (Type == CommonType::String && AsString)
            {
                Allocator::Free(AsString);
                AsString = nullptr;
            }
            else if (Type == CommonType::Blob && AsBlob.Data)
            {
                Allocator::Free(AsBlob.Data);
                AsBlob.Data = nullptr;
                AsBlob.Length = 0;
            }
            else if (Type == CommonType::Object && AsObject)
            {
                UnlinkObject();
                AsObject = nullptr;
            }

            Type = type;
        }
    }

public:
    /// <summary>
    /// Casts value from its type to another
    /// </summary>
    /// <param name="to">Output type</param>
    /// <returns>Changed value</returns>
    CommonValue Cast(CommonType to) const
    {
        return Cast(*this, to);
    }

    /// <summary>
    /// Casts value from its type to another
    /// </summary>
    /// <param name="v">Value to cast</param>
    /// <param name="to">Output type</param>
    /// <returns>Changed value</returns>
    static CommonValue Cast(const CommonValue& v, CommonType to)
    {
        // If they are the same types, then just return value
        if (v.Type == to)
        {
            // The same
            return v;
        }

        // Switch on some switches to switch between the switches
        switch (to)
        {
        case CommonType::Bool:
            return v.GetBool();
        case CommonType::Integer:
            return v.GetInteger();
        case CommonType::Float:
            return v.GetFloat();
        case CommonType::Vector2:
            return v.GetVector2();
        case CommonType::Vector3:
            return v.GetVector3();
        case CommonType::Vector4:
            return v.GetVector4();
        case CommonType::Rotation:
            return v.GetRotation();
        case CommonType::Color:
            return v.GetColor();
        }

        CRASH;
        return Null;
    }

public:
    friend bool operator==(const CommonValue& a, const CommonValue& b)
    {
        ASSERT(a.Type == b.Type);
        switch (a.Type)
        {
        case CommonType::Bool:
            return a.AsBool == b.AsBool;
        case CommonType::Integer:
            return a.AsInteger == b.AsInteger;
        case CommonType::Float:
            return a.AsFloat == b.AsFloat;
        case CommonType::Vector2:
            return a.AsVector2 == b.AsVector2;
        case CommonType::Vector3:
            return a.AsVector3 == b.AsVector3;
        case CommonType::Vector4:
            return a.AsVector4 == b.AsVector4;
        case CommonType::Color:
            return a.AsColor == b.AsColor;
        case CommonType::Guid:
            return a.AsGuid == b.AsGuid;
        case CommonType::String:
            return StringUtils::Compare(a.AsString, b.AsString) == 0;
        case CommonType::Box:
            return a.AsBox == b.AsBox;
        case CommonType::Rotation:
            return a.AsRotation == b.AsRotation;
        case CommonType::Transform:
            return a.AsTransform == b.AsTransform;
        case CommonType::Sphere:
            return a.AsSphere == b.AsSphere;
        case CommonType::Rectangle:
            return a.AsRectangle == b.AsRectangle;
        case CommonType::Ray:
            return a.AsRay == b.AsRay;
        case CommonType::Pointer:
        case CommonType::Object:
            return a.AsPointer == b.AsPointer;
        case CommonType::Matrix:
            return a.AsMatrix == b.AsMatrix;
        case CommonType::Blob:
            return a.AsBlob.Data == b.AsBlob.Data && a.AsBlob.Length == b.AsBlob.Length;
        default: CRASH;
            return false;
        }
    }

    friend bool operator!=(const CommonValue& a, const CommonValue& b)
    {
        return !operator==(a, b);
    }

    friend bool operator>(const CommonValue& a, const CommonValue& b)
    {
        ASSERT(a.Type == b.Type);
        switch (a.Type)
        {
        case CommonType::Bool:
            return a.AsBool > b.AsBool;
        case CommonType::Integer:
            return a.AsInteger > b.AsInteger;
        case CommonType::Float:
            return a.AsFloat > b.AsFloat;
        case CommonType::Vector2:
            return a.AsVector2 > b.AsVector2;
        case CommonType::Vector3:
            return a.AsVector3 > b.AsVector3;
        case CommonType::Vector4:
            return a.AsVector4 > b.AsVector4;
        //case CommonType::Color: return a.AsColor > b.AsColor;
        //case CommonType::Guid: return a.AsGuid > b.AsGuid;
        case CommonType::String:
            return StringUtils::Compare(a.AsString, b.AsString) > 0;
        //case CommonType::Box: return a.AsBox > b.AsBox;
        //case CommonType::Rotation: return a.AsRotation > b.AsRotation;
        //case CommonType::Transform: return a.AsTransform > b.AsTransform;
        //case CommonType::Sphere: return a.AsSphere > b.AsSphere;
        //case CommonType::Rectangle: return a.AsRectangle > b.AsRectangle;
        //case CommonType::Ray: return a.AsRay > b.AsRay;
        case CommonType::Pointer:
        case CommonType::Object:
            return a.AsPointer > b.AsPointer;
        //case CommonType::Matrix: return a.AsMatrix > b.AsMatrix;
        case CommonType::Blob:
            return a.AsBlob.Length > b.AsBlob.Length;
        default: CRASH;
            return false;
        }
    }

    friend bool operator<(const CommonValue& a, const CommonValue& b)
    {
        return !operator==(a, b) && !operator>(a, b);
    }

    friend bool operator>=(const CommonValue& a, const CommonValue& b)
    {
        return !operator<(a, b);
    }

    friend bool operator<=(const CommonValue& a, const CommonValue& b)
    {
        return !operator>(a, b);
    }

public:
    static bool NearEqual(const CommonValue& a, const CommonValue& b, float epsilon);

    static CommonValue Lerp(const CommonValue& a, const CommonValue& b, float alpha);

    // Gets the ID of the object (valid only if common value is of type Object).
    Guid GetObjectId() const;

private:
    void OnObjectDeleted(ScriptingObject* obj)
    {
        AsObject = nullptr;
    }

    void LinkObject();
    void UnlinkObject();
};
