// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Scripting/SoftObjectReference.h"

/// <summary>
/// Contains localized strings table for a given culture.
/// </summary>
/// <seealso cref="JsonAssetBase" />
API_CLASS(NoSpawn) class FLAXENGINE_API LocalizedStringTable : public JsonAssetBase
{
    DECLARE_ASSET_HEADER(LocalizedStringTable);
public:
    /// <summary>
    /// The locale of the localized string table (eg. pl-PL).
    /// </summary>
    API_FIELD() String Locale;

    /// <summary>
    /// The fallback language table to use for missing keys. Eg. table for 'en-GB' can point to 'en' as a fallback to prevent problem of missing localized strings.
    /// </summary>
    API_FIELD() SoftObjectReference<LocalizedStringTable> FallbackTable;

    /// <summary>
    /// The string table. Maps the message id into the localized text. For plural messages the list contains separate items for value numbers.
    /// </summary>
    API_FIELD() Dictionary<String, Array<String>> Entries;

public:
    /// <summary>
    /// Adds the localized string to the table.
    /// </summary>
    /// <param name="id">The message id. Used for lookups.</param>
    /// <param name="value">The localized text.</param>
    API_FUNCTION() void AddString(const StringView& id, const StringView& value);

    /// <summary>
    /// Adds the localized plural string to the table.
    /// </summary>
    /// <param name="id">The message id. Used for lookups.</param>
    /// <param name="value">The localized text.</param>
    /// <param name="n">The plural value (0, 1, 2..).</param>
    API_FUNCTION() void AddPluralString(const StringView& id, const StringView& value, int32 n);

    /// <summary>
    /// Gets the localized string by using string id lookup. Uses fallback table if text is not included in this table.
    /// </summary>
    /// <param name="id">The message identifier.</param>
    /// <returns>The localized text.</returns>
    API_FUNCTION() String GetString(const String& id) const;

    /// <summary>
    /// Gets the localized plural string by using string id lookup. Uses fallback table if text is not included in this table.
    /// </summary>
    /// <param name="id">The message identifier.</param>
    /// <param name="n">The value count for plural message selection.</param>
    /// <returns>The localized text.</returns>
    API_FUNCTION() String GetPluralString(const String& id, int32 n) const;

protected:
    // [JsonAssetBase]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
    void OnGetData(rapidjson_flax::StringBuffer& buffer) const override;
};
