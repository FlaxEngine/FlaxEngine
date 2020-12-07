// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "FlaxStorage.h"

/// <summary>
/// Flax Game Engine asset files container object.
/// </summary>
class FLAXENGINE_API FlaxFile : public FlaxStorage
{
    friend ContentStorageManager;

protected:

    Entry _asset;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="FlaxFile"/> class.
    /// </summary>
    /// <param name="path">The path.</param>
    FlaxFile(const StringView& path)
        : FlaxStorage(path)
    {
        _asset.ID = Guid::Empty;
    }

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="FlaxFile"/> class.
    /// </summary>
    ~FlaxFile()
    {
    }

public:

    // [FlaxStorage]
    String ToString() const override
    {
        return String::Format(TEXT("Asset \'{0}\'"), _path);
    }

    bool IsPackage() const override
    {
        return false;
    }

    bool AllowDataModifications() const override
    {
        return true;
    }

    bool HasAsset(const Guid& id) const override
    {
        return _asset.ID == id;
    }

    bool HasAsset(const AssetInfo& info) const override
    {
#if USE_EDITOR
        if (_path != info.Path)
            return false;
#endif
        return _asset.ID == info.ID && _asset.TypeName == info.TypeName;
    }

    int32 GetEntriesCount() const override
    {
        return _asset.ID.IsValid() ? 1 : 0;
    }

    void GetEntry(int32 index, Entry& output) const override
    {
        ASSERT(index == 0);
        output = _asset;
    }

    void GetEntries(Array<Entry>& output) const override
    {
        if (_asset.ID.IsValid())
            output.Add(_asset);
    }

    void Dispose() override
    {
        // Base
        FlaxStorage::Dispose();

        // Clean
        _asset.ID = Guid::Empty;
    }

protected:

    // [FlaxStorage]
    bool GetEntry(const Guid& id, Entry& e) override
    {
        e = _asset;
        return id != _asset.ID;
    }

    void AddEntry(Entry& e) override
    {
        ASSERT(_asset.ID.IsValid() == false);
        _asset = e;
    }
};
