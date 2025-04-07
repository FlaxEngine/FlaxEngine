// Copyright (c) Wojciech Figat. All rights reserved.

#include "UIControl.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Serialization/Serialization.h"

#if COMPILE_WITHOUT_CSHARP
#define UICONTROL_INVOKE(event)
#else
// Cached methods (FlaxEngine.CSharp.dll is loaded only once)
MMethod* UIControl_Serialize = nullptr;
MMethod* UIControl_Deserialize = nullptr;
MMethod* UIControl_ParentChanged = nullptr;
MMethod* UIControl_TransformChanged = nullptr;
MMethod* UIControl_OrderInParentChanged = nullptr;
MMethod* UIControl_ActiveChanged = nullptr;
MMethod* UIControl_BeginPlay = nullptr;
MMethod* UIControl_EndPlay = nullptr;

#define UICONTROL_INVOKE(event) \
    auto* managed = GetManagedInstance(); \
    if (managed) \
	{ \
	    MObject* exception = nullptr; \
	    UIControl_##event->Invoke(managed, nullptr, &exception); \
	    if (exception) \
	    { \
		    MException ex(exception); \
		    ex.Log(LogType::Error, TEXT("UICanvas::" #event)); \
	    } \
	}
#endif

UIControl::UIControl(const SpawnParams& params)
    : Actor(params)
{
#if !COMPILE_WITHOUT_CSHARP
    Platform::MemoryBarrier();
    if (UIControl_Serialize == nullptr)
    {
        MClass* mclass = GetClass();
        UIControl_Deserialize = mclass->GetMethod("Deserialize", 2);
        UIControl_ParentChanged = mclass->GetMethod("ParentChanged");
        UIControl_TransformChanged = mclass->GetMethod("TransformChanged");
        UIControl_OrderInParentChanged = mclass->GetMethod("OrderInParentChanged");
        UIControl_ActiveChanged = mclass->GetMethod("ActiveChanged");
        UIControl_BeginPlay = mclass->GetMethod("BeginPlay");
        UIControl_EndPlay = mclass->GetMethod("EndPlay");
        UIControl_Serialize = mclass->GetMethod("Serialize", 2);
        Platform::MemoryBarrier();
    }
#endif
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
    SERIALIZE_MEMBER(NavTargetUp, _navTargetUp);
    SERIALIZE_MEMBER(NavTargetDown, _navTargetDown);
    SERIALIZE_MEMBER(NavTargetLeft, _navTargetLeft);
    SERIALIZE_MEMBER(NavTargetRight, _navTargetRight);

#if !COMPILE_WITHOUT_CSHARP
    void* params[2];
    MString* controlType = nullptr;
    params[0] = &controlType;
    params[1] = other ? other->GetOrCreateManagedInstance() : nullptr;
    MObject* exception = nullptr;
    const auto invokeResultStr = (MString*)UIControl_Serialize->Invoke(GetOrCreateManagedInstance(), params, &exception);
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

    const StringView controlTypeChars = MCore::String::GetChars(controlType);
    if (controlTypeChars.Length() != 0)
    {
        stream.JKEY("Control");
        stream.String(controlTypeChars);
    }

    const StringView invokeResultStrChars = MCore::String::GetChars(invokeResultStr);
    stream.JKEY("Data");
    stream.RawValue(invokeResultStrChars);
#endif
}

void UIControl::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(NavTargetUp, _navTargetUp);
    DESERIALIZE_MEMBER(NavTargetDown, _navTargetDown);
    DESERIALIZE_MEMBER(NavTargetLeft, _navTargetLeft);
    DESERIALIZE_MEMBER(NavTargetRight, _navTargetRight);

#if !COMPILE_WITHOUT_CSHARP
    MTypeObject* typeObj = nullptr;
    const auto controlMember = stream.FindMember("Control");
    if (controlMember != stream.MemberEnd())
    {
        const StringAnsiView controlType(controlMember->value.GetStringAnsiView());
        const MClass* type = Scripting::FindClass(controlType);
        if (type != nullptr)
        {
            typeObj = INTERNAL_TYPE_GET_OBJECT(type->GetType());
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
        void* args[2];
        args[0] = MCore::String::New(buffer.GetString(), (int32)buffer.GetSize());
        args[1] = typeObj;
        MObject* exception = nullptr;
        UIControl_Deserialize->Invoke(GetOrCreateManagedInstance(), args, &exception);
        if (exception)
        {
            MException ex(exception);
            ex.Log(LogType::Error, TEXT("UIControl::Deserialize"));
        }
    }
#endif
}

void UIControl::OnParentChanged()
{
    // Base
    Actor::OnParentChanged();

    UICONTROL_INVOKE(ParentChanged);
}

void UIControl::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
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

void UIControl::OnActiveChanged()
{
    UICONTROL_INVOKE(ActiveChanged);

    // Base
    Actor::OnActiveChanged();
}

#if !COMPILE_WITHOUT_CSHARP

void UIControl::GetNavTargets(UIControl*& up, UIControl*& down, UIControl*& left, UIControl*& right) const
{
    up = _navTargetUp;
    down = _navTargetDown;
    left = _navTargetLeft;
    right = _navTargetRight;
}

void UIControl::SetNavTargets(UIControl* up, UIControl* down, UIControl* left, UIControl* right)
{
    _navTargetUp = up;
    _navTargetDown = down;
    _navTargetLeft = left;
    _navTargetRight = right;
}

#endif
