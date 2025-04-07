// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BaseTypes.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

/// <summary>
/// Globally Unique Identifier (GUID) represented by 128-bit integer (16 bytes) that can be used across all computers and networks wherever a unique identifier is required. Such an identifier has a very low probability of being duplicated.
/// </summary>
API_STRUCT(InBuild, Namespace="System") struct FLAXENGINE_API Guid
{
public:
    /// <summary>
    /// Accepted format specifiers for the format parameter.
    /// </summary>
    enum class FormatType
    {
        // 32 digits:
        // 00000000000000000000000000000000
        N,

        // 32 digits separated by hyphens:
        // 00000000-0000-0000-0000-000000000000
        D,

        // 32 digits separated by hyphens, enclosed in braces:
        // {00000000-0000-0000-0000-000000000000}
        B,

        // 32 digits separated by hyphens, enclosed in parentheses:
        // (00000000-0000-0000-0000-000000000000)
        P,
    };

public:
    union
    {
        struct
        {
            // The first component.
            uint32 A;

            // The second component.
            uint32 B;

            // The third component.
            uint32 C;

            // The fourth component.
            uint32 D;
        };

        // Raw bytes with the Guid.
        byte Raw[16];

        // Raw values with the Guid.
        uint32 Values[4];
    };

public:
    // Empty Guid (considered as invalid ID).
    static Guid Empty;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Guid()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Guid"/> struct.
    /// </summary>
    /// <param name="a">The first component.</param>
    /// <param name="b">The second component.</param>
    /// <param name="c">The third component.</param>
    /// <param name="d">The fourth component.</param>
    Guid(uint32 a, uint32 b, uint32 c, uint32 d)
        : A(a)
        , B(b)
        , C(c)
        , D(d)
    {
    }

public:
    bool operator==(const Guid& other) const
    {
        return ((A ^ other.A) | (B ^ other.B) | (C ^ other.C) | (D ^ other.D)) == 0;
    }

    bool operator!=(const Guid& other) const
    {
        return ((A ^ other.A) | (B ^ other.B) | (C ^ other.C) | (D ^ other.D)) != 0;
    }

    // Provides access to the Guid components (0...3).
    uint32& operator[](int32 index)
    {
        ASSERT(index >= 0 && index < 4);
        return Values[index];
    }

    // Provides read-only access to the Guid components (0...3).
    const uint32& operator[](int index) const
    {
        ASSERT(index >= 0 && index < 4);
        return Values[index];
    }

    // Checks whether this Guid is valid or not. A Guid that has all its components set to zero is considered invalid.
    FORCE_INLINE bool IsValid() const
    {
        return (A | B | C | D) != 0;
    }

    /// Checks if Guid is valid.
    explicit operator bool() const
    {
        return (A | B | C | D) != 0;
    }

public:
    String ToString() const;
    String ToString(FormatType format) const;
    void ToString(char* buffer, FormatType format) const;
    void ToString(Char* buffer, FormatType format) const;

public:
    /// <summary>
    /// Try to parse Guid from string
    /// </summary>
    /// <param name="text">Input text</param>
    /// <param name="value">Result value</param>
    /// <returns>True if cannot parse text, otherwise false</returns>
    static bool Parse(const StringView& text, Guid& value);

    /// <summary>
    /// Try to parse Guid from string
    /// </summary>
    /// <param name="text">Input text</param>
    /// <param name="value">Result value</param>
    /// <returns>True if cannot parse text, otherwise false</returns>
    static bool Parse(const StringAnsiView& text, Guid& value);

    /// <summary>
    /// Creates the unique identifier.
    /// </summary>
    /// <returns>New Guid</returns>
    FORCE_INLINE static Guid New()
    {
        Guid result;
        Platform::CreateGuid(result);
        return result;
    }
};

template<>
struct TIsPODType<Guid>
{
    enum { Value = true };
};

inline uint32 GetHash(const Guid& key)
{
    return static_cast<int32>(key.A ^ key.B ^ key.C ^ key.D);
}

DEFINE_DEFAULT_FORMATTING(Guid, "{:0>8x}{:0>8x}{:0>8x}{:0>8x}", v.A, v.B, v.C, v.D);
