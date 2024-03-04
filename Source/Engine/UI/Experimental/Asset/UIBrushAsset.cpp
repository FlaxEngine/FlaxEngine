//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "UIBrushAsset.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include <Engine/Serialization/JsonWriters.h>

REGISTER_JSON_ASSET(UIBrushAsset, "FlaxEngine.UIBrushAsset", false);
UIBrushAsset::UIBrushAsset(const SpawnParams& params, const AssetInfo* info) : JsonAssetBase(params, info)
{
    Brush = nullptr;
}
