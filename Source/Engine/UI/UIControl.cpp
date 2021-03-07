// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "UIControl.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Serialization/Serialization.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>

// Cached methods (FlaxEngine.CSharp.dll is loaded only once)
MMethod* UIControl_Serialize = nullptr;
MMethod* UIControl_SerializeDiff = nullptr;
MMethod* UIControl_Deserialize = nullptr;
MMethod* UIControl_ParentChanged = nullptr;
MMethod* UIControl_TransformChanged = nullptr;
MMethod* UIControl_OrderInParentChanged = nullptr;
MMethod* UIControl_ActiveInTreeChanged = nullptr;
MMethod* UIControl_BeginPlay = nullptr;
MMethod* UIControl_EndPlay = nullptr;

#define UICONTROL_INVOKE(event) \
	if (HasManagedInstance()) \
	{ \
	    MonoObject* exception = nullptr; \
	    UIControl_##event->Invoke(GetManagedInstance(), nullptr, &exception); \
	    if (exception) \
	    { \
		    MException ex(exception); \
		    ex.Log(LogType::Error, TEXT("UICanvas::" #event)); \
	    } \
	}

UIControl::UIControl(const SpawnParams& params)
    : Actor(params)
{
    if (UIControl_Serialize == nullptr)
    {
        MClass* mclass = GetClass();
        UIControl_Serialize = mclass->GetMethod("Serialize", 1);
        UIControl_SerializeDiff = mclass->GetMethod("SerializeDiff", 2);
        UIControl_Deserialize = mclass->GetMethod("Deserialize", 2);
        UIControl_ParentChanged = mclass->GetMethod("ParentChanged");
        UIControl_TransformChanged = mclass->GetMethod("TransformChanged");
        UIControl_OrderInParentChanged = mclass->GetMethod("OrderInParentChanged");
        UIControl_ActiveInTreeChanged = mclass->GetMethod("ActiveInTreeChanged");
        UIControl_BeginPlay = mclass->GetMethod("BeginPlay");
        UIControl_EndPlay = mclass->GetMethod("EndPlay");
    }
}

#if USE_EDITOR

BoundingBox UIControl::GetEditorBox() const
{
    const Vector3 size(50);
    return BoundingBox(_transform.Translation - size, _transform.Translation + size);
}

#endif

void UIControl::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(UIControl);

    void* params[2];
    MonoString* controlType = nullptr;
    params[0] = &controlType;
    params[1] = other ? other->GetOrCreateManagedInstance() : nullptr;
    MonoObject* exception = nullptr;
    const auto method = other ? UIControl_SerializeDiff : UIControl_Serialize;
    const auto invokeResultStr = (MonoString*)method->Invoke(GetOrCreateManagedInstance(), params, &exception);
    if (exception)
    {
        MException ex(exception);
        ex.Log(LogType::Error, TEXT("UIControl::Serialize"));
        return;
    }

    // No control
    if (!controlType)
    {
        if (!other)
        {
            stream.JKEY("Control");
            stream.String("", 0);

            stream.JKEY("Data");
            stream.RawValue("{}", 2);
        }
        return;
    }

    const auto controlTypeLength = mono_string_length(controlType);
    if (controlTypeLength != 0)
    {
        stream.JKEY("Control");
        const auto controlTypeChars = mono_string_to_utf8(controlType);
        stream.String(controlTypeChars);
        mono_free(controlTypeChars);
    }

    stream.JKEY("Data");
    const auto invokeResultChars = mono_string_to_utf8(invokeResultStr);
    stream.RawValue(invokeResultChars);
    mono_free(invokeResultChars);
}

void UIControl::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    MonoReflectionType* typeObj = nullptr;
    const auto controlMember = stream.FindMember("Control");
    if (controlMember != stream.MemberEnd())
    {
        const StringAnsiView controlType(controlMember->value.GetString(), controlMember->value.GetStringLength());
        const auto type = Scripting::FindClass(controlType);
        if (type != nullptr)
        {
            typeObj = mono_type_get_object(mono_domain_get(), mono_class_get_type(type->GetNative()));
        }
        else
        {
            LOG(Warning, "Unknown UIControl type: {0}", String(controlType));
        }
    }

    const auto dataMember = stream.FindMember("Data");
    if (dataMember != stream.MemberEnd())
    {
        rapidjson_flax::StringBuffer buffer;
        rapidjson_flax::Writer<rapidjson_flax::StringBuffer> writer(buffer);
        dataMember->value.Accept(writer);
        const auto str = buffer.GetString();
        void* args[2];
        args[0] = mono_string_new(mono_domain_get(), str);
        args[1] = typeObj;
        MonoObject* exception = nullptr;
        UIControl_Deserialize->Invoke(GetOrCreateManagedInstance(), args, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT("UIControl::Deserialize"));
        }
    }
}

void UIControl::OnParentChanged()
{
    // Base
    Actor::OnParentChanged();

    if (!IsDuringPlay())
        return;

    UICONTROL_INVOKE(ParentChanged);
}

void UIControl::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);

    UICONTROL_INVOKE(TransformChanged);
}

void UIControl::OnBeginPlay()
{
    UICONTROL_INVOKE(BeginPlay);

    // Base
    Actor::OnBeginPlay();
}

void UIControl::OnEndPlay()
{
    UICONTROL_INVOKE(EndPlay);

    // Base
    Actor::OnEndPlay();
}

void UIControl::OnOrderInParentChanged()
{
    // Base
    Actor::OnOrderInParentChanged();

    UICONTROL_INVOKE(OrderInParentChanged);
}

void UIControl::OnActiveInTreeChanged()
{
    // Base
    Actor::OnActiveInTreeChanged();

    UICONTROL_INVOKE(ActiveInTreeChanged);
}
