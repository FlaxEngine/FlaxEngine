// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Types/Variant.h"

/// <summary>
/// The global gameplay variables container asset that can be accessed across whole project.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API GameplayGlobals : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(GameplayGlobals, 2);
public:
    /// <summary>
    /// The variable data.
    /// </summary>
    struct Variable
    {
        /// <summary>
        /// The current value.
        /// </summary>
        Variant Value;

        /// <summary>
        /// The default value.
        /// </summary>
        Variant DefaultValue;
    };

public:
    /// <summary>
    /// The collection of gameplay global variables identified by the name.
    /// </summary>
    Dictionary<String, Variable> Variables;

public:
    /// <summary>
    /// Gets the values (run-time).
    /// </summary>
    /// <returns>The values (run-time).</returns>
    API_PROPERTY() Dictionary<String, Variant> GetValues() const;

    /// <summary>
    /// Sets the values (run-time).
    /// </summary>
    /// <param name="values">The values (run-time).</param>
    API_PROPERTY() void SetValues(const Dictionary<String, Variant>& values);

    /// <summary>
    /// Gets the default values (edit-time).
    /// </summary>
    /// <returns>The default values (edit-time).</returns>
    API_PROPERTY() Dictionary<String, Variant> GetDefaultValues() const;

    /// <summary>
    /// Sets the default values (edit-time).
    /// </summary>
    /// <param name="values">The default values (edit-time).</param>
    API_PROPERTY() void SetDefaultValues(const Dictionary<String, Variant>& values);

public:
    /// <summary>
    /// Gets the value of the global variable (it must be added first).
    /// </summary>
    /// <param name="name">The variable name.</param>
    /// <returns>The value.</returns>
    API_FUNCTION() Variant GetValue(const StringView& name) const;

    /// <summary>
    /// Sets the value of the global variable (it must be added first).
    /// </summary>
    /// <param name="name">The variable name.</param>
    /// <param name="value">The value.</param>
    API_FUNCTION() void SetValue(const StringView& name, const Variant& value);

    /// <summary>
    /// Resets the variables values to default values.
    /// </summary>
    API_FUNCTION() void ResetValues();

#if USE_EDITOR

    /// <summary>
    /// Saves this asset to the file. Supported only in Editor.
    /// </summary>
    /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
    /// <returns>True if cannot save data, otherwise false.</returns>
    API_FUNCTION() bool Save(const StringView& path = StringView::Empty);

#endif

public:
    // [BinaryAsset]
    void InitAsVirtual() override;

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
