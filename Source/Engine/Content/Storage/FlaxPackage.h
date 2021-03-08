// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "FlaxStorage.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// Flax resources package container.
/// </summary>
class FLAXENGINE_API FlaxPackage : public FlaxStorage
{
protected:

    Dictionary<Guid, Entry> _entries;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxPackage"/> class.
    /// </summary>
    /// <param name="path">The path.</param>
    FlaxPackage(const StringView& path);

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
