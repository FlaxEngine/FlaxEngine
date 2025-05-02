// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Object.h"
#include "GraphNode.h"

/// <summary>
/// Shader source generator value container. Caches the value type and the value variable name (shader local, global parameter or constant value). Supports value type casting and component swizzle.
/// </summary>
struct ShaderGraphValue : Object
{
private:
    static const Char* _subs[];

public:
    /// <summary>
    /// The value type.
    /// </summary>
    VariantType::Types Type;

    /// <summary>
    /// The shader value.
    /// </summary>
    String Value;

public:
    /// <summary>
    /// Zero value (as float).
    /// </summary>
    static const ShaderGraphValue Zero;

    /// <summary>
    /// Half value (as float).
    /// </summary>
    static const ShaderGraphValue Half;

    /// <summary>
    /// One value (as float).
    /// </summary>
    static const ShaderGraphValue One;

    /// <summary>
    /// True value (as bool).
    /// </summary>
    static const ShaderGraphValue True;

    /// <summary>
    /// False value (as bool).
    /// </summary>
    static const ShaderGraphValue False;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    ShaderGraphValue()
        : Type(VariantType::Types::Null)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="type">The type.</param>
    /// <param name="value">The value.</param>
    ShaderGraphValue(VariantType::Types type, const Char* value)
        : Type(type)
        , Value(value)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="type">The type.</param>
    /// <param name="value">The value.</param>
    ShaderGraphValue(VariantType::Types type, const String&& value)
        : Type(type)
        , Value(MoveTemp(value))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="type">The type.</param>
    /// <param name="value">The value.</param>
    ShaderGraphValue(VariantType::Types type, const String& value)
        : Type(type)
        , Value(value)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="value">The value.</param>
    explicit ShaderGraphValue(const bool value)
        : Type(VariantType::Types::Bool)
        , Value(value ? TEXT("true") : TEXT("false"))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="value">The value.</param>
    explicit ShaderGraphValue(const float value)
        : Type(VariantType::Types::Float)
        , Value(StringUtils::ToString(value))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="value">The value.</param>
    explicit ShaderGraphValue(const double value)
        : Type(VariantType::Types::Float)
        , Value(StringUtils::ToString(value))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="value">The value.</param>
    explicit ShaderGraphValue(const int32 value)
        : Type(VariantType::Types::Int)
        , Value(StringUtils::ToString(value))
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="ShaderGraphValue"/> struct.
    /// </summary>
    /// <param name="v">The value.</param>
    explicit ShaderGraphValue(const Variant& v);

public:
    /// <summary>
    /// Returns true if value is valid.
    /// </summary>
    FORCE_INLINE bool IsValid() const
    {
        return Type != VariantType::Types::Null;
    }

    /// <summary>
    /// Returns true if value is invalid.
    /// </summary>
    FORCE_INLINE bool IsInvalid() const
    {
        return Type == VariantType::Types::Null;
    }

    /// <summary>
    /// Checks if value contains static part with zero.
    /// </summary>
    bool IsZero() const;

    /// <summary>
    /// Checks if value contains static part with one.
    /// </summary>
    bool IsOne() const;

    /// <summary>
    /// Checks if value is a compile-time constant literal (eg. int, bool or float).
    /// </summary>
    bool IsLiteral() const;

    /// <summary>
    /// Clears this instance.
    /// </summary>
    void Clear()
    {
        Type = VariantType::Types::Null;
        Value.Clear();
    }

public:
    /// <summary>
    /// Formats thw value.
    /// </summary>
    /// <param name="format">The format text.</param>
    /// <param name="v1">The value.</param>
    /// <returns>The formatted value.</returns>
    static String Format(const Char* format, const ShaderGraphValue& v1)
    {
        return String::Format(format, v1.Value);
    }

    /// <summary>
    /// Formats thw value.
    /// </summary>
    /// <param name="format">The format text.</param>
    /// <param name="v1">The first value.</param>
    /// <param name="v2">The second value.</param>
    /// <returns>The formatted value.</returns>
    static String Format(const Char* format, const ShaderGraphValue& v1, const ShaderGraphValue& v2)
    {
        return String::Format(format, v1.Value, v2.Value);
    }

    /// <summary>
    /// Formats thw value.
    /// </summary>
    /// <param name="format">The format text.</param>
    /// <param name="v1">The first value.</param>
    /// <param name="v2">The second value.</param>
    /// <param name="v3">The third value.</param>
    /// <returns>The formatted value.</returns>
    static String Format(const Char* format, const ShaderGraphValue& v1, const ShaderGraphValue& v2, const ShaderGraphValue& v3)
    {
        return String::Format(format, v1.Value, v2.Value, v3.Value);
    }

    /// <summary>
    /// Formats thw value.
    /// </summary>
    /// <param name="format">The format text.</param>
    /// <param name="v1">The first value.</param>
    /// <param name="v2">The second value.</param>
    /// <param name="v3">The third value.</param>
    /// <param name="v4">The fourth value.</param>
    /// <returns>The formatted value.</returns>
    static String Format(const Char* format, const ShaderGraphValue& v1, const ShaderGraphValue& v2, const ShaderGraphValue& v3, const ShaderGraphValue& v4)
    {
        return String::Format(format, v1.Value, v2.Value, v3.Value, v4.Value);
    }

public:
    /// <summary>
    /// Initializes the shader variable for given connection type Zero.
    /// </summary>
    /// <param name="type">The graph connection type.</param>
    /// <returns>Initial value for given type.</returns>
    static ShaderGraphValue InitForZero(VariantType::Types type);

    /// <summary>
    /// Initializes the shader variable for given connection type Half.
    /// </summary>
    /// <param name="type">The graph connection type.</param>
    /// <returns>Initial value for given type.</returns>
    static ShaderGraphValue InitForHalf(VariantType::Types type);

    /// <summary>
    /// Initializes the shader variable for given connection type One.
    /// </summary>
    /// <param name="type">The graph connection type.</param>
    /// <returns>Initial value for given type.</returns>
    static ShaderGraphValue InitForOne(VariantType::Types type);

    /// <summary>
    /// Create float2 from X and Y values.
    /// </summary>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <returns>float2</returns>
    static ShaderGraphValue Float2(const ShaderGraphValue& x, const ShaderGraphValue& y)
    {
        return ShaderGraphValue(
            VariantType::Types::Float2,
            String::Format(TEXT("float2({0}, {1})"),
                           Cast(x, VariantType::Types::Float).Value,
                           Cast(y, VariantType::Types::Float).Value));
    }

    /// <summary>
    /// Create float3 from X, Y and Z values.
    /// </summary>
    /// <param name="x">The x.</param>
    /// <param name="y">The y.</param>
    /// <param name="z">The z.</param>
    /// <returns>float3</returns>
    static ShaderGraphValue Float3(const ShaderGraphValue& x, const ShaderGraphValue& y, const ShaderGraphValue& z)
    {
        return ShaderGraphValue(
            VariantType::Types::Float3,
            String::Format(TEXT("float3({0}, {1}, {2})"),
                           Cast(x, VariantType::Types::Float).Value,
                           Cast(y, VariantType::Types::Float).Value,
                           Cast(z, VariantType::Types::Float).Value));
    }

    /// <summary>
    /// Create float4 from X, Y, Z and W values.
    /// </summary>
    /// <param name="x">The X.</param>
    /// <param name="y">The Y.</param>
    /// <param name="z">The Z.</param>
    /// <param name="w">The W.</param>
    /// <returns>float4</returns>
    static ShaderGraphValue Float4(const ShaderGraphValue& x, const ShaderGraphValue& y, const ShaderGraphValue& z, const ShaderGraphValue& w)
    {
        return ShaderGraphValue(
            VariantType::Types::Float4,
            String::Format(TEXT("float4({0}, {1}, {2}, {3})"),
                           Cast(x, VariantType::Types::Float).Value,
                           Cast(y, VariantType::Types::Float).Value,
                           Cast(z, VariantType::Types::Float).Value,
                           Cast(w, VariantType::Types::Float).Value));
    }

public:
    /// <summary>
    /// Gets the X component of the value. Valid only for single or vector types.
    /// </summary>
    /// <returns>The X component.</returns>
    ShaderGraphValue GetX() const
    {
        return ShaderGraphValue(VariantType::Types::Float, Value + _subs[0]);
    }

    /// <summary>
    /// Gets the Y component of the value. Valid only for vector types.
    /// </summary>
    /// <returns>The Y component.</returns>
    ShaderGraphValue GetY() const;

    /// <summary>
    /// Gets the Z component of the value. Valid only for vector types.
    /// </summary>
    /// <returns>The Z component.</returns>
    ShaderGraphValue GetZ() const;

    /// <summary>
    /// Gets the W component of the value. Valid only for vector types.
    /// </summary>
    /// <returns>The W component.</returns>
    ShaderGraphValue GetW() const;

public:
    /// <summary>
    /// Casts the value to the bool type.
    /// </summary>
    /// <returns>Bool</returns>
    ShaderGraphValue AsBool() const
    {
        return Cast(*this, VariantType::Types::Bool);
    }

    /// <summary>
    /// Casts the value to the integer type.
    /// </summary>
    /// <returns>Integer</returns>
    ShaderGraphValue AsInt() const
    {
        return Cast(*this, VariantType::Types::Int);
    }

    /// <summary>
    /// Casts the value to the unsigned integer type.
    /// </summary>
    /// <returns>UnsignedInteger</returns>
    ShaderGraphValue AsUint() const
    {
        return Cast(*this, VariantType::Types::Uint);
    }

    /// <summary>
    /// Casts the value to the float type.
    /// </summary>
    /// <returns>Float</returns>
    ShaderGraphValue AsFloat() const
    {
        return Cast(*this, VariantType::Types::Float);
    }

    /// <summary>
    /// Casts the value to the Float2 type.
    /// </summary>
    /// <returns>Float2</returns>
    ShaderGraphValue AsFloat2() const
    {
        return Cast(*this, VariantType::Types::Float2);
    }

    /// <summary>
    /// Casts the value to the Float3 type.
    /// </summary>
    /// <returns>Float3</returns>
    ShaderGraphValue AsFloat3() const
    {
        return Cast(*this, VariantType::Types::Float3);
    }

    /// <summary>
    /// Casts the value to the Float4 type.
    /// </summary>
    /// <returns>Float4</returns>
    ShaderGraphValue AsFloat4() const
    {
        return Cast(*this, VariantType::Types::Float4);
    }

    /// <summary>
    /// Casts the value from its type to the another type.
    /// </summary>
    /// <param name="to">The result type.</param>
    /// <returns>The result value.</returns>
    ShaderGraphValue Cast(const VariantType::Types to) const
    {
        return Cast(*this, to);
    }

    /// <summary>
    /// Casts the value from its type to the another type.
    /// </summary>
    /// <param name="v">The value to cast.</param>
    /// <param name="to">The result type.</param>
    /// <returns>The result value.</returns>
    static ShaderGraphValue Cast(const ShaderGraphValue& v, VariantType::Types to);

public:
    // [Object]
    String ToString() const override
    {
        return Value;
    }
};
