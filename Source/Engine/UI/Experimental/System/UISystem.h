//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "UIBlueprint.h"
#include "Engine/Scripting/ScriptingType.h"

API_CLASS(static,Namespace = "FlaxEngine.Experimental.UI")
class UISystem
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(UISystem);

    API_FUNCTION() static UIBlueprint* CreateBlueprint();
    API_FUNCTION() static UIBlueprint* CreateFromBlueprintAsset(UIBlueprintAsset& Asset);
#if USE_EDITOR
    API_FUNCTION(internal) static void SaveBlueprint(UIBlueprint& InUIBlueprint);
    API_FUNCTION(internal) static UIBlueprint* LoadEditorBlueprintAsset(const StringView& InPath);
    API_FUNCTION(internal) static void AddDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags);
    API_FUNCTION(internal) static void RemoveDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags);
    API_FUNCTION(internal) static void SetDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags);
#endif
private:
    friend class UIBlueprintAsset;
    static UIComponent* DeserializeComponent(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier, Array<String>& Types, Array<UIBlueprint::Variable>& Variables);
    static void SerializeComponent(ISerializable::SerializeStream& stream, UIComponent* component, Array<String>& Types);
};
