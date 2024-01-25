//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once

#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/ScriptingType.h"
#include "../Types/UIPanelComponent.h"

API_CLASS(NoSpawn) class FLAXENGINE_API UIBlueprintAsset : public JsonAssetBase
{
    DECLARE_ASSET_HEADER(UIBlueprintAsset);
    API_FIELD(internal) UIComponent* Component;
    API_STRUCT(internal) struct Variable
    {
        Variable() { name = ""; comp = nullptr; }
        Variable(String name, UIComponent* comp) : name(name), comp(comp) {}

        DECLARE_SCRIPTING_TYPE_MINIMAL(UIComponentTransform);
        API_FIELD(internal) String name;

        //[TODO] Use "shared object" for 'comp' so there is no possibility of "access validation" exeption.
        // user of the API can just remove the component and pointer will be invalid
        API_FIELD(internal) UIComponent* comp;
    };
    API_FIELD(internal) Array<Variable> Variables;
protected:
    virtual void OnGetData(rapidjson_flax::StringBuffer& buffer) const override;

    // [Asset]
    LoadResult loadAsset() override;
    void unload(bool isReloading) override;

private:
    static UIComponent* DeserializeComponent(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier, Array<String>& Types, Array<Variable>& Variables);
    static void SerializeComponent(ISerializable::SerializeStream& stream, UIComponent* component, Array<String>& Types);

public:
    API_FUNCTION(internal) UIComponent* CreateInstance();

    API_FUNCTION(internal) static void SendEvent(UIComponent* InFromUIComponent, const UIPointerEvent& InEvent,API_PARAM(Out)const UIComponent*& OutHit, API_PARAM(Out) UIEventResponse& OutEventResponse);
#if USE_EDITOR
    /// <summary>
    /// Adds the desiner flags. [Editor Only]
    /// </summary>
    /// <param name="comp">The comp.</param>
    /// <param name="flags">The flags.</param>
    API_FUNCTION(internal) static void AddDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
    {
        if (!comp)
            return;
        comp->DesignerFlags |= flags;
        if(auto c = comp->Cast<UIPanelComponent>())
        {
            for (auto s : c->GetSlots())
            {
                AddDesinerFlags(s->Content, flags);
            }
        }
    }
    /// <summary>
    /// Removes the desiner flags. [Editor Only]
    /// </summary>
    /// <param name="comp">The comp.</param>
    /// <param name="flags">The flags.</param>
    API_FUNCTION(internal) static void RemoveDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
    {
        if (!comp)
            return;
        comp->DesignerFlags &= ~flags;
        if (auto c = comp->Cast<UIPanelComponent>())
        {
            for (auto s : c->GetSlots())
            {
                AddDesinerFlags(s->Content, flags);
            }
        }
    }

    /// <summary>
    /// Sets the desiner flags. [Editor Only]
    /// </summary>
    /// <param name="comp">The comp.</param>
    /// <param name="flags">The flags.</param>
    API_FUNCTION(internal) static void SetDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
    {
        if (!comp)
            return;
        comp->DesignerFlags = flags;
        if (auto c = comp->Cast<UIPanelComponent>())
        {
            for (auto s : c->GetSlots())
            {
                AddDesinerFlags(s->Content, flags);
            }
        }
    }
#endif
};
