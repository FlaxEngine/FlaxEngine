// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MClass.h"

#if USE_MONO

#include "MType.h"
#include "MTypes.h"
#include "MField.h"
#include "MProperty.h"
#include "MMethod.h"
#include "MEvent.h"
#include "Engine/Scripting/Scripting.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>
#include <ThirdParty/mono-2.0/mono/metadata/attrdefs.h>

#define GET_CUSTOM_ATTR() (MonoCustomAttrInfo*)(_attrInfo ? _attrInfo : _attrInfo = mono_custom_attrs_from_class(_monoClass))
#include "Engine/Core/Log.h"

MClass::MClass(const MAssembly* parentAssembly, MonoClass* monoClass, const MString& fullname)
    : _monoClass(monoClass)
    , _attrInfo(nullptr)
    , _assembly(parentAssembly)
    , _fullname(fullname)
    , _visibility(MVisibility::Private)
    , _hasCachedProperties(false)
    , _hasCachedFields(false)
    , _hasCachedMethods(false)
    , _hasCachedAttributes(false)
    , _hasCachedEvents(false)
    , _isStatic(false)
    , _isSealed(false)
    , _isAbstract(false)
    , _isInterface(false)
{
    ASSERT(_monoClass);

    const uint32_t flags = mono_class_get_flags(_monoClass);

    switch (flags & MONO_TYPE_ATTR_VISIBILITY_MASK)
    {
    case MONO_TYPE_ATTR_NOT_PUBLIC:
    case MONO_TYPE_ATTR_NESTED_PRIVATE:
        _visibility = MVisibility::Private;
        break;
    case MONO_TYPE_ATTR_PUBLIC:
    case MONO_TYPE_ATTR_NESTED_PUBLIC:
        _visibility = MVisibility::Public;
        break;
    case MONO_TYPE_ATTR_NESTED_FAMILY:
    case MONO_TYPE_ATTR_NESTED_ASSEMBLY:
        _visibility = MVisibility::Internal;
        break;
    case MONO_TYPE_ATTR_NESTED_FAM_OR_ASSEM:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MONO_TYPE_ATTR_NESTED_FAM_AND_ASSEM:
        _visibility = MVisibility::PrivateProtected;
        break;
    default:
    CRASH;
    }

    const uint32_t staticClassFlags = MONO_TYPE_ATTR_ABSTRACT | MONO_TYPE_ATTR_SEALED;
    _isStatic = (flags & staticClassFlags) == staticClassFlags;
    _isSealed = !_isStatic && (flags & MONO_TYPE_ATTR_SEALED) == MONO_TYPE_ATTR_SEALED;
    _isAbstract = !_isStatic && (flags & MONO_TYPE_ATTR_ABSTRACT) == MONO_TYPE_ATTR_ABSTRACT;
    _isInterface = (flags & MONO_TYPE_ATTR_CLASS_SEMANTIC_MASK) == MONO_TYPE_ATTR_INTERFACE;
}

MClass::~MClass()
{
    if (_attrInfo)
        mono_custom_attrs_free((MonoCustomAttrInfo*)_attrInfo);

    _fields.ClearDelete();
    _properties.ClearDelete();
    _methods.ClearDelete();
    _attributes.ClearDelete();
    _events.ClearDelete();
}

String MClass::ToString() const
{
    return String(_fullname);
}

MonoClass* MClass::GetNative() const
{
    return _monoClass;
}

bool MClass::IsGeneric() const
{
    return _fullname.FindLast('`') != -1;
}

MType MClass::GetType() const
{
    return MType(mono_class_get_type(_monoClass));
}

MClass* MClass::GetBaseClass() const
{
    MonoClass* monoBase = mono_class_get_parent(_monoClass);
    if (monoBase == nullptr)
        return nullptr;

    return Scripting::FindClass(monoBase);
}

bool MClass::IsSubClassOf(const MClass* klass) const
{
    return klass && mono_class_is_subclass_of(_monoClass, klass->GetNative(), false) != 0;
}

bool MClass::IsSubClassOf(MonoClass* monoClass) const
{
    return monoClass && mono_class_is_subclass_of(_monoClass, monoClass, true) != 0;
}

bool MClass::IsInstanceOfType(MonoObject* object) const
{
    if (object == nullptr)
        return false;

    MonoClass* monoClass = mono_object_get_class(object);
    return mono_class_is_subclass_of(monoClass, _monoClass, false) != 0;
}

uint32 MClass::GetInstanceSize() const
{
    uint32 dummy = 0;

    if (mono_class_is_valuetype(_monoClass))
        return mono_class_value_size(_monoClass, &dummy);

    return mono_class_instance_size(_monoClass);
}

MMethod* MClass::FindMethod(const char* name, int32 numParams, bool checkBaseClasses)
{
    auto method = GetMethod(name, numParams);
    if (!method && checkBaseClasses)
    {
        auto base = GetBaseClass();
        if (base)
            method = base->FindMethod(name, numParams, true);
    }
    return method;
}

MMethod* MClass::GetMethod(const char* name, int32 numParams)
{
    // Lookup for cached method
    for (int32 i = 0; i < _methods.Count(); i++)
    {
        if (_methods[i]->GetName() == name && _methods[i]->GetParametersCount() == numParams)
            return _methods[i];
    }

    // Find Mono method
    MonoMethod* monoMethod = mono_class_get_method_from_name(_monoClass, name, numParams);
    if (monoMethod == nullptr)
        return nullptr;

    // Create method
    auto method = New<MMethod>(monoMethod, name, this);
    _methods.Add(method);

    return method;
}

const Array<MMethod*>& MClass::GetMethods()
{
    if (_hasCachedMethods)
        return _methods;

    void* iter = nullptr;
    MonoMethod* curClassMethod;
    while ((curClassMethod = mono_class_get_methods(_monoClass, &iter)))
    {
        // Check if has not been added
        bool isMissing = true;
        for (int32 i = 0; i < _methods.Count(); i++)
        {
            if (_methods[i]->GetNative() == curClassMethod)
            {
                isMissing = false;
                break;
            }
        }

        if (isMissing)
        {
            // Create method
            auto method = New<MMethod>(curClassMethod, this);
            _methods.Add(method);
        }
    }

    _hasCachedMethods = true;
    return _methods;
}

MField* MClass::GetField(const char* name)
{
    // Lookup for cached field
    for (int32 i = 0; i < _fields.Count(); i++)
    {
        if (_fields[i]->GetName() == name)
            return _fields[i];
    }

    // Find mono field
    MonoClassField* field = mono_class_get_field_from_name(_monoClass, name);
    if (field == nullptr)
        return nullptr;

    // Create field
    auto mfield = New<MField>(field, name, this);
    _fields.Add(mfield);
    return mfield;
}

const Array<MField*>& MClass::GetFields()
{
    if (_hasCachedFields)
        return _fields;

    void* iter = nullptr;
    MonoClassField* curClassField;
    while ((curClassField = mono_class_get_fields(_monoClass, &iter)))
    {
        const char* fieldName = mono_field_get_name(curClassField);
        GetField(fieldName);
    }

    _hasCachedFields = true;
    return _fields;
}

MEvent* MClass::GetEvent(const char* name)
{
    GetEvents();
    for (int32 i = 0; i < _events.Count(); i++)
    {
        if (_events[i]->GetName() == name)
            return _events[i];
    }
    return nullptr;
}

MEvent* MClass::AddEvent(const char* name, MonoEvent* event)
{
    // Lookup for cached event
    for (int32 i = 0; i < _events.Count(); i++)
    {
        if (_events[i]->GetName() == name)
            return _events[i];
    }

    // Create field
    auto result = New<MEvent>(event, name, this);
    _events.Add(result);

    return result;
}

const Array<MEvent*>& MClass::GetEvents()
{
    if (_hasCachedEvents)
        return _events;

    void* iter = nullptr;
    MonoEvent* curEvent;
    while ((curEvent = mono_class_get_events(_monoClass, &iter)))
    {
        const char* fieldName = mono_event_get_name(curEvent);
        AddEvent(fieldName, curEvent);
    }

    _hasCachedEvents = true;
    return _events;
}

MProperty* MClass::GetProperty(const char* name)
{
    // Lookup for cached property
    for (int32 i = 0; i < _properties.Count(); i++)
    {
        if (_properties[i]->GetName() == name)
            return _properties[i];
    }

    // Find mono property
    MonoProperty* monoProperty = mono_class_get_property_from_name(_monoClass, name);
    if (monoProperty == nullptr)
        return nullptr;

    // Create method
    auto mproperty = New<MProperty>(monoProperty, name, this);
    _properties.Add(mproperty);

    return mproperty;
}

const Array<MProperty*>& MClass::GetProperties()
{
    if (_hasCachedProperties)
        return _properties;

    void* iter = nullptr;
    MonoProperty* curClassProperty;
    while ((curClassProperty = mono_class_get_properties(_monoClass, &iter)))
    {
        const char* propertyName = mono_property_get_name(curClassProperty);
        GetProperty(propertyName);
    }

    _hasCachedProperties = true;
    return _properties;
}

MonoObject* MClass::CreateInstance() const
{
    MonoObject* obj = mono_object_new(mono_domain_get(), _monoClass);
    if (!mono_class_is_valuetype(_monoClass))
        mono_runtime_object_init(obj);
    return obj;
}

MonoObject* MClass::CreateInstance(void** params, uint32 numParams)
{
    MonoObject* obj = mono_object_new(mono_domain_get(), _monoClass);
    const auto constructor = GetMethod(".ctor", numParams);
    ASSERT(constructor);
    constructor->Invoke(obj, params, nullptr);
    return obj;
}

bool MClass::HasAttribute(MClass* monoClass)
{
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    return attrInfo != nullptr && mono_custom_attrs_has_attr(attrInfo, monoClass->GetNative()) != 0;
}

bool MClass::HasAttribute()
{
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    return attrInfo && attrInfo->num_attrs > 0;
}

MonoObject* MClass::GetAttribute(MClass* monoClass)
{
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
    return attrInfo ? mono_custom_attrs_get_attr(attrInfo, monoClass->GetNative()) : nullptr;
}

const Array<MonoObject*>& MClass::GetAttributes()
{
    if (_hasCachedAttributes)
        return _attributes;

    _hasCachedAttributes = true;
    MonoCustomAttrInfo* attrInfo = GET_CUSTOM_ATTR();
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
