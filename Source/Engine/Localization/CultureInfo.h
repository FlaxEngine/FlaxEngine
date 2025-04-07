// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Formatting.h"

/// <summary>
/// The information about a specific culture (aka locale). The information includes the names for the culture, the writing system, the calendar used, the sort order of strings, and formatting for dates and numbers.
/// </summary>
API_CLASS(InBuild, Namespace="System.Globalization") class FLAXENGINE_API CultureInfo
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(CultureInfo);

private:
    void* _data;
    int32 _lcid;
    int32 _lcidParent;
    String _name;
    String _nativeName;
    String _englishName;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="CultureInfo"/> class.
    /// </summary>
    /// <param name="lcid">The culture identifier.</param>
    CultureInfo(int32 lcid);

    /// <summary>
    /// Initializes a new instance of the <see cref="CultureInfo"/> class.
    /// </summary>
    /// <param name="name">The culture name (eg. pl-PL).</param>
    CultureInfo(const StringView& name);

    /// <summary>
    /// Initializes a new instance of the <see cref="CultureInfo"/> class.
    /// </summary>
    /// <param name="name">The culture name (eg. pl-PL).</param>
    CultureInfo(const StringAnsiView& name);

public:
    /// <summary>
    /// Gets the culture identifier.
    /// </summary>
    int32 GetLCID() const;

    /// <summary>
    /// Gets the parent culture identifier.
    /// </summary>
    int32 GetParentLCID() const;

    /// <summary>
    /// Gets the culture name (eg. pl-PL).
    /// </summary>
    const String& GetName() const;

    /// <summary>
    /// Gets the full localized culture name (eg. Polish (Poland)).
    /// </summary>
    const String& GetNativeName() const;

    /// <summary>
    /// Gets the culture name in English (eg. polski (Polska)).
    /// </summary>
    const String& GetEnglishName() const;

    String ToString() const;
    bool operator==(const CultureInfo& other) const;
};

DEFINE_DEFAULT_FORMATTING_VIA_TO_STRING(CultureInfo);

namespace MUtils
{
    extern void* ToManaged(const CultureInfo& value);
    extern CultureInfo ToNative(void* value);
}
