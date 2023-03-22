// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "MDomain.h"
#include "MCore.h"
#include "MAssembly.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Core/Log.h"
#include "Engine/Debug/Exceptions/ArgumentException.h"
#include "Engine/Debug/Exceptions/Exceptions.h"
#if USE_MONO
#include <mono/metadata/threads.h>
#endif

extern MDomain* MActiveDomain;

MDomain::MDomain(const MString& domainName)
    : _domainName(domainName)
{
}

bool MDomain::SetCurrentDomain(bool force)
{
#if USE_MONO
    if (mono_domain_set(_monoDomain, force) == 0)
        return false;
#endif
    MActiveDomain = this;
    return true;
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
#if USE_MONO
    if (!IsInMainThread())
    {
        mono_thread_attach(_monoDomain);
    }
#endif
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
