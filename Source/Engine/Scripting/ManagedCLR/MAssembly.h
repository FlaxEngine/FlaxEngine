// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"
#include "MAssemblyOptions.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// Represents a managed assembly, which is a reusable, versionable, and self-describing building block of a common language runtime application.
/// </summary>
class FLAXENGINE_API MAssembly
{
    friend MDomain;

public:

    typedef Dictionary<MString, MClass*> ClassesDictionary;

private:

#if USE_MONO
    MonoAssembly* _monoAssembly;
    MonoImage* _monoImage;
#endif
    MDomain* _domain;

    int32 _isLoaded : 1;
    int32 _isLoading : 1;
    int32 _isDependency : 1;
    int32 _isFileLocked : 1;
    mutable int32 _hasCachedClasses : 1;

    mutable ClassesDictionary _classes;
    CriticalSection _locker;

    int32 _reloadCount;
    MString _name;
    String _assemblyPath;

    Array<byte> _debugData;

    const MAssemblyOptions _options;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MAssembly"/> class.
    /// </summary>
    /// <param name="domain">The assembly domain.</param>
    /// <param name="name">The assembly name.</param>
    /// <param name="options">The assembly options.</param>
    MAssembly(MDomain* domain, const StringAnsiView& name, const MAssemblyOptions& options);

    /// <summary>
    /// Finalizes an instance of the <see cref="MAssembly"/> class.
    /// </summary>
    ~MAssembly();

public:

    /// <summary>
    /// Managed assembly actions delegate type.
    /// </summary>
    typedef Delegate<MAssembly*> AssemblyDelegate;

    /// <summary>
    /// Action fired when assembly starts loading.
    /// </summary>
    AssemblyDelegate Loading;

    /// <summary>
    /// Action fired when assembly gets loaded.
    /// </summary>
    AssemblyDelegate Loaded;

    /// <summary>
    /// Action fired when assembly loading fails.
    /// </summary>
    AssemblyDelegate LoadFailed;

    /// <summary>
    /// Action fired when assembly start unloading.
    /// </summary>
    AssemblyDelegate Unloading;

    /// <summary>
    /// Action fired when assembly gets unloaded.
    /// </summary>
    AssemblyDelegate Unloaded;

public:

    /// <summary>
    /// Returns true if assembly is during loading state.
    /// </summary>
    /// <returns>True if is loading, otherwise false.</returns>
    FORCE_INLINE bool IsLoading() const
    {
        return _isLoading != 0;
    }

    /// <summary>
    /// Returns true if assembly has been loaded.
    /// </summary>
    /// <returns>True if is loaded, otherwise false.</returns>
    FORCE_INLINE bool IsLoaded() const
    {
        return _isLoaded != 0;
    }

    /// <summary>
    /// Gets the assembly name.
    /// </summary>
    /// <returns>The assembly name.</returns>
    FORCE_INLINE const MString& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Gets the assembly name as string.
    /// </summary>
    /// <returns>The assembly name.</returns>
    String ToString() const;

    /// <summary>
    /// Gets the assembly path.
    /// </summary>
    /// <remarks>
    /// If assembly was made from scratch (empty), path will return null.
    /// </remarks>
    /// <returns>The assembly path.</returns>
    FORCE_INLINE const String& GetAssemblyPath() const
    {
        return _assemblyPath;
    }

    /// <summary>
    /// Gets the parent domain.
    /// </summary>
    /// <returns>The domain object.</returns>
    FORCE_INLINE MDomain* GetDomain() const
    {
        return _domain;
    }

public:

#if USE_MONO

    /// <summary>
    /// Gets the Mono assembly.
    /// </summary>
    /// <returns>The Mono assembly.</returns>
    FORCE_INLINE MonoAssembly* GetMonoAssembly() const
    {
        return _monoAssembly;
    }

    /// <summary>
    /// Gets the Mono image.
    /// </summary>
    /// <returns>The Mono image.</returns>
    FORCE_INLINE MonoImage* GetMonoImage() const
    {
        return _monoImage;
    }

#endif

public:

    /// <summary>
    /// Gets the options that assembly was created with.
    /// </summary>
    /// <returns>The options.</returns>
    FORCE_INLINE const MAssemblyOptions& GetOptions() const
    {
        return _options;
    }

    /// <summary>
    /// Loads assembly for domain.
    /// </summary>
    /// <param name="assemblyPath">The assembly path.</param>
    /// <returns>True if cannot load, otherwise false</returns>
    bool Load(const String& assemblyPath);

#if USE_MONO

    /// <summary>
    /// Loads assembly for domain.
    /// </summary>
    /// <param name="monoImage">The assembly image.</param>
    /// <returns>True if cannot load, otherwise false.</returns>
    bool Load(MonoImage* monoImage);

#endif

    /// <summary>
    /// Cleanup data. Caller must ensure not to use any types from this assembly after it has been unloaded.
    /// </summary>
    /// <param name="isReloading">If true assembly is during reloading and should force release the runtime data.</param>
    void Unload(bool isReloading = false);

public:

    /// <summary>
    /// Attempts to find a managed class with the specified namespace and name in this assembly. Returns null if one cannot be found.
    /// </summary>
    /// <param name="typeName">The type name.</param>
    /// <returns>The class object or null if failed to find it.</returns>
    MClass* GetClass(const StringAnsiView& typeName) const;

#if USE_MONO

    /// <summary>
    /// Converts an internal mono representation of a class into engine class.
    /// </summary>
    /// <param name="monoClass">The Mono class.</param>
    /// <returns>The class object.</returns>
    MClass* GetClass(MonoClass* monoClass) const;

    /// <summary>
    /// Gets the native of the assembly (for the current domain). Can be used to pass to the scripting backend as a parameter.
    /// </summary>
    /// <returns>The native assembly object.</returns>
    MonoReflectionAssembly* GetNative() const;

#endif

    /// <summary>
    /// Gets the classes lookup cache. Performs full initialization if not cached. The result cache contains all classes from the assembly.
    /// </summary>
    /// <returns>The cache.</returns>
    const ClassesDictionary& GetClasses() const;

private:

#if USE_MONO

    /// <summary>
    /// Loads the assembly for domain.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadDefault(const String& assemblyPath);

    /// <summary>
    /// Loads the assembly for domain from non-blocking image.
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadWithImage(const String& assemblyPath);

#endif

    void OnLoading();
    void OnLoaded(const struct DateTime& startTime);
    void OnLoadFailed();
};
