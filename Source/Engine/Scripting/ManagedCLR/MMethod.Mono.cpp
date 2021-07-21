// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MMethod.h"

#if USE_MONO

#include "MType.h"
#include "MClass.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <ThirdParty/mono-2.0/mono/metadata/mono-debug.h>
#include <ThirdParty/mono-2.0/mono/metadata/attrdefs.h>

MMethod::MMethod(MonoMethod* monoMethod, MClass* parentClass)
    : MMethod(monoMethod, mono_method_get_name(monoMethod), parentClass)
{
}

MMethod::MMethod(MonoMethod* monoMethod, const char* name, MClass* parentClass)
    : _monoMethod(monoMethod)
    , _parentClass(parentClass)
    , _name(name)
    , _hasCachedAttributes(false)
{
#if BUILD_DEBUG
    // Validate input name
    ASSERT(StringUtils::Compare(name, mono_method_get_name(monoMethod)) == 0);
#endif

    const uint32_t flags = mono_method_get_flags(monoMethod, nullptr);

    _isStatic = (flags & MONO_METHOD_ATTR_STATIC) != 0;
    switch (flags & MONO_METHOD_ATTR_ACCESS_MASK)
    {
    case MONO_METHOD_ATTR_PRIVATE:
        _visibility = MVisibility::Private;
        break;
    case MONO_METHOD_ATTR_FAM_AND_ASSEM:
        _visibility = MVisibility::PrivateProtected;
        break;
    case MONO_METHOD_ATTR_ASSEM:
        _visibility = MVisibility::Internal;
        break;
    case MONO_METHOD_ATTR_FAMILY:
        _visibility = MVisibility::Protected;
        break;
    case MONO_METHOD_ATTR_FAM_OR_ASSEM:
        _visibility = MVisibility::ProtectedInternal;
        break;
    case MONO_METHOD_ATTR_PUBLIC:
        _visibility = MVisibility::Public;
        break;
    default:
    CRASH;
    }

#if COMPILE_WITH_PROFILER
    const MString& className = parentClass->GetFullName();
    ProfilerName.Resize(className.Length() + 2 + _name.Length());
    Platform::MemoryCopy(ProfilerName.Get(), className.Get(), className.Length());
    ProfilerName.Get()[className.Length()] = ':';
    ProfilerName.Get()[className.Length() + 1] = ':';
    Platform::MemoryCopy(ProfilerName.Get() + className.Length() + 2, _name.Get(), _name.Length());
#endif
}

MonoObject* MMethod::Invoke(void* instance, void** params, MonoObject** exception) const
{
    PROFILE_CPU_NAMED(*ProfilerName);
    return mono_runtime_invoke(_monoMethod, instance, params, exception);
}

MonoObject* MMethod::InvokeVirtual(MonoObject* instance, void** params, MonoObject** exception) const
{
    PROFILE_CPU_NAMED(*ProfilerName);
    MonoMethod* virtualMethod = mono_object_get_virtual_method(instance, _monoMethod);
    return mono_runtime_invoke(virtualMethod, instance, params, exception);
}

#if !USE_MONO_AOT

void* MMethod::GetThunk()
{
    if (!_cachedThunk)
    {
        _cachedThunk = mono_method_get_unmanaged_thunk(_monoMethod);
    }
    return _cachedThunk;
}

#endif

MType MMethod::GetReturnType() const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    MonoType* returnType = mono_signature_get_return_type(sig);
    return MType(returnType);
}

int32 MMethod::GetParametersCount() const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    return mono_signature_get_param_count(sig);
}

MType MMethod::GetParameterType(int32 paramIdx) const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < (int32)mono_signature_get_param_count(sig));
    void* it = nullptr;
    mono_signature_get_params(sig, &it);
    return MType(((MonoType**)it)[paramIdx]);
}

bool MMethod::GetParameterIsOut(int32 paramIdx) const
{
    MonoMethodSignature* sig = mono_method_signature(_monoMethod);
    ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < (int32)mono_signature_get_param_count(sig));
    return mono_signature_param_is_out(sig, paramIdx) != 0;
}

bool MMethod::HasAttribute(MClass* monoClass) const
{
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
    if (attrInfo == nullptr)
        return false;

    const bool hasAttr = mono_custom_attrs_has_attr(attrInfo, monoClass->GetNative()) != 0;
    mono_custom_attrs_free(attrInfo);
    return hasAttr;
}

bool MMethod::HasAttribute() const
{
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
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

MonoObject* MMethod::GetAttribute(MClass* monoClass) const
{
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
    if (attrInfo == nullptr)
        return nullptr;

    MonoObject* foundAttr = mono_custom_attrs_get_attr(attrInfo, monoClass->GetNative());
    mono_custom_attrs_free(attrInfo);
    return foundAttr;
}

const Array<MonoObject*>& MMethod::GetAttributes()
{
    if (_hasCachedAttributes)
        return _attributes;

    _hasCachedAttributes = true;
    MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_method(_monoMethod);
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
