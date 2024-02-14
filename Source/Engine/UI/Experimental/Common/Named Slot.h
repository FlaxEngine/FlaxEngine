//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "../Types/UIPanelComponent.h"
#include "Engine/Content/AssetReference.h"
#include <Engine/Content/Assets/Texture.h>

API_CLASS(Namespace = "FlaxEngine.Experimental.UI", Attributes = "UIDesigner(DisplayLabel=\"NamedSlot\",CategoryName=\"Common\",EditorComponent=false,HiddenInDesigner=false)")
class UINamedSlot : public UIPanelComponent
{
    DECLARE_SCRIPTING_TYPE(UINamedSlot);
};
