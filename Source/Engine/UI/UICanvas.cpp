// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "UICanvas.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Serialization/Serialization.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

// Cached methods (FlaxEngine.CSharp.dll is loaded only once)
MMethod* UICanvas_Serialize = nullptr;
MMethod* UICanvas_SerializeDiff = nullptr;
MMethod* UICanvas_Deserialize = nullptr;
MMethod* UICanvas_PostDeserialize = nullptr;
MMethod* UICanvas_OnEnable = nullptr;
MMethod* UICanvas_OnDisable = nullptr;
MMethod* UICanvas_EndPlay = nullptr;

#define UICANVAS_INVOKE(event) \
    auto instance = GetManagedInstance(); \
    if (instance) \
    { \
	    MonoObject* exception = nullptr; \
	    UICanvas_##event->Invoke(instance, nullptr, &exception); \
	    if (exception) \
	    { \
		    MException ex(exception); \
		    ex.Log(LogType::Error, TEXT("UICanvas::" #event)); \
	    } \
    }

UICanvas::UICanvas(const SpawnParams& params)
    : Actor(params)
{
    Platform::MemoryBarrier();
    if (UICanvas_Serialize == nullptr)
    {
        MClass* mclass = GetClass();
        UICanvas_Serialize = mclass->GetMethod("Serialize");
        UICanvas_SerializeDiff = mclass->GetMethod("SerializeDiff", 1);
        UICanvas_Deserialize = mclass->GetMethod("Deserialize", 1);
        UICanvas_PostDeserialize = mclass->GetMethod("PostDeserialize");
        UICanvas_OnEnable = mclass->GetMethod("OnEnable");
        UICanvas_OnDisable = mclass->GetMethod("OnDisable");
        UICanvas_EndPlay = mclass->GetMethod("EndPlay");
    }
}

#if USE_EDITOR

BoundingBox UICanvas::GetEditorBox() const
{
    const Vector3 size(50);
    return BoundingBox(_transform.Translation - size, _transform.Translation + size);
}

#endif

void UICanvas::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(UICanvas);

    stream.JKEY("V");
    void* params[1];
    params[0] = other ? other->GetOrCreateManagedInstance() : nullptr;
    MonoObject* exception = nullptr;
    auto method = other ? UICanvas_SerializeDiff : UICanvas_Serialize;
    auto invokeResultStr = (MonoString*)method->Invoke(GetOrCreateManagedInstance(), params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("UICanvas::Serialize"));

        // Empty object
        stream.StartObject();
        stream.EndObject();
    }
    else
    {
        // Write result data
        auto invokeResultChars = mono_string_to_utf8(invokeResultStr);
        stream.RawValue(invokeResultChars);
        mono_free(invokeResultChars);
    }
}

void UICanvas::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    // Handle C# object data serialization
    const auto dataMember = stream.FindMember("V");
    if (dataMember != stream.MemberEnd())
    {
        rapidjson_flax::StringBuffer buffer;
        rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
        dataMember->value.Accept(writer);
        const auto str = buffer.GetString();
        void* args[1];
        args[0] = mono_string_new(mono_domain_get(), str);
        MonoObject* exception = nullptr;
        UICanvas_Deserialize->Invoke(GetOrCreateManagedInstance(), args, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT("UICanvas::Deserialize"));
        }
    }

    if (IsDuringPlay())
    {
        UICANVAS_INVOKE(PostDeserialize);
    }
}

void UICanvas::BeginPlay(SceneBeginData* data)
{
    UICANVAS_INVOKE(PostDeserialize);

    // Base
    Actor::BeginPlay(data);
}

void UICanvas::EndPlay()
{
    UICANVAS_INVOKE(EndPlay);

    // Base
    Actor::EndPlay();
}

void UICanvas::OnEnable()
{
    UICANVAS_INVOKE(OnEnable);

    // Base
    Actor::OnEnable();
}

void UICanvas::OnDisable()
{
    // Base
    Actor::OnDisable();

    UICANVAS_INVOKE(OnDisable);
}

void UICanvas::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
