// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "MProperty.h"
#include "Engine/Core/Math/Math.h"
#include "MMethod.h"
#include "MClass.h"
#include "MType.h"
#if USE_MONO
#include <mono/metadata/mono-debug.h>

MProperty::MProperty(MonoProperty* monoProperty, const char* name, MClass* parentClass)
    : _monoProperty(monoProperty)
    , _getMethod(nullptr)
    , _setMethod(nullptr)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
    , _hasSetMethod(true)
    , _hasGetMethod(true)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_property_get_name(monoProperty)) == 0);
#endif

    GetGetMethod();
    GetSetMethod();
}

#endif

MProperty::~MProperty()
{
    if (_getMethod)
        Delete(_getMethod);
    if (_setMethod)
        Delete(_setMethod);
}

MType MProperty::GetType()
{
    if (GetGetMethod() != nullptr)
        return GetGetMethod()->GetReturnType();
    return GetSetMethod()->GetReturnType();
}

MMethod* MProperty::GetGetMethod()
{
    if (!_hasGetMethod)
        return nullptr;
    if (_getMethod == nullptr)
    {
#if USE_MONO
        auto method = mono_property_get_get_method(_monoProperty);
        if (method != nullptr)
        {
            _hasGetMethod = true;
            return _getMethod = New<MMethod>(method, _parentClass);
        }
#endif
    }
    return _getMethod;
}

MMethod* MProperty::GetSetMethod()
{
    if (!_hasSetMethod)
        return nullptr;
    if (_setMethod == nullptr)
    {
#if USE_MONO
        auto method = mono_property_get_set_method(_monoProperty);
        if (method != nullptr)
        {
            _hasSetMethod = true;
            return _setMethod = New<MMethod>(method, _parentClass);
        }
#endif
    }
    return _setMethod;
}

MVisibility MProperty::GetVisibility()
{
    if (GetGetMethod() && GetSetMethod())
    {
        return static_cast<MVisibility>(
            Math::Max(
                static_cast<int>(GetGetMethod()->GetVisibility()),
                static_cast<int>(GetSetMethod()->GetVisibility())
            ));
    }
    if (GetGetMethod())
    {
        return GetGetMethod()->GetVisibility();
    }
    return GetSetMethod()->GetVisibility();
}

bool MProperty::IsStatic()
{
    if (GetGetMethod())
    {
        return GetGetMethod()->IsStatic();
    }
    if (GetSetMethod())
    {
        return GetSetMethod()->IsStatic();
    }
    return false;
}

MObject* MProperty::GetValue(MObject* instance, MObject** exception)
{
#if USE_MONO
    return mono_property_get_value(_monoProperty, instance, nullptr, exception);
#else
    return nullptr;
#endif
}

void MProperty::SetValue(MObject* instance, void* value, MObject** exception)
{
#if USE_MONO
    void* params[1];
    params[0] = value;
    mono_property_set_value(_monoProperty, instance, params, exception);
#endif
}

bool MProperty::HasAttribute(MClass* monoClass) const
{
#if USE_MONO
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, monoClass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
#else
    return false;
#endif
}

bool MProperty::HasAttribute() const
{
#if USE_MONO
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return false;

    if (attrInfo->num_attrs > 0)
    {
        mono_custom_attrs_free(attrInfo);
        return true;
    }
    mono_custom_attrs_free(attrInfo);
    return false;
#else
    return false;
#endif
}

MObject* MProperty::GetAttribute(MClass* monoClass) const
{
#if USE_MONO
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, monoClass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
#else
    return nullptr;
#endif
}

const Array<MObject*>& MProperty::GetAttributes()
{
    if (_hasCachedAttributes)
        return _attributes;

    _hasCachedAttributes = true;
#if USE_MONO
    MonoClass* parentClass = mono_property_get_parent(_monoProperty);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_property(parentClass, _monoProperty);
    if (attrInfo == nullptr)
        return _attributes;

    MonoArray* monoAttributesArray = mono_custom_attrs_construct(attrInfo);
    const auto length = (uint32)mono_array_length(monoAttributesArray);
    _attributes.Resize(length);
    for (uint32 i = 0; i < length; i++)
        _attributes[i] = mono_array_get(monoAttributesArray, MonoObject *, i);
    mono_custom_attrs_free(attrInfo);
#endif
    return _attributes;
}
