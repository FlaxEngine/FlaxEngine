#include "UIBlueprintAsset.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Core/Cache.h"

#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"
#include <rapidjson/document.h>

REGISTER_JSON_ASSET(UIBlueprintAsset, "FlaxEngine.UIBlueprintAsset", true);

UIBlueprintAsset::UIBlueprintAsset(const SpawnParams& params, const AssetInfo* info)
    : JsonAssetBase(params, info)
    , Component(nullptr)
{
}
void UIBlueprintAsset::OnGetData(rapidjson_flax::StringBuffer& buffer) const
{
    PrettyJsonWriter Final(buffer);
    rapidjson_flax::StringBuffer dataBuffer;
    PrettyJsonWriter writerObj(dataBuffer);
    ISerializable::SerializeStream& stream = writerObj;
    Array<StringAnsiView> Types{};
    SerializeComponent(stream,Component, Types);


    Final.StartObject();
    Final.JKEY("TypeNames");
    //writer.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
    Final.StartArray();
    for (auto i = 0; i < Types.Count(); i++)
    {
        Final.String(Types[i].GetText(), Types[i].Length());
    }
    Final.EndArray();
    Final.JKEY("Tree");
    Final.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
    Final.EndObject();

    //const auto TypeNames = SERIALIZE_FIND_MEMBER(Document, "TypeNames");
    //if (TypeNames != Document.MemberEnd())
    //{
    //    const auto elements = TypeNames->value.GetArray();
    //    ISerializable::SerializeStream& stream = (ISerializable::SerializeStream&)TypeNames->value.GetArray();
    //    for (unsigned int i = 0; i < elements.Size(); i++)
    //    {
    //        stream.String(Types[i]);
    //    }
    //    //TypeNames->value Types
    //}
}
Asset::LoadResult UIBlueprintAsset::loadAsset()
{
    // Base
    const auto result = JsonAssetBase::loadAsset();
    if (result == LoadResult::Ok)
    {
        auto modifier = Cache::ISerializeModifier.Get();
        ISerializable::DeserializeStream& stream = *Data;
        const auto TypeNames = SERIALIZE_FIND_MEMBER(stream, "TypeNames");
        if (TypeNames)
        {
            if (TypeNames != Document.MemberEnd())
            {
                if (TypeNames->value.IsArray()) 
                {
                    const auto rTypes = TypeNames->value.GetArray();
                    Array<StringAnsiView>& Types = Array<StringAnsiView>(rTypes.Size());
                    for (unsigned int i = 0; i < rTypes.Size(); i++)
                    {
                        Types[i] = rTypes[i].GetStringAnsiView();
                    }
                    const auto Tree = SERIALIZE_FIND_MEMBER(stream, "Tree");
                    if (Tree != Document.MemberEnd())
                    {
                        auto comp = DeserializeComponent((ISerializable::DeserializeStream)Tree->value.GetObject(), modifier.Value, Types);
                        if (comp)
                        {
                            Component = comp;
                        }
                    }
                }
            }
        }
    }
    return result;
}
//void UIBlueprintAsset::DeseralizeSlots(ISerializable::DeserializeStream& obj, ISerializeModifier* modifier, UIPanelComponent* parent)
//{
//    
//    const auto e = SERIALIZE_FIND_MEMBER(obj, "TypeName");
//    if (e != obj.MemberEnd())
//    {
//        auto klass = Scripting::FindScriptingType(e->value.GetString());
//        if (klass)
//        {
//            auto so = ScriptingObject::NewObject(klass);
//            auto component = ScriptingObject::Cast<UIComponent>(so);
//            if (component)
//            {
//                if (parent)
//                {
//                    component->Deserialize(obj, modifier);
//                    parent->AddChild(component);
//                }
//                const auto e = obj.FindMember(rapidjson_flax::Value("Slots", (sizeof("Slots") / sizeof("Slots"[0])) - 1));
//
//                if (e != obj.MemberEnd())
//                {
//                    auto panelComponent = ScriptingObject::Cast<UIPanelComponent>(so);
//                    if (panelComponent)
//                    {
//                        auto ar = e->value.GetArray();
//                        for (unsigned int i = 0; i < ar.Size(); i++)
//                        {
//                            DeseralizeSlots((ISerializable::DeserializeStream)ar[i].GetObject(), modifier, panelComponent);
//                        }
//                    }
//                }
//            }
//        }
//    }
//}
void UIBlueprintAsset::unload(bool isReloading)
{
    // Base
    JsonAssetBase::unload(isReloading);
    if (!isReloading && Component)
        Delete(Component);
}

UIComponent* UIBlueprintAsset::DeserializeComponent(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier, Array<StringAnsiView>& Types)
{
    UIComponent* component = nullptr;
    const auto typeName  = SERIALIZE_FIND_MEMBER(stream,"TypeName");
    if (typeName != stream.MemberEnd())
    {
        auto scriptingType = Scripting::FindScriptingType(typeName->value.GetString());
        if (scriptingType)
        {
            auto object = ScriptingObject::NewObject(scriptingType);
            if(object)
            {
                component = ScriptingObject::Cast<UIComponent>(object);
                if(component)
                {
                    component->Deserialize(stream, modifier);
                }
                else
                {
                    LOG(Error, "[UIBlueprint] Found incompatible type {0} with {1} during deserializesion", scriptingType.GetType().Fullname.ToString(), UIComponent::TypeInitializer.GetType().Fullname.ToString());
                }
            }
            else
            {
                LOG(Error, "[UIBlueprint] Faled to create type {0} during deserializesion", scriptingType.GetType().Fullname.ToString());
            }
        }
        else
        {
            LOG(Error, "[UIBlueprint] Found unkonw type {0} during deserializesion", scriptingType.GetType().Fullname.ToString());
        }
    }
    else
    {
        LOG(Error, "[UIBlueprint] can't find TypeName field");
    }

    auto panelComponent = ScriptingObject::Cast<UIPanelComponent>(component);
    if (panelComponent)
    {
        const auto slots = SERIALIZE_FIND_MEMBER(stream, "Slots");
        if (slots != stream.MemberEnd()) 
        {
            const auto elements = slots->value.GetArray();
            for (unsigned int  i = 0; i < elements.Size(); i++)
            {
                ISerializable::DeserializeStream& slotstream = (ISerializable::DeserializeStream)elements[i].GetObject();
                DeserializeComponent(slotstream, modifier, Types);
            }
        }
    }
    return nullptr;
}

void UIBlueprintAsset::SerializeComponent(ISerializable::SerializeStream& stream, UIComponent* component, Array<StringAnsiView>& Types)
{
    stream.StartObject();
    //type
    stream.JKEY("ID");
    const auto typeName = component->GetType().Fullname;
    int32 ID = -1;
    for (int32 i = 0; i < Types.Count(); i++)
    {
        if (typeName.Length() != Types[i].Length())
        {
            if (typeName.Compare(Types[i]))
            {
                ID = i;
                break;
            }
        }
    }
    if(ID != -1)
    {
        stream.Int(ID);
    }
    else
    {
        Types.Add(typeName);
        stream.Int(Types.Count()-1);
    }
    //stream.String();
    //data
    component->Serialize(stream, nullptr);
    
    auto panelComponent = ScriptingObject::Cast<UIPanelComponent>(component);
    if(panelComponent)
    {
        if (panelComponent->HasAnyChildren()) 
        {
            Array<UIComponent*> children = panelComponent->GetAllChildren();
            stream.JKEY("Slots");
            stream.StartArray();
            for (auto i = 0; i < children.Count(); i++)
            {
                SerializeComponent(stream, children[i], Types);
            }
            stream.EndArray();
        }
    }
    stream.EndObject();
}

