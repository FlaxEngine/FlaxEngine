// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Dictionary.h"
#include "MTypes.h"
#include "MAssemblyOptions.h"

/// <summary>
/// Domain separates multiple processes within one executed CLR environment.
/// </summary>
/// <remarks>
/// At once you can execute methods, get instances etc, only from on Domain at the time.
/// If you want to execute any code that given domain contains, you need to switch context, and dispatch current thread to CLR environment.
/// </remarks>
class FLAXENGINE_API MDomain
{
    friend MCore;
    friend MAssembly;
public:

    typedef Dictionary<MString, MAssembly*> AssembliesDictionary;

private:

#if USE_MONO
    MonoDomain* _monoDomain;
#endif
    MString _domainName;
    AssembliesDictionary _assemblies;

public:

    MDomain(const MString& domainName);

public:

#if USE_MONO
    /// <summary>
    /// Gets native domain class.
    /// </summary>
    FORCE_INLINE MonoDomain* GetNative() const
    {
        return _monoDomain;
    }
#endif

    /// <summary>
    /// Gets current domain name
    /// </summary>
    FORCE_INLINE const MString& GetName() const
    {
        return _domainName;
    }

    /// <summary>
    /// Gets the current domain assemblies.
    /// </summary>
    FORCE_INLINE const AssembliesDictionary& GetAssemblies() const
    {
        return _assemblies;
    }

public:

    /// <summary>
    /// Create assembly container from current domain
    /// </summary>
    /// <param name="assemblyName">Assembly name to later receive from assemblies dictionary</param>
    /// <param name="options">The assembly options container.</param>
    /// <returns>MAssembly object ready to Load</returns>
    MAssembly* CreateEmptyAssembly(const MString& assemblyName, const MAssemblyOptions options);

    /// <summary>
    /// Removes assembly from current domain and request unloading.
    /// </summary>
    /// <param name="assemblyName">Assembly name</param>
    void RemoveAssembly(const MString& assemblyName);

    /// <summary>
    /// Gets current domain assembly.
    /// </summary>
    /// <returns>The managed assembly or null if not found.</returns>
    MAssembly* GetAssembly(const MString& assemblyName) const;

    /// <summary>
    /// Attaches current CLR domain calls to the current thread.
    /// </summary>
    void Dispatch() const;

    /// <summary>
    /// Sets currently using domain.
    /// </summary>
    /// <returns>True if succeed in settings, false if failed.</returns>
    bool SetCurrentDomain(bool force = false);

    /// <summary>
    /// Returns class from current domain.
    /// </summary>
    MClass* FindClass(const StringAnsiView& fullname) const;
};
