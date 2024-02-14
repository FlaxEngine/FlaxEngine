//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Scripting/ScriptingType.h"

API_CLASS(NoSpawn) class FLAXENGINE_API UIBrushAsset : public JsonAssetBase
{
    DECLARE_ASSET_HEADER(UIBrushAsset);
public:
    class UIBrush* Brush;
};
