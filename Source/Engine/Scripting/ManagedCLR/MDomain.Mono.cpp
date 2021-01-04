// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MDomain.h"

#if USE_MONO

#include "MCore.h"
#include "MAssembly.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Core/Log.h"
#include "Engine/Debug/Exceptions/ArgumentException.h"
#include "Engine/Debug/Exceptions/Exceptions.h"
#include <ThirdParty/mono-2.0/mono/metadata/threads.h>

MDomain::MDomain(const MString& domainName, MonoDomain* monoDomain)
    : _monoDomain(monoDomain)
    , _domainName(domainName)
    , _coreInstance(MCore::Instance())
{
}

MonoDomain* MDomain::GetNative() const
{
    return _monoDomain;
}

bool MDomain::SetCurrentDomain(bool force)
{
    ASSERT(_monoDomain);
    const auto monoBool = mono_domain_set(_monoDomain, force) != 0;
    if (monoBool)
    {
        _coreInstance->_activeDomain = this;
    }
    return monoBool;
}

MAssembly* MDomain::CreateEmptyAssembly(const MString& assemblyName, const MAssemblyOptions options)
{
    // Validates if assembly is already open
    if (_assemblies.ContainsKey(assemblyName))
    {
        Log::ArgumentException(String::Format(TEXT("{0} assembly has already been added"), String(assemblyName)));
        return _assemblies.At(assemblyName);
    }

    // Create shared pointer to the assembly 
    const auto assembly = New<MAssembly>(this, assemblyName, options);

    // Add assembly instance to dictionary
    _assemblies.Add(assemblyName, assembly);

    return assembly;
}

void MDomain::RemoveAssembly(const MString& assemblyName)
{
    auto assembly = _assemblies[assemblyName];
    _assemblies.Remove(assemblyName);
    Delete(assembly);
}

MAssembly* MDomain::GetAssembly(const MString& assemblyName) const
{
    MAssembly* result = nullptr;
    if (!_assemblies.TryGet(assemblyName, result))
    {
        Log::ArgumentOutOfRangeException(TEXT("Current assembly was not found in given domain"));
    }
    return result;
}

void MDomain::Dispatch() const
{
    if (!IsInMainThread())
    {
        mono_thread_attach(_monoDomain);
    }
}

MClass* MDomain::FindClass(const StringAnsiView& fullname) const
{
    for (auto assembly = _assemblies.Begin(); assembly != _assemblies.End(); ++assembly)
    {
        const auto currentClass = assembly->Value->GetClass(fullname);
        if (!currentClass)
        {
            return currentClass;
        }
    }
    return nullptr;
}

#endif
