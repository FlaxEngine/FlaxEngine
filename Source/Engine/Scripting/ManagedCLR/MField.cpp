// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "MField.h"
#include "MType.h"
#include "MClass.h"
#if USE_MONO
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/attrdefs.h>

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

#endif

MType MField::GetType() const
{
#if USE_MONO
    return MType(_monoType);
#else
    return MType();
#endif
}

int32 MField::GetOffset() const
{
#if USE_MONO
    return mono_field_get_offset(_monoField) - sizeof(MonoObject);
#else
    return 0;
#endif
}

void MField::GetValue(MObject* instance, void* result) const
{
#if USE_MONO
    mono_field_get_value(instance, _monoField, result);
#endif
}

MObject* MField::GetValueBoxed(MObject* instance) const
{
#if USE_MONO
    return mono_field_get_value_object(mono_domain_get(), _monoField, instance);
#else
    return nullptr;
#endif
}

void MField::SetValue(MObject* instance, void* value) const
{
#if USE_MONO
    mono_field_set_value(instance, _monoField, value);
#endif
}

bool MField::HasAttribute(MClass* monoClass) const
{
#if USE_MONO
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, monoClass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
#else
    return false;
#endif
}

bool MField::HasAttribute() const
{
#if USE_MONO
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
#endif
    return false;
}

MObject* MField::GetAttribute(MClass* monoClass) const
{
#if USE_MONO
    MonoClass* parentClass = mono_field_get_parent(_monoField);
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _monoField);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, monoClass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
#else
    return nullptr;
#endif
}

const Array<MObject*>& MField::GetAttributes()
{
    if (_hasCachedAttributes)
        return _attributes;

    _hasCachedAttributes = true;
#if USE_MONO
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
#endif
    return _attributes;
}
