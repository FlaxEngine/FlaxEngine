// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "FlaxStorage.h"

/// <summary>
/// Flax Game Engine asset files container object.
/// </summary>
class FLAXENGINE_API FlaxFile : public FlaxStorage
{
protected:

    Entry _asset;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxFile"/> class.
    /// </summary>
    /// <param name="path">The path.</param>
    FlaxFile(const StringView& path);

public:

    // [FlaxStorage]
    String ToString() const override;
    bool IsPackage() const override;
    bool AllowDataModifications() const override;
    bool HasAsset(const Guid& id) const override;
    bool HasAsset(const AssetInfo& info) const override;
    int32 GetEntriesCount() const override;
    void GetEntry(int32 index, Entry& output) const override;
    void GetEntries(Array<Entry>& output) const override;
    void Dispose() override;

protected:

    // [FlaxStorage]
    bool GetEntry(const Guid& id, Entry& e) override;
    void AddEntry(Entry& e) override;
};
