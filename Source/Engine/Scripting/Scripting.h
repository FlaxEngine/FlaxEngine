// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Types.h"

template<typename T, int32 MaxThreads>
class ThreadLocal;

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
    static Delegate<> ScriptsLoaded;

    /// <summary>
    /// Action fired on scripting engine unloading start (always main thread).
    /// </summary>
    static Delegate<> ScriptsUnload;

    /// <summary>
    /// Action fired on scripting engine reload start (always main thread).
    /// </summary>
    static Delegate<> ScriptsReloading;

    /// <summary>
    /// Action fired on scripting engine reload start (always main thread).
    /// </summary>
    static Delegate<> ScriptsReloaded;

public:
    /// <summary>
    /// Occurs on scripting update.
    /// </summary>
    static Delegate<> Update;

    /// <summary>
    /// Occurs on scripting late update.
    /// </summary>
    static Delegate<> LateUpdate;

    /// <summary>
    /// Occurs on scripting fixed update.
    /// </summary>
    static Delegate<> FixedUpdate;

    /// <summary>
    /// Occurs on scripting late fixed update.
    /// </summary>
    static Delegate<> LateFixedUpdate;

    /// <summary>
    /// Occurs on scripting draw update. Called during frame rendering and can be used to invoke custom rendering with GPUDevice.
    /// </summary>
    static Delegate<> Draw;

    /// <summary>
    /// Occurs when scripting engine is disposing. Engine is during closing and some services may be unavailable (eg. loading scenes). This may be called after the engine fatal error event.
    /// </summary>
    static Delegate<> Exit;
public:

    /// <summary>
    /// Gets the root domain.
    /// </summary>
    static MDomain* GetRootDomain();

    /// <summary>
    /// Gets the scripts domain (it can be the root domain if not using separate domain for scripting).
    /// </summary>
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
    /// Gets all registered scripting objects.
    /// </summary>
    /// <remarks>Use with caution due to potentially large memory allocation.</remarks>
    /// <returns>The collection of the objects.</returns>
    static Array<ScriptingObject*, HeapAllocation> GetObjects();

    /// <summary>
    /// Finds the class with given fully qualified name within whole assembly.
    /// </summary>
    /// <param name="fullname">The full name of the type eg: System.Int64.</param>
    /// <returns>The MClass object or null if missing.</returns>
    static MClass* FindClass(const StringAnsiView& fullname);

    /// <summary>
    /// Finds the scripting type of the given fullname by searching loaded scripting assemblies.
    /// </summary>
    /// <param name="fullname">The full name of the type eg: System.Int64.</param>
    /// <returns>The scripting type or invalid type if missing.</returns>
    static ScriptingTypeHandle FindScriptingType(const StringAnsiView& fullname);

    /// <summary>
    /// Creates a new instance of the given type object (native construction).
    /// </summary>
    /// <param name="type">The scripting object type class.</param>
    /// <returns>The created object or null if failed.</returns>
    static ScriptingObject* NewObject(const ScriptingTypeHandle& type);

    /// <summary>
    /// Creates a new instance of the given class object (native construction).
    /// </summary>
    /// <param name="type">The Managed type class.</param>
    /// <returns>The created object or null if failed.</returns>
    static ScriptingObject* NewObject(const MClass* type);

public:

    typedef Dictionary<Guid, Guid, HeapAllocation> IdsMappingTable;

    /// <summary>
    /// The objects lookup identifier mapping used to override the object ids on FindObject call (used by the object references deserialization).
    /// </summary>
    static ThreadLocal<IdsMappingTable*, PLATFORM_THREADS_LIMIT> ObjectsLookupIdMapping;

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
    /// <param name="type">The type of the object to find (optional).</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* FindObject(Guid id, const MClass* type = nullptr);

    /// <summary>
    /// Tries to find the object by the given class.
    /// </summary>
    /// <param name="type">The type of the object to find.</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* TryFindObject(const MClass* type);

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
    /// <param name="type">The type of the object to find (optional).</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* TryFindObject(Guid id, const MClass* type = nullptr);

    /// <summary>
    /// Finds the object by the given managed instance handle. Searches only registered scene objects.
    /// </summary>
    /// <param name="managedInstance">The managed instance pointer.</param>
    /// <returns>The found object or null if missing.</returns>
    static ScriptingObject* FindObject(const MObject* managedInstance);

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
    static bool IsTypeFromGameScripts(const MClass* type);

    static void ProcessBuildInfoPath(String& path, const String& projectFolderPath);

    /// <summary>
    /// Calls the given action on the next scripting update.
    /// </summary>
    /// <param name="action">The action to invoke.</param>
    static void InvokeOnUpdate(const Function<void()>& action);
private:

    static bool LoadBinaryModules(const String& path, const String& projectFolderPath);

    // Scripting Object API
    static void RegisterObject(ScriptingObject* obj);
    static void UnregisterObject(ScriptingObject* obj);
    static void OnObjectIdChanged(ScriptingObject* obj, const Guid& oldId);
};
