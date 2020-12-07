// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Threading/ThreadLocal.h"
#include "ScriptingObject.h"

class BinaryModule;

/// <summary>
/// Embedded managed scripting runtime service.
/// </summary>
class FLAXENGINE_API Scripting
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Scripting);
    friend ScriptingObject;
    friend BinaryModule;
public:

    /// <summary>
    /// Action fired when scripting loads a binary module (eg. with game scripts).
    /// </summary>
    static Delegate<BinaryModule*> BinaryModuleLoaded;

    /// <summary>
    /// Action fired on scripting engine loaded (always main thread).
    /// </summary>
    static Action ScriptsLoaded;

    /// <summary>
    /// Action fired on scripting engine unloading start (always main thread).
    /// </summary>
    static Action ScriptsUnload;

    /// <summary>
    /// Action fired on scripting engine reload start (always main thread).
    /// </summary>
    static Action ScriptsReloading;

    /// <summary>
    /// Action fired on scripting engine reload start (always main thread).
    /// </summary>
    static Action ScriptsReloaded;

public:

    /// <summary>
    /// Gets mono root domain
    /// </summary>
    /// <returns>The Mono root domain.</returns>
    static MDomain* GetRootDomain();

    /// <summary>
    /// Gets mono scripts domain
    /// </summary>
    /// <returns>The Mono domain.</returns>
    static MonoDomain* GetMonoScriptsDomain();

    /// <summary>
    /// Gets scripts domain
    /// </summary>
    /// <returns>The domain.</returns>
    static MDomain* GetScriptsDomain();

public:

    /// <summary>
    /// Load/Reload scripts now
    /// </summary>
    /// <returns>True if failed or cannot be done, otherwise false</returns>
    static bool Load();

    /// <summary>
    /// Release scripting layer (will destroy internal scripts data)
    /// </summary>
    static void Release();

#if USE_EDITOR

    /// <summary>
    /// Reloads scripts.
    /// </summary>
    /// <param name="canTriggerSceneReload">True if allow to scene scripts reload callback, otherwise it won't be possible.</param>
    static void Reload(bool canTriggerSceneReload = true);

#endif

public:

    /// <summary>
    /// Finds the class from the given Mono class object within whole assembly.
    /// </summary>
    /// <param name="monoClass">The Mono class.</param>
    /// <returns>The MClass object or null if missing.</returns>
    static MClass* FindClass(MonoClass* monoClass);

    /// <summary>
    /// Finds the class with given fully qualified name within whole assembly.
    /// </summary>
    /// <param name="fullname">The full name of the type eg: System.Int64.MaxInt.</param>
    /// <returns>The MClass object or null if missing.</returns>
    static MClass* FindClass(const StringAnsiView& fullname);

    /// <summary>
    /// Finds the native class with given fully qualified name within whole assembly.
    /// </summary>
    /// <param name="fullname">The full name of the type eg: System.Int64.MaxInt.</param>
    /// <returns>The MClass object or null if missing.</returns>
    static MonoClass* FindClassNative(const StringAnsiView& fullname);

    /// <summary>
    /// Finds the scripting type of the given fullname by searching loaded scripting assemblies.
    /// </summary>
    /// <param name="fullname">The full name of the type eg: System.Int64.MaxInt.</param>
    /// <returns>The scripting type or invalid type if missing.</returns>
    static ScriptingTypeHandle FindScriptingType(const StringAnsiView& fullname);

public:

    typedef Dictionary<Guid, Guid> IdsMappingTable;

    /// <summary>
    /// The objects lookup identifier mapping used to override the object ids on FindObject call (used by the object references deserialization).
    /// </summary>
    static ThreadLocal<IdsMappingTable*, 32> ObjectsLookupIdMapping;

    /// <summary>
    /// Finds the object by the given identifier. Searches registered scene objects and optionally assets. Logs warning if fails.
    /// </summary>
    /// <param name="id">The object unique identifier.</param>
    /// <returns>The found object or null if missing.</returns>
    template<typename T>
    FORCE_INLINE static T* FindObject(const Guid& id)
    {
        return (T*)FindObject(id, T::GetStaticClass());
    }

    /// <summary>
    /// Finds the object by the given identifier. Searches registered scene objects and optionally assets. Logs warning if fails.
    /// </summary>
    /// <param name="id">The object unique identifier.</param>
    /// <param name="type">The type of the object to find.</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* FindObject(Guid id, MClass* type);

    /// <summary>
    /// Tries to find the object by the given identifier.
    /// </summary>
    /// <param name="id">The object unique identifier.</param>
    /// <returns>The found object or null if missing.</returns>
    template<typename T>
    FORCE_INLINE static T* TryFindObject(const Guid& id)
    {
        return (T*)TryFindObject(id, T::GetStaticClass());
    }

    /// <summary>
    /// Tries to find the object by the given identifier.
    /// </summary>
    /// <param name="id">The object unique identifier.</param>
    /// <param name="type">The type of the object to find.</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* TryFindObject(Guid id, MClass* type);

    /// <summary>
    /// Finds the object by the given managed instance handle. Searches only registered scene objects.
    /// </summary>
    /// <param name="managedInstance">The managed instance pointer.</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* FindObject(const MonoObject* managedInstance);

    /// <summary>
    /// Event called by the internal call on a finalizer thread when the managed objects gets deleted by the GC.
    /// </summary>
    /// <param name="obj">The unmanaged object pointer that was related to the managed object.</param>
    static void OnManagedInstanceDeleted(ScriptingObject* obj);

public:

    /// <summary>
    /// Returns true if game modules are loaded.
    /// </summary>
    static bool HasGameModulesLoaded();

    /// <summary>
    /// Returns true if every assembly is loaded.
    /// </summary>
    static bool IsEveryAssemblyLoaded();

    /// <summary>
    /// Returns true if given type is from one of the game scripts assemblies.
    /// </summary>
    static bool IsTypeFromGameScripts(MClass* type);

    static void ProcessBuildInfoPath(String& path, const String& projectFolderPath);

private:

    static bool LoadBinaryModules(const String& path, const String& projectFolderPath);

    // Scripting Object API
    static void RegisterObject(ScriptingObject* obj);
    static void UnregisterObject(ScriptingObject* obj);
    static void OnObjectIdChanged(ScriptingObject* obj, const Guid& oldId);
};
