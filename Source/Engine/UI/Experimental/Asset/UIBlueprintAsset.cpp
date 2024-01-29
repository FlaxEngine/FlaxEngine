//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#include "UIBlueprintAsset.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"

REGISTER_JSON_ASSET(UIBlueprintAsset, "FlaxEngine.UIBlueprintAsset", true);
UIBlueprintAsset::UIBlueprintAsset(const SpawnParams& params, const AssetInfo* info) : JsonAssetBase(params, info){}
JsonAssetBase::LoadResult UIBlueprintAsset::loadAsset() { return JsonAssetBase::loadAsset(); }
