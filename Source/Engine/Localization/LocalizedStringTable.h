// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Content/AssetReference.h"

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
    /// The fallback table used if we couldnt find the key here
    /// </summary>
    API_FIELD() AssetReference<LocalizedStringTable> FallbackTable;

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
    /// Gets the localized string for the current language by using string id lookup.
    /// </summary>
    /// <param name="id">The message identifier.</param>
    /// <param name="fallback">The optional fallback string value to use if localized string is missing.</param>
    /// <returns>The localized text.</returns>
    API_FUNCTION() String GetString(const String& id);

    /// <summary>
    /// Gets the localized plural string for the current language by using string id lookup.
    /// </summary>
    /// <param name="id">The message identifier.</param>
    /// <param name="n">The value count for plural message selection.</param>
    /// <param name="fallback">The optional fallback string value to use if localized string is missing.</param>
    /// <returns>The localized text.</returns>
    API_FUNCTION() String GetPluralString(const String& id, int32 n);


#if USE_EDITOR

    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    API_FUNCTION() bool Save(const StringView& path = StringView::Empty);

#endif

protected:
    // [JsonAssetBase]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;
};
