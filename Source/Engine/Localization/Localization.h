// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "CultureInfo.h"
#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// The language and culture localization manager.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Localization
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Localization);
public:
    /// <summary>
    /// Gets the current culture (date, time, currency and values formatting locale).
    /// </summary>
    API_PROPERTY() static const CultureInfo& GetCurrentCulture();

    /// <summary>
    /// Sets the current culture (date, time, currency and values formatting locale).
    /// </summary>
    API_PROPERTY() static void SetCurrentCulture(const CultureInfo& value);

    /// <summary>
    /// Gets the current language (text display locale).
    /// </summary>
    API_PROPERTY() static const CultureInfo& GetCurrentLanguage();

    /// <summary>
    /// Sets the current language (text display locale).
    /// </summary>
    API_PROPERTY() static void SetCurrentLanguage(const CultureInfo& value);

    /// <summary>
    /// Sets both the current language (text display locale) and the current culture (date, time, currency and values formatting locale) at once.
    /// </summary>
    API_FUNCTION() static void SetCurrentLanguageCulture(const CultureInfo& value);

    /// <summary>
    /// Occurs when current culture or language gets changed. Can be used to refresh UI to reflect language changes.
    /// </summary>
    API_EVENT() static Delegate<> LocalizationChanged;

public:
    /// <summary>
    /// Gets the localized string for the current language by using string id lookup.
    /// </summary>
    /// <param name="id">The message identifier.</param>
    /// <param name="fallback">The optional fallback string value to use if localized string is missing.</param>
    /// <returns>The localized text.</returns>
    API_FUNCTION() static String GetString(const String& id, const String& fallback = String::Empty);

    /// <summary>
    /// Gets the localized plural string for the current language by using string id lookup.
    /// </summary>
    /// <param name="id">The message identifier.</param>
    /// <param name="n">The value count for plural message selection.</param>
    /// <param name="fallback">The optional fallback string value to use if localized string is missing.</param>
    /// <returns>The localized text.</returns>
    API_FUNCTION() static String GetPluralString(const String& id, int32 n, const String& fallback = String::Empty);
};
