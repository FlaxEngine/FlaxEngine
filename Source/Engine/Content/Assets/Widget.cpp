// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Widget.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Platform/FileSystem.h"
#if USE_EDITOR
#include "Engine/Threading/Threading.h"
#endif

REGISTER_BINARY_ASSET(Widget, "FlaxEngine.Widget", true);
REGISTER_BINARY_ASSET(EditorWidget, "FlaxEditor.Widget", true);

Widget::Widget(const SpawnParams& params, const AssetInfo* info) : RawDataAsset(params, info){}
EditorWidget::EditorWidget(const SpawnParams& params, const AssetInfo* info) : Widget(params, info){}

#if USE_EDITOR
void Widget::OnSave()
{
    RawDataAsset::Data.Resize(tData.Length());
    for (auto i = 0; i < Data.Count(); i++)
    {
        auto c = tData[i];
        // w_char to unsigned char (byte) to lower the file size
        // this make this asset a mix between binary and text.
        // as a side effects with is desired the asset can work with github
        // code is not expecting to get any chracters outsude the ASCII range
        RawDataAsset::Data[i] = (c < 0) ? (c + 256) : (c > 256) ? 255 : c;
    }
}
#endif
uint64 Widget::GetMemoryUsage() const
{
    auto n = RawDataAsset::GetMemoryUsage();
    return n;
}

Asset::LoadResult Widget::load()
{
    auto r = RawDataAsset::load();
    if (r == Asset::LoadResult::Ok) 
    {
        //deseralize
        tData.Resize(RawDataAsset::Data.Count());
        for (auto i = 0; i < Data.Count(); i++)
        {
            tData[i] = RawDataAsset::Data[i];
        }
    }
    return r;
}

void Widget::unload(bool isReloading)
{
    RawDataAsset::unload(isReloading);
}
