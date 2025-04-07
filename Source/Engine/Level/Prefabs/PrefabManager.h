// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

class Prefab;
class Actor;
class SceneObject;
struct Transform;

// Short documentation for the prefabs system:
// Prefab - an asset in json format that contains a prefab data
// Prefab Data - serialized data of the prefab objects collection, the first object in the prefab data is a root object of the prefab
// Prefab Instance - spawned prefab objects, contain links to prefab asset and prefab objects, can be modified at runtime, can contain more than one actor (whole actors tree, including nested prefabs)
// Diff - modified properties of the prefab instance object compared to the prefab asset

/// <summary>
/// The prefab manager handles the prefabs creation, synchronization and serialization.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API PrefabManager
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(PrefabManager);

    /// <summary>
    /// Spawns the instance of the prefab objects. Prefab will be spawned to the first loaded scene.
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab);

    /// <summary>
    /// Spawns the instance of the prefab objects. Prefab will be spawned to the first loaded scene.
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="position">The spawn position in the world space.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab, const Vector3& position);

    /// <summary>
    /// Spawns the instance of the prefab objects. Prefab will be spawned to the first loaded scene.
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="position">The spawn position in the world space.</param>
    /// <param name="rotation">The spawn rotation (in world space).</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab, const Vector3& position, const Quaternion& rotation);

    /// <summary>
    /// Spawns the instance of the prefab objects. Prefab will be spawned to the first loaded scene.
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="position">The spawn position in the world space.</param>
    /// <param name="rotation">The spawn rotation (in world space).</param>
    /// <param name="scale">The spawn scale.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab, const Vector3& position, const Quaternion& rotation, const Vector3& scale);

    /// <summary>
    /// Spawns the instance of the prefab objects. Prefab will be spawned to the first loaded scene.
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="transform">The spawn transformation in the world space.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab, const Transform& transform);

    /// <summary>
    /// Spawns the instance of the prefab objects. Prefab will be spawned to the first loaded scene.
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="parent">The parent actor to add spawned object instance. Can be null to just deserialize contents of the prefab.</param>
    /// <param name="transform">The spawn transformation in the world space.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab, Actor* parent, const Transform& transform);

    /// <summary>
    /// Spawns the instance of the prefab objects. If parent actor is specified then created actors are fully initialized (OnLoad event and BeginPlay is called if parent actor is already during gameplay).
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="parent">The parent actor to add spawned object instance. Can be null to just deserialize contents of the prefab.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    API_FUNCTION() static Actor* SpawnPrefab(Prefab* prefab, Actor* parent);

    /// <summary>
    /// Spawns the instance of the prefab objects. If parent actor is specified then created actors are fully initialized (OnLoad event and BeginPlay is called if parent actor is already during gameplay).
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="parent">The parent actor to add spawned object instance. Can be null to just deserialize contents of the prefab.</param>
    /// <param name="objectsCache">The options output objects cache that can be filled with prefab object id mapping to deserialized object (actor or script).</param>
    /// <param name="withSynchronization">True if perform prefab changes synchronization for the spawned objects. It will check if need to add new objects due to nested prefab modifications.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    static Actor* SpawnPrefab(Prefab* prefab, Actor* parent, Dictionary<Guid, SceneObject*, HeapAllocation>* objectsCache, bool withSynchronization = true);

    /// <summary>
    /// Spawns the instance of the prefab objects. If parent actor is specified then created actors are fully initialized (OnLoad event and BeginPlay is called if parent actor is already during gameplay).
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="transform">prefab instance transform.</param>
    /// <param name="parent">The parent actor to add spawned object instance. Can be null to just deserialize contents of the prefab.</param>
    /// <param name="objectsCache">The options output objects cache that can be filled with prefab object id mapping to deserialized object (actor or script).</param>
    /// <param name="withSynchronization">True if perform prefab changes synchronization for the spawned objects. It will check if need to add new objects due to nested prefab modifications.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    static Actor* SpawnPrefab(Prefab* prefab, const Transform& transform, Actor* parent, Dictionary<Guid, SceneObject*, HeapAllocation>* objectsCache, bool withSynchronization = true);

    struct FLAXENGINE_API SpawnOptions
    {
        // Spawn transformation.
        const Transform* Transform = nullptr;
        // The parent actor to add spawned object instance. Can be null to just deserialize contents of the prefab.
        Actor* Parent = nullptr;
        // Custom objects mapping (maps prefab objects into spawned objects).
        const Dictionary<Guid, Guid, HeapAllocation>* IDs = nullptr;
        // Output objects cache that can be filled with prefab object id mapping to deserialized object (actor or script).
        Dictionary<Guid, SceneObject*, HeapAllocation>* ObjectsCache = nullptr;
        //  if perform prefab changes synchronization for the spawned objects. It will check if need to add new objects due to nested prefab modifications.
        bool WithSync = true;
        // True if linked spawned prefab objects with the source prefab, otherwise links will be valid only for nested prefab objects.
        bool WithLink = true;
    };

    /// <summary>
    /// Spawns the instance of the prefab objects. If parent actor is specified then created actors are fully initialized (OnLoad event and BeginPlay is called if parent actor is already during gameplay).
    /// </summary>
    /// <param name="prefab">The prefab asset.</param>
    /// <param name="options">The spawn options container.</param>
    /// <returns>The created actor (root) or null if failed.</returns>
    static Actor* SpawnPrefab(Prefab* prefab, const SpawnOptions& options);

#if USE_EDITOR

    /// <summary>
    /// Helper global state flag set to true during prefab asset creation. Can be used to skip local objects data serialization to prefab data.
    /// </summary>
    static bool IsCreatingPrefab;

    /// <summary>
    /// Creates the prefab asset from the given root actor. Saves it to the output file path.
    /// </summary>
    /// <param name="targetActor">The target actor (prefab root). It cannot be a scene but it can contain a scripts and/or full hierarchy of objects to save.</param>
    /// <param name="outputPath">The output asset path.</param>
    /// <param name="autoLink">True if auto-connect the target actor and related objects to the created prefab.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool CreatePrefab(Actor* targetActor, const StringView& outputPath, bool autoLink);

    /// <summary>
    /// Filters the list of the prefab instances to serialize to prefab data. Removes the objects disabled for saving and validates any recursive prefab links.
    /// </summary>
    /// <param name="prefabId">The ID of the prefab to serialize. Can be invalid to disable prefab links recursion checks.</param>
    /// <param name="objects">The objects collection.</param>
    /// <param name="showWarning">True if show warnings to the used on invalid data usage, otherwise false.</param>
    /// <returns>True if data is invalid and cannot be used for a prefab, otherwise false.</returns>
    static bool FilterPrefabInstancesToSave(const Guid& prefabId, Array<SceneObject*, HeapAllocation>& objects, bool showWarning = true);

    /// <summary>
    /// The prefabs references mapping table. Maps the prefab asset id to the collection of the root actors of the prefab instances.
    /// </summary>
    static Dictionary<Guid, Array<Actor*, HeapAllocation>, HeapAllocation> PrefabsReferences;

    /// <summary>
    /// Locks PrefabsReferences to be used in a multi threaded environment.
    /// </summary>
    static CriticalSection PrefabsReferencesLocker;

    /// <summary>
    /// Applies the difference from the prefab object instance, saves the changes and synchronizes them with the active instances of the prefab asset.
    /// </summary>
    /// <remarks>
    /// Applies all the changes from not only the given actor instance but all actors created within that prefab instance.
    /// </remarks>
    /// <param name="instance">The modified instance.</param>
    /// <returns>True if data is failed to apply the changes, otherwise false.</returns>
    API_FUNCTION() static bool ApplyAll(Actor* instance);

#endif
};
