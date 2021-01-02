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
    FlaxPackage(const StringView& path)
        : FlaxStorage(path)
        , _entries(256)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="FlaxPackage"/> class.
    /// </summary>
    ~FlaxPackage()
    {
    }

public:

    // [FlaxStorage]
    String ToString() const override
    {
        return String::Format(TEXT("Package \'{0}\'"), _path);
    }

    bool IsPackage() const override
    {
        return true;
    }

    bool AllowDataModifications() const override
    {
        return false;
    }

    bool HasAsset(const Guid& id) const override
    {
        return _entries.ContainsKey(id);
    }

    bool HasAsset(const AssetInfo& info) const override;

    int32 GetEntriesCount() const override
    {
        return _entries.Count();
    }

    void GetEntry(int32 index, Entry& output) const override
    {
        ASSERT(index >= 0 && index < _entries.Count());
        for (auto i = _entries.Begin(); i.IsNotEnd(); ++i)
        {
            if (index-- <= 0)
            {
                output = i->Value;
                return;
            }
        }
    }

    void GetEntries(Array<Entry>& output) const override
    {
        _entries.GetValues(output);
    }

    void Dispose() override;

protected:

    // [FlaxStorage]
    bool GetEntry(const Guid& id, Entry& e) override;
    void AddEntry(Entry& e) override;
};
