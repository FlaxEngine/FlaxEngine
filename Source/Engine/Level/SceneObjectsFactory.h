// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "SceneObject.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Threading/ThreadLocal.h"

/// <summary>
/// Helper class for scene objects creation and deserialization utilities.
/// </summary>
class FLAXENGINE_API SceneObjectsFactory
{
public:
    struct PrefabInstance
    {
        int32 StatIndex;
        int32 RootIndex;
        Guid RootId;
        Prefab* Prefab;
        bool FixRootParent = false;
        Dictionary<Guid, Guid> IdsMapping;
    };

    struct Context
    {
        ISerializeModifier* Modifier;
        bool Async = false;
        Array<PrefabInstance> Instances;
        Dictionary<Guid, int32> ObjectToInstance;
        CriticalSection Locker;
        ThreadLocal<ISerializeModifier*> Modifiers;
#if USE_EDITOR
        HashSet<Prefab*> DeprecatedPrefabs;
#endif

        Context(ISerializeModifier* modifier);
        ~Context();

        ISerializeModifier* GetModifier();
        void SetupIdsMapping(const SceneObject* obj, ISerializeModifier* modifier) const;
    };

    /// <summary>
    /// Creates the scene object from the specified data value. Does not perform deserialization.
    /// </summary>
    /// <param name="context">The serialization context.</param>
    /// <param name="stream">The serialized data stream.</param>
    static SceneObject* Spawn(Context& context, const ISerializable::DeserializeStream& stream);

    /// <summary>
    /// Deserializes the scene object from the specified data value.
    /// </summary>
    /// <param name="context">The serialization context.</param>
    /// <param name="obj">The instance to deserialize.</param>
    /// <param name="stream">The serialized data stream.</param>
    static void Deserialize(Context& context, SceneObject* obj, ISerializable::DeserializeStream& stream);

    /// <summary>
    /// Handles the object deserialization error.
    /// </summary>
    /// <param name="value">The value.</param>
    static void HandleObjectDeserializationError(const ISerializable::DeserializeStream& value);

    /// <summary>
    /// Creates a new actor object of the given type identifier.
    /// [Deprecated: 18.07.2019 expires 18.07.2020]
    /// </summary>
    /// <param name="typeId">The type identifier.</param>
    /// <param name="id">The actor identifier.</param>
    /// <returns>The created actor, or null if failed.</returns>
    static Actor* CreateActor(int32 typeId, const Guid& id);

public:
    struct PrefabSyncData
    {
        friend SceneObjectsFactory;
        friend class PrefabManager;

        // The created scene objects. Collection can be modified (eg. for spawning missing objects).
        Array<SceneObject*>& SceneObjects;
        // The scene objects data.
        const ISerializable::DeserializeStream& Data;
        // The objects deserialization modifier. Collection will be modified (eg. for spawned objects mapping).
        ISerializeModifier* Modifier;

        PrefabSyncData(Array<SceneObject*>& sceneObjects, const ISerializable::DeserializeStream& data, ISerializeModifier* modifier);
        void InitNewObjects();

    private:
        struct NewObj
        {
            Prefab* Prefab;
            const ISerializable::DeserializeStream* PrefabData;
            Guid PrefabObjectId;
            Guid Id;
        };

        int32 InitialCount;
        Array<NewObj> NewObjects;
    };

    /// <summary>
    /// Initializes the prefab instances inside the scene objects for proper references deserialization.
    /// </summary>
    /// <remarks>
    /// Should be called after spawning scene objects but before scene objects deserialization.
    /// </remarks>
    /// <param name="context">The serialization context.</param>
    /// <param name="data">The sync data.</param>
    static void SetupPrefabInstances(Context& context, const PrefabSyncData& data);

    /// <summary>
    /// Synchronizes the new prefab instances by spawning missing objects that were added to prefab but were not saved with scene objects collection.
    /// </summary>
    /// <remarks>
    /// Should be called after spawning scene objects but before scene objects deserialization and PostLoad event when scene objects hierarchy is ready (parent-child relation exists). But call it before Init and BeginPlay events.
    /// </remarks>
    /// <param name="context">The serialization context.</param>
    /// <param name="data">The sync data.</param>
    static void SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data);

    /// <summary>
    /// Synchronizes the prefab instances. Prefabs may have objects removed so deserialized instances need to synchronize with it. Handles also changing prefab object parent in the instance.
    /// </summary>
    /// <remarks>
    /// Should be called after scene objects deserialization and PostLoad event when scene objects hierarchy is ready (parent-child relation exists). But call it before Init and BeginPlay events.
    /// </remarks>
    /// <param name="context">The serialization context.</param>
    /// <param name="data">The sync data.</param>
    static void SynchronizePrefabInstances(Context& context, PrefabSyncData& data);

private:
    static void SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const Guid& actorPrefabObjectId, int32 i, const ISerializable::DeserializeStream& stream);
    static void SynchronizeNewPrefabInstance(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const Guid& prefabObjectId);
};
