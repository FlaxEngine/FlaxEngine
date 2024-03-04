//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "UIBlueprintAsset.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include <Engine/Serialization/JsonWriters.h>

REGISTER_JSON_ASSET(UIBlueprintAsset, "FlaxEngine.UIBlueprintAsset", true);
UIBlueprintAsset::UIBlueprintAsset(const SpawnParams& params, const AssetInfo* info) : JsonAssetBase(params, info){}
JsonAssetBase::LoadResult UIBlueprintAsset::loadAsset() { return JsonAssetBase::loadAsset(); }

void UIBlueprintAsset::OnGetData(rapidjson_flax::StringBuffer& buffer) const
{
    if (Data == nullptr) {
        PrettyJsonWriter Final(buffer);
        {
            Final.StartObject();
            Final.JKEY("UIBlueprint");
            Final.String("", 0);
            Final.JKEY("TypeNames");
            Final.StartArray();
            Final.EndArray();
            Final.EndObject();
        }
    }
    else
    {
        JsonAssetBase::OnGetData(buffer);
    }
}
