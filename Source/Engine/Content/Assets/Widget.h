#pragma once
#include "RawDataAsset.h"

API_CLASS(NoSpawn) class FLAXENGINE_API Widget : public RawDataAsset
{
    DECLARE_BINARY_ASSET_HEADER(Widget, 1);
protected:
    API_FIELD() String tData;
public:
#if USE_EDITOR
    void OnSave() override;
#endif
public:
    // [BinaryAsset]
    uint64 GetMemoryUsage() const override;

protected:
    LoadResult load() override;
    void unload(bool isReloading) override;
};
API_CLASS(NoSpawn) class FLAXENGINE_API EditorWidget : public Widget
{
    DECLARE_BINARY_ASSET_HEADER(EditorWidget, 1);
};
