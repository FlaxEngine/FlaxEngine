//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
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
    Array<String> Types{};

    Final.StartObject();
    if (Component != nullptr)
    {
        SerializeComponent(stream, Component, Types);
        Final.JKEY("TypeNames");
        //writer.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
        Final.StartArray();
        for (auto i = 0; i < Types.Count(); i++)
        {
            Final.String(Types[i].ToStringAnsi().GetText(), Types[i].Length());
        }
        Final.EndArray();
        Final.JKEY("Tree");
        Final.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
    }
    Final.EndObject();
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
            if (TypeNames != stream.MemberEnd())
            {
                if (TypeNames->value.IsArray())
                {
                    const auto& streamArray = TypeNames->value.GetArray();
                    Array<String> Types {};
                    Types.Resize(streamArray.Size());
                    for (auto i = 0; i < Types.Count(); i++)
                    {
                        Types[i] = streamArray[i].GetText();
                    }
                    if (Types.IsEmpty())
                    {
                        LOG(Error, "[UIBlueprint] Invalid Data Structure the TypeNames are missing");
                        return LoadResult::MissingDataChunk;
                    }
                    const auto Tree = SERIALIZE_FIND_MEMBER(stream, "Tree");
                    if (Tree != stream.MemberEnd())
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

void UIBlueprintAsset::unload(bool isReloading)
{
    // Base
    JsonAssetBase::unload(isReloading);
    if (!isReloading && Component)
        Delete(Component);
}

UIComponent* UIBlueprintAsset::DeserializeComponent(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier, Array<String>& Types)
{
    UIComponent* component = nullptr;
    const auto typeName  = SERIALIZE_FIND_MEMBER(stream,"ID");
    if (typeName != stream.MemberEnd())
    {
        auto id = typeName->value.GetInt();
        if (Types.IsValidIndex(id))
        {
            auto scriptingType = Scripting::FindScriptingType(Types[id].ToStringAnsi());
            if (scriptingType)
            {
                auto object = ScriptingObject::NewObject(scriptingType);
                if (object)
                {
                    component = ScriptingObject::Cast<UIComponent>(object);
                    if (component)
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
            LOG(Error, "[UIBlueprint] Found unkonw type ID {0} during deserializesion", id);
        }
    }
    else
    {
        LOG(Error, "[UIBlueprint] can't find ID field");
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
                const auto childComponent = DeserializeComponent(slotstream, modifier, Types);
                if (childComponent)
                {
                    panelComponent->AddChild(childComponent);
                }
            }
        }
    }
    return component;
}

void UIBlueprintAsset::SerializeComponent(ISerializable::SerializeStream& stream, UIComponent* component, Array<String>& Types)
{
    stream.StartObject();
    //type
    stream.JKEY("ID");
    const String& typeName = component->GetType().Fullname.ToString();
    int32 ID = Types.Find(typeName);
    if(ID == -1)
    {
        Types.Add(typeName);
        stream.Int(Types.Count() - 1);
    }
    else
    {
        stream.Int(ID);
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

