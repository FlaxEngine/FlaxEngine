// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "CultureInfo.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/ManagedCLR/MType.h"
#include "Engine/Utilities/StringConverter.h"
#if USE_MONO
#include <mono/metadata/appdomain.h>
#include <mono/metadata/culture-info.h>
#include <mono/metadata/culture-info-tables.h>

typedef struct
{
    MonoObject obj;
    MonoBoolean is_read_only;
    gint32 lcid;
    //...
} MonoCultureInfo;
#endif

CultureInfo::CultureInfo(int32 lcid)
{
    _lcid = lcid;
    _lcidParent = 0;
    _data = nullptr;
    if (lcid == 0)
        return;
    if (lcid == 127)
    {
        _englishName = TEXT("Invariant Culture");
        return;
    }
#if USE_MONO
    for (int32 i = 0; i < NUM_CULTURE_ENTRIES; i++)
    {
        auto& e = culture_entries[i];
        if (e.lcid == lcid)
        {
            _data = (void*)&e;
            _lcidParent = (int32)e.parent_lcid;
            const char* name = idx2string(e.name);
            _name.SetUTF8(name, StringUtils::Length(name));
            const char* nativename = idx2string(e.nativename);
            _nativeName.SetUTF8(nativename, StringUtils::Length(nativename));
            const char* englishname = idx2string(e.englishname);
            _englishName.SetUTF8(englishname, StringUtils::Length(englishname));
            break;
        }
    }
#endif
    if (!_data)
    {
        _englishName = TEXT("Invariant Culture");
        LOG(Error, "Unknown LCID {0} for CultureInfo", lcid);
    }
}

CultureInfo::CultureInfo(const StringView& name)
    : CultureInfo(StringAnsiView(StringAsANSI<16>(name.Get(), name.Length()).Get(), name.Length()))
{
}

CultureInfo::CultureInfo(const StringAnsiView& name)
{
    _data = nullptr;
    if (name.IsEmpty())
    {
        _lcid = 127;
        _lcidParent = 0;
        _englishName = TEXT("Invariant Culture");
        return;
    }
#if USE_MONO
    for (int32 i = 0; i < NUM_CULTURE_ENTRIES; i++)
    {
        auto& e = culture_entries[i];
        if (name == idx2string(e.name))
        {
            _data = (void*)&e;
            _lcid = (int32)e.lcid;
            _lcidParent = (int32)e.parent_lcid;
            _name.SetUTF8(name.Get(), name.Length());
            const char* nativename = idx2string(e.nativename);
            _nativeName.SetUTF8(nativename, StringUtils::Length(nativename));
            const char* englishname = idx2string(e.englishname);
            _englishName.SetUTF8(englishname, StringUtils::Length(englishname));
            break;
        }
    }
#endif
    if (!_data)
    {
        _lcid = 127;
        _lcidParent = 0;
        _englishName = TEXT("Invariant Culture");
        LOG(Error, "Unknown name {0} for CultureInfo", String(name));
    }
}

int32 CultureInfo::GetLCID() const
{
    return _lcid;
}

int32 CultureInfo::GetParentLCID() const
{
    return _lcidParent;
}

const String& CultureInfo::GetName() const
{
    return _name;
}

const String& CultureInfo::GetNativeName() const
{
    return _nativeName;
}

const String& CultureInfo::GetEnglishName() const
{
    return _englishName;
}

bool CultureInfo::IsRightToLeft() const
{
#if USE_MONO
    const auto data = static_cast<CultureInfoEntry*>(_data);
    if (data)
        return data->text_info.is_right_to_left ? true : false;
#endif
    return false;
}

String CultureInfo::ToString() const
{
    return _name;
}

bool CultureInfo::operator==(const CultureInfo& other) const
{
    return _lcid == other._lcid;
}

void* MUtils::ToManaged(const CultureInfo& value)
{
#if USE_MONO
    const auto klass = mono_class_from_name(mono_get_corlib(), "System.Globalization", "CultureInfo");
    if (klass)
    {
        void* iter = nullptr;
        MonoMethod* method;
        while ((method = mono_class_get_methods(klass, &iter)))
        {
            if (StringUtils::Compare(mono_method_get_name(method), ".ctor") == 0)
            {
                const auto sig = mono_method_signature(method);
                void* sigParams = nullptr;
                mono_signature_get_params(sig, &sigParams);
                if (mono_signature_get_param_count(sig) == 1 && mono_type_get_class(((MonoType**)sigParams)[0]) != mono_get_string_class())
                {
                    MonoObject* obj = mono_object_new(mono_domain_get(), klass);
                    int32 lcid = value.GetLCID();
                    void* params[1];
                    params[0] = &lcid;
                    mono_runtime_invoke(method, obj, params, nullptr);
                    return obj;
                }
            }
        }
    }
#endif
    return nullptr;
}

CultureInfo MUtils::ToNative(void* value)
{
    int32 lcid = 127;
#if USE_MONO
    if (value)
        lcid = static_cast<MonoCultureInfo*>(value)->lcid;
#endif
    return CultureInfo(lcid);
}
