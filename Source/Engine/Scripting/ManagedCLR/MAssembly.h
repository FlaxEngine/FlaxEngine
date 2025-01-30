// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// Represents a managed assembly, which is a reusable, versionable, and self-describing building block of a common language runtime application.
/// </summary>
class FLAXENGINE_API MAssembly
{
    friend MCore;
    friend MDomain;
    friend Scripting;

public:
    typedef Dictionary<StringAnsi, MClass*> ClassesDictionary;

private:
#if USE_MONO
    MonoAssembly* _monoAssembly = nullptr;
    MonoImage* _monoImage = nullptr;
#elif USE_NETCORE
    void* _handle = nullptr;
    StringAnsi _fullname;
#endif
    MDomain* _domain;

    int32 _isLoaded : 1;
    int32 _isLoading : 1;
    mutable int32 _hasCachedClasses : 1;

    mutable ClassesDictionary _classes;

    int32 _reloadCount;
    StringAnsi _name;
    String _assemblyPath;

    Array<byte> _debugData;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="MAssembly"/> class.
    /// </summary>
    /// <param name="domain">The assembly domain.</param>
    /// <param name="name">The assembly name.</param>
    MAssembly(MDomain* domain, const StringAnsiView& name);

#if USE_NETCORE
    /// <summary>
    /// Initializes a new instance of the <see cref="MAssembly"/> class.
    /// </summary>
    /// <param name="domain">The assembly domain.</param>
    /// <param name="name">The assembly name.</param>
    /// <param name="fullname">The assembly full name.</param>
    /// <param name="handle">The managed handle of the assembly.</param>
    MAssembly(MDomain* domain, const StringAnsiView& name, const StringAnsiView& fullname, void* handle);
#endif

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
    FORCE_INLINE bool IsLoading() const
    {
        return _isLoading != 0;
    }

    /// <summary>
    /// Returns true if assembly has been loaded.
    /// </summary>
    FORCE_INLINE bool IsLoaded() const
    {
        return _isLoaded != 0;
    }

    /// <summary>
    /// Gets the assembly name.
    /// </summary>
    FORCE_INLINE const StringAnsi& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Gets the assembly name as string.
    /// </summary>
    String ToString() const;

    /// <summary>
    /// Gets the assembly path.
    /// </summary>
    /// <remarks>
    /// If assembly was made from scratch (empty), path will return null.
    /// </remarks>
    FORCE_INLINE const String& GetAssemblyPath() const
    {
        return _assemblyPath;
    }

    /// <summary>
    /// Gets the parent domain.
    /// </summary>
    FORCE_INLINE MDomain* GetDomain() const
    {
        return _domain;
    }

#if USE_MONO
    FORCE_INLINE MonoAssembly* GetMonoAssembly() const
    {
        return _monoAssembly;
    }

    FORCE_INLINE MonoImage* GetMonoImage() const
    {
        return _monoImage;
    }
#elif USE_NETCORE
    FORCE_INLINE void* GetHandle() const
    {
        return _handle;
    }
#endif

public:
    /// <summary>
    /// Loads assembly for domain.
    /// </summary>
    /// <param name="assemblyPath">The assembly path.</param>
    /// <param name="nativePath">The optional path to the native code assembly (eg. if C# assembly contains bindings).</param>
    /// <returns>True if cannot load, otherwise false</returns>
    bool Load(const String& assemblyPath, const StringView& nativePath = StringView::Empty);

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
    MonoReflectionAssembly* GetNative() const;
#endif

    /// <summary>
    /// Gets the classes lookup cache. Performs full initialization if not cached. The result cache contains all classes from the assembly.
    /// </summary>
    const ClassesDictionary& GetClasses() const;

private:
    bool LoadCorlib();
    bool LoadImage(const String& assemblyPath, const StringView& nativePath);
    bool UnloadImage(bool isReloading);
    void OnLoading();
    void OnLoaded(struct Stopwatch& stopwatch);
    void OnLoadFailed();
    bool ResolveMissingFile(String& assemblyPath) const;
};
