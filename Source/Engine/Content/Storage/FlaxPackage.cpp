// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "FlaxPackage.h"

bool FlaxPackage::HasAsset(const AssetInfo& info) const
{
    ASSERT(_path == info.Path);

    Entry* e = _entries.TryGet(info.ID);
    return e && e->TypeName == info.TypeName;
}

bool FlaxPackage::GetEntry(const Guid& id, Entry& e)
{
    return !_entries.TryGet(id, e);
}

void FlaxPackage::AddEntry(Entry& e)
{
    ASSERT(HasAsset(e.ID) == false);
    _entries.Add(e.ID, e);
}

void FlaxPackage::Dispose()
{
    // Base
    FlaxStorage::Dispose();

    // Clean
    _entries.Clear();
}
