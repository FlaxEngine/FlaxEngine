//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC

#include "../UISystem.h"


#include "Engine/Core/Log.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Engine/EngineService.h"

#include "Engine/Core/Cache.h"
#include "Engine/Content/Content.h"

#include "Engine/Core/ISerializable.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/Serialization.h"
#include <Editor/Editor.h>
#include <Engine/Engine/Time.h>

UIBlueprint* UISystem::CreateBlueprint()
{
    UIBlueprint* bp = ScriptingObject::NewObject<UIBlueprint>();
    return bp;
}
UIBlueprint* UISystem::CreateFromBlueprintAsset(UIBlueprintAsset& InAsset)
{
    return CreateFromAsset(InAsset,false);
}

void UISystem::SaveBlueprint(UIBlueprint& InUIBlueprint)
{
    if (InUIBlueprint.Asset)
    {
        auto asset = InUIBlueprint.Asset.Get();
        rapidjson_flax::StringBuffer buffer;
        PrettyJsonWriter Final(buffer);
        rapidjson_flax::StringBuffer dataBuffer;
        PrettyJsonWriter writerObj(dataBuffer);
        ISerializable::SerializeStream& stream = writerObj;
        Array<String> Types{};
        {
            Final.StartObject();
            UISystem::SerializeComponent(stream, InUIBlueprint.Component, Types);
            Final.JKEY("UIBlueprint");
            auto bluprintType = InUIBlueprint.GetType().Fullname;
            Final.String(bluprintType.GetText(), bluprintType.Length());
            Final.JKEY("TypeNames");
            Final.StartArray();
            for (auto i = 0; i < Types.Count(); i++)
            {
                Final.String(Types[i].ToStringAnsi().GetText(), Types[i].Length());
            }
            Final.EndArray();
            Final.JKEY("Tree");
            Final.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
            Final.EndObject();

        }
        //Final.EndObject();
        String& val = String(buffer.GetString());

        asset->SetData(val);
        asset->Save();
    }
    else
    {
        LOG(Warning, "[UIBlueprint][INVALID USAGE] Unable to save blueprint, The asset ref is missing");
    }
}

UIBlueprint* UISystem::LoadEditorBlueprintAsset(const StringView& InPath)
{
    Asset* asset = Content::LoadAsyncInternal(InPath, UIBlueprintAsset::TypeInitializer);
    if (asset != nullptr)
    {
        UIBlueprintAsset* bpasset = asset->Cast<UIBlueprintAsset>();
        if (bpasset)
        {
            asset->WaitForLoaded();
            return CreateFromAsset(*bpasset,true);
        }
        else
        {
            LOG(Warning, "[UIBlueprint] Cast to UIBlueprintAsset Has Faled");
        }
    }
    return nullptr;
}

 void UISystem::AddDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
{
    if (!comp)
        return;
    comp->DesignerFlags |= flags;
    if (auto c = comp->Cast<UIPanelComponent>())
    {
        for (auto s : c->GetSlots())
        {
            AddDesinerFlags(s->Content, flags);
        }
    }
}

 void UISystem::RemoveDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
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

 void UISystem::SetDesinerFlags(UIComponent* comp, UIComponentDesignFlags flags)
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

 UIComponent* UISystem::DeserializeComponent(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier, Array<String>& Types, Array<UIBlueprint::Variable>& Variables)
 {
     UIComponent* component = nullptr;
     const auto typeName = SERIALIZE_FIND_MEMBER(stream, "ID");
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
                         if (component->IsVariable)
                         {
                             Variables.Add(UIBlueprint::Variable(component->Label, component));
                             component->CreatedByUIBlueprint = true;
                         }
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
                 LOG(Error, "[UIBlueprint] Found unkonw scriptingType at {0} during deserializesion", id);
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
             for (unsigned int i = 0; i < elements.Size(); i++)
             {
                 ISerializable::DeserializeStream& slotstream = (ISerializable::DeserializeStream)elements[i].GetObject();
                 const auto childComponent = DeserializeComponent(slotstream, modifier, Types, Variables);
                 if (childComponent)
                 {
                     panelComponent->AddChild(childComponent);
                     if (childComponent->Slot != nullptr)
                         childComponent->Slot->Deserialize(slotstream, modifier);
                 }
             }
         }
     }
     return component;
 }
 void UISystem::SerializeComponent(ISerializable::SerializeStream& stream, UIComponent* component, Array<String>& Types)
 {
     if (component->CreatedByUIBlueprint)
     {
         stream.StartObject();
         //type
         stream.JKEY("ID");
         const String& typeName = component->GetType().Fullname.ToString();
         int32 ID = Types.Find(typeName);
         if (ID == -1)
         {
             Types.Add(typeName);
             stream.Int(Types.Count() - 1);
         }
         else
         {
             stream.Int(ID);
         }
         //data
         component->Serialize(stream, nullptr);
         if (component->Slot != nullptr)
             component->Slot->Serialize(stream, nullptr);

         auto panelComponent = ScriptingObject::Cast<UIPanelComponent>(component);
         if (panelComponent)
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
 }

 UIBlueprint* UISystem::CreateFromAsset(UIBlueprintAsset& InAsset, bool ForEditor)
 {
     UIBlueprint* bp = nullptr;
     const auto result = InAsset.loadAsset();
     if (result == UIBlueprintAsset::LoadResult::Ok)
     {
         auto modifier = Cache::ISerializeModifier.Get();
         ISerializable::DeserializeStream& stream = (*InAsset.Data);
         const auto bluprintTypeName = SERIALIZE_FIND_MEMBER(stream, "UIBlueprint");
         if (bluprintTypeName)
         {
             if (bluprintTypeName != stream.MemberEnd())
             {
                 auto type = bluprintTypeName->value.GetStringAnsiView();
                 auto scriptingType = Scripting::FindScriptingType(type);
                 if (scriptingType)
                 {
                     auto object = ScriptingObject::NewObject(scriptingType);
                     if (object)
                     {
                         if (auto casted = object->Cast<UIBlueprint>())
                         {
                             bp = casted;
                         }
                         else
                         {
                             LOG(Error, "[UIBlueprint] Cast of {0} to UIBlueprint Type has faled", type.ToString());
                             return nullptr;
                         }
                     }
                     else
                     {
                         LOG(Error, "[UIBlueprint] Cant create UIBlueprint Type: {0}", type.ToString());
                         return nullptr;
                     }
                 }
                 else
                 {
                     LOG(Error, "[UIBlueprint] Unkown UIBlueprint Type: {0}", type.ToString());
                     return nullptr;
                 }
             }
         }
         else
         {
             LOG(Warning, "[UIBlueprint] Missing UIBlueprint Script");
         }



         const auto TypeNames = SERIALIZE_FIND_MEMBER(stream, "TypeNames");
         if (TypeNames)
         {
             if (TypeNames != stream.MemberEnd())
             {
                 if (TypeNames->value.IsArray())
                 {
                     const auto& streamArray = TypeNames->value.GetArray();
                     Array<String> Types{};
                     Types.Resize(streamArray.Size());
                     for (auto i = 0; i < Types.Count(); i++)
                     {
                         Types[i] = streamArray[i].GetText();
                     }
                     if (Types.IsEmpty())
                     {
                         LOG(Error, "[UIBlueprint] Invalid Data Structure the TypeNames are missing");
                         return nullptr;
                     }
                     const auto Tree = SERIALIZE_FIND_MEMBER(stream, "Tree");
                     if (Tree != stream.MemberEnd())
                     {
                         if (bp == nullptr)
                         {
                             bp = ScriptingObject::NewObject<UIBlueprint>();
                         }
                         bp->Component = DeserializeComponent((ISerializable::DeserializeStream)Tree->value.GetObject(), modifier.Value, Types, bp->Variables);
                         bp->Asset.Set(&InAsset);
                         bp->OnInitialized();
                     }
                 }
             }
         }
         else
         {
             LOG(Warning, "[UIBlueprint] Missing UIBlueprint TypeNames field");
         }
     }
     //if (ForEditor)
     //{
     //    UISystemService::EditorTick.Bind<UIBlueprint, &UIBlueprint::Tick>(bp);
     //    bp->Deleted.Bind(
     //        [bp](ScriptingObject* obj) 
     //        {
     //            UISystemService::EditorTick.Unbind<UIBlueprint, &UIBlueprint::Tick>(bp);
     //        }
     //    );
     //}
     //else
     //{
     //    UISystemService::GameTick.Bind<UIBlueprint, &UIBlueprint::Tick>(bp);
     //    bp->Deleted.Bind(
     //        [bp](ScriptingObject* obj)
     //        {
     //            UISystemService::GameTick.Unbind<UIBlueprint, &UIBlueprint::Tick>(bp);
     //        }
     //    );
     //}
     
     return bp;
 }
