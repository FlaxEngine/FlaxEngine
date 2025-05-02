// Copyright (c) Wojciech Figat. All rights reserved.

#include "CultureInfo.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Scripting/Types.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"

#if USE_CSHARP
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#endif

// Use in-built cultures info tables from mono
#include "CultureInfo.Tables.h"

#if USE_MONO
typedef struct
{
    MonoObject obj;
    MonoBoolean is_read_only;
    gint32 lcid;
    //...
} MonoCultureInfo;
#endif

namespace
{
    const CultureInfoEntry* FindEntry(const StringAnsiView& name)
    {
        for (int32 i = 0; i < NUM_CULTURE_ENTRIES; i++)
        {
            const CultureInfoEntry& e = culture_entries[i];
            if (name == idx2string(e.name))
                return &e;
        }
        return nullptr;
    }
};

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
    if (!_data)
    {
        _lcid = 127;
        _lcidParent = 0;
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
    const CultureInfoEntry* e = FindEntry(name);
    if (!e && name.Find('-') != -1)
    {
        e = FindEntry(name.Substring(0, name.Find('-')));
    }
    if (e)
    {
        _data = (void*)e;
        _lcid = (int32)e->lcid;
        _lcidParent = (int32)e->parent_lcid;
        const char* ename = idx2string(e->name);
        _name.SetUTF8(ename, StringUtils::Length(ename));
        const char* nativename = idx2string(e->nativename);
        _nativeName.SetUTF8(nativename, StringUtils::Length(nativename));
        const char* englishname = idx2string(e->englishname);
        _englishName.SetUTF8(englishname, StringUtils::Length(englishname));
    }
    else
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
#if USE_CSHARP
    auto scriptingClass = Scripting::GetStaticClass();
    CHECK_RETURN(scriptingClass, nullptr);
    auto cultureInfoToManaged = scriptingClass->GetMethod("CultureInfoToManaged", 1);
    CHECK_RETURN(cultureInfoToManaged, nullptr);

    int32 lcid = value.GetLCID();
    void* params[1];
    params[0] = &lcid;
    MObject* obj = cultureInfoToManaged->Invoke(nullptr, params, nullptr);

    return obj;
#endif
    return nullptr;
}

CultureInfo MUtils::ToNative(void* value)
{
    int32 lcid = 127;
#if USE_MONO
    if (value)
        lcid = static_cast<MonoCultureInfo*>(value)->lcid;
#elif USE_CSHARP
    const MClass* klass = GetBinaryModuleCorlib()->Assembly->GetClass("System.Globalization.CultureInfo");
    if (value && klass)
    {
        MProperty* lcidProperty = klass->GetProperty("LCID");
        if (lcidProperty && lcidProperty->GetGetMethod())
        {
            MObject* lcidObj = lcidProperty->GetGetMethod()->Invoke(value, nullptr, nullptr);
            lcid = *(int32*)MCore::Object::Unbox(lcidObj);
        }
    }
#endif
    return CultureInfo(lcid);
}
