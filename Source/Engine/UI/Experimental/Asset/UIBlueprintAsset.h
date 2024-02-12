//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Scripting/ScriptingType.h"

API_CLASS(NoSpawn) class FLAXENGINE_API UIBlueprintAsset : public JsonAssetBase
{
    DECLARE_ASSET_HEADER(UIBlueprintAsset);
protected:
    //System
    friend class UISystem;
    // [Asset]
    LoadResult loadAsset() override;
    virtual void OnGetData(rapidjson_flax::StringBuffer& buffer) const override;
};
