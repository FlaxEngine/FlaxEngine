// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "BaseTypes.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Templates.h"

/// <summary>
/// Represents the version number made of major, minor, build and revision numbers.
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API Version
{
private:
    int32 _major;
    int32 _minor;
    int32 _build;
    int32 _revision;

public:
    /// <summary>
    /// Initializes a new instance of the Version class with the specified major, minor, build, and revision numbers.
    /// </summary>
    /// <param name="major">The major version number.</param>
    /// <param name="minor">The minor version number.</param>
    /// <param name="build">The build number.</param>
    /// <param name="revision">The revision number.</param>
    Version(int32 major, int32 minor, int32 build, int32 revision);

    /// <summary>
    /// Initializes a new instance of the Version class using the specified major, minor, and build values.
    /// </summary>
    /// <param name="major">The major version number.</param>
    /// <param name="minor">The minor version number.</param>
    /// <param name="build">The build number.</param>
    Version(int32 major, int32 minor, int32 build);

    /// <summary>
    /// Initializes a new instance of the Version class using the specified major and minor values.
    /// </summary>
    /// <param name="major">The major version number.</param>
    /// <param name="minor">The minor version number.</param>
    Version(int32 major, int32 minor);

    /// <summary>
    /// Initializes a new instance of the Version class.
    /// </summary>
    Version()
    {
        _major = 0;
        _minor = 0;
        _revision = -1;
        _build = -1;
    }

public:
    /// <summary>
    /// Gets the value of the build component of the version number for the current Version object.
    /// </summary>
    /// <returns>The build number, or -1 if the build number is undefined.</returns>
    FORCE_INLINE int32 Build() const
    {
        return _build;
    }

    /// <summary>
    /// Gets the value of the major component of the version number for the current Version object.
    /// </summary>
    /// <returns>The major version number.</returns>
    FORCE_INLINE int32 Major() const
    {
        return _major;
    }

    /// <summary>
    /// Gets the value of the minor component of the version number for the current Version object.
    /// </summary>
    /// <returns>The minor version number.</returns>
    FORCE_INLINE int32 Minor() const
    {
        return _minor;
    }

    /// <summary>
    /// Gets the value of the revision component of the version number for the current Version object.
    /// </summary>
    /// <returns>The revision number, or -1 if the revision number is undefined.</returns>
    FORCE_INLINE int32 Revision() const
    {
        return _revision;
    }

public:
    /// <summary>
    /// Compares the current Version object to a specified Version object and returns an indication of their relative values.
    /// </summary>
    /// <param name="value">A Version object to compare to the current Version object, or null.</param>
    /// <returns>A signed integer that indicates the relative values of the two objects, as shown in the following table.Return value Meaning Less than zero The current Version object is a version before <paramref name="value" />. Zero The current Version object is the same version as <paramref name="value" />. Greater than zero The current Version object is a version subsequent to <paramref name="value" />. -or-<paramref name="value" /> is null.</returns>
    int32 CompareTo(const Version& value) const;

    /// <summary>
    /// Returns a value indicating whether the current Version object and a specified Version object represent the same value.
    /// </summary>
    /// <param name="obj">A Version object to compare to the current Version object, or null.</param>
    /// <returns>True if every component of the current Version object matches the corresponding component of the <paramref name="obj" /> parameter; otherwise, false.</returns>
    bool Equals(const Version& obj) const
    {
        return _major == obj._major && _minor == obj._minor && _build == obj._build && _revision == obj._revision;
    }

    FORCE_INLINE bool operator==(const Version& other) const
    {
        return Equals(other);
    }
    FORCE_INLINE bool operator>(const Version& other) const
    {
        return other < *this;
    }
    FORCE_INLINE bool operator>=(const Version& other) const
    {
        return other <= *this;
    }
    FORCE_INLINE bool operator!=(const Version& other) const
    {
        return !(*this == other);
    }
    FORCE_INLINE bool operator<(const Version& other) const
    {
        return CompareTo(other) < 0;
    }
    FORCE_INLINE bool operator<=(const Version& other) const
    {
        return CompareTo(other) <= 0;
    }

public:
    /// <summary>
    /// Converts the value of the current Version object to its equivalent <see cref="T:String" /> representation.
    /// A specified count indicates the number of components to return.
    /// </summary>
    /// <returns>The <see cref="T:String" /> representation of the values of the major, minor, build, and revision components of the current Version object, each separated by a period character ('.'). The <paramref name="fieldCount" /> parameter determines how many components are returned.fieldCount Return Value 0 An empty string (""). 1 major 2 major.minor 3 major.minor.build 4 major.minor.build.revision For example, if you create Version object using the constructor Version(1,3,5), ToString(2) returns "1.3" and ToString(4) throws an exception.</returns>
    /// <param name="fieldCount">The number of components to return. The <paramref name="fieldCount" /> ranges from 0 to 4.</param>
    String ToString(int32 fieldCount) const;

    String ToString() const;

public:
    /// <summary>
    /// Try to parse Version from string.
    /// </summary>
    /// <param name="text">Input text.</param>
    /// <param name="value">Result value.</param>
    /// <returns>True if cannot parse text, otherwise false.</returns>
    static bool Parse(const String& text, Version* value);
};

inline uint32 GetHash(const Version& key)
{
    return ((key.Major() & 15) << 28) | ((key.Minor() & 255) << 20) | ((key.Build() & 255) << 12) | (key.Revision() & 4095);
}

template<>
struct TIsPODType<Version>
{
    enum { Value = true };
};
