// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "MEvent.h"

#if USE_MONO

#include "MType.h"
#include "MClass.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>

MEvent::MEvent(MonoEvent* monoEvent, const char* name, MClass* parentClass)
    : _monoEvent(monoEvent)
    , _addMethod(nullptr)
    , _removeMethod(nullptr)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
    , _hasAddMonoMethod(true)
    , _hasRemoveMonoMethod(true)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_event_get_name(monoEvent)) == 0);
#endif
}

MType MEvent::GetType()
{
    if (GetAddMethod() != nullptr)
        return GetAddMethod()->GetReturnType();
    if (GetRemoveMethod() != nullptr)
        return GetRemoveMethod()->GetReturnType();
    CRASH;
    return MType();
}

MMethod* MEvent::GetAddMethod()
{
    if (!_hasAddMonoMethod)
        return nullptr;
    if (_addMethod == nullptr)
    {
        auto addMonoMethod = mono_event_get_add_method(_monoEvent);
        if (addMonoMethod != nullptr)
        {
            _hasAddMonoMethod = true;
            return _addMethod = New<MMethod>(addMonoMethod, _parentClass);
        }
    }
    return _addMethod;
}

MMethod* MEvent::GetRemoveMethod()
{
    if (!_hasRemoveMonoMethod)
        return nullptr;
    if (_removeMethod == nullptr)
    {
        auto removeMonoMethod = mono_event_get_remove_method(_monoEvent);
        if (removeMonoMethod)
        {
            _hasRemoveMonoMethod = true;
            return _removeMethod = New<MMethod>(removeMonoMethod, _parentClass);
        }
        return nullptr;
    }
    return _removeMethod;
}

bool MEvent::HasAttribute(MClass* monoClass) const
{
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, monoClass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MEvent::HasAttribute() const
{
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return false;

    if (attrInfo->num_attrs > 0)
    {
        mono_custom_attrs_free(attrInfo);
        return true;
    }
    mono_custom_attrs_free(attrInfo);
    return false;
}

MonoObject* MEvent::GetAttribute(MClass* monoClass) const
{
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, monoClass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MonoObject*>& MEvent::GetAttributes()
{
    if (_hasCachedAttributes)
        return _attributes;

    _hasCachedAttributes = true;
    MonoClass* parentClass = mono_event_get_parent(_monoEvent);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_event(parentClass, _monoEvent);
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const auto length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject*, i);
    mono_custom_attrs_free(attrInfo);
    return _attributes;
}

#endif
