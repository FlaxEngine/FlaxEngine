// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MField.h"

#if USE_MONO

#include "MType.h"
#include "MClass.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>
#include <ThirdParty/mono-2.0/mono/metadata/attrdefs.h>

MField::MField(MonoClassField* monoField, const char* name, MClass* parentClass)
    : _monoField(monoField)
    , _monoType(mono_field_get_type(monoField))
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_field_get_name(monoField)) == 0);
#endif

    const uint32_t flags = mono_field_get_flags(monoField);
    switch (flags & MONO_FIELD_ATTR_FIELD_ACCESS_MASK)
    {
        case MONO_FIELD_ATTR_PRIVATE:
            _visibility = MVisibility::Private;
            break;
        case MONO_FIELD_ATTR_FAM_AND_ASSEM:
            _visibility = MVisibility::PrivateProtected;
            break;
        case MONO_FIELD_ATTR_ASSEMBLY:
            _visibility = MVisibility::Internal;
            break;
        case MONO_FIELD_ATTR_FAMILY:
            _visibility = MVisibility::Protected;
            break;
        case MONO_FIELD_ATTR_FAM_OR_ASSEM:
            _visibility = MVisibility::ProtectedInternal;
            break;
        case MONO_FIELD_ATTR_PUBLIC:
            _visibility = MVisibility::Public;
            break;
        default:
            CRASH;
    }
    _isStatic = (flags & MONO_FIELD_ATTR_STATIC) != 0;
}

MType MField::GetType() const
{
    return MType(_monoType);
}

int32 MField::GetOffset() const
{
    return mono_field_get_offset(_monoField) - sizeof(MonoObject);
}

void MField::GetValue(MonoObject* instance, void* result) const
{
    mono_field_get_value(instance, _monoField, result);
}

MonoObject* MField::GetValueBoxed(MonoObject* instance) const
{
    return mono_field_get_value_object(mono_domain_get(), _monoField, instance);
}

void MField::SetValue(MonoObject* instance, void* value) const
{
    mono_field_set_value(instance, _monoField, value);
}

bool MField::HasAttribute(MClass* monoClass) const
{
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, monoClass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MField::HasAttribute() const
{
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
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

MonoObject* MField::GetAttribute(MClass* monoClass) const
{
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, monoClass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MonoObject*>& MField::GetAttributes()
{
    if (_hasCachedAttributes)
        return _attributes;

    _hasCachedAttributes = true;
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
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
