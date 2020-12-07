// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "SceneObject.h"

/// <summary>
/// Helper class for scene objects creation and deserialization utilities.
/// </summary>
class SceneObjectsFactory
{
public:

    typedef Dictionary<Actor*, const rapidjson_flax::Value*> ActorToRemovedObjectsDataLookup;

public:

    /// <summary>
    /// Creates the scene object from the specified data value. Does not perform deserialization.
    /// </summary>
    /// <param name="stream">The serialized data stream.</param>
    /// <param name="modifier">The serialization modifier. Cannot be null.</param>
    static SceneObject* Spawn(ISerializable::DeserializeStream& stream, ISerializeModifier* modifier);

    /// <summary>
    /// Deserializes the scene object from the specified data value.
    /// </summary>
    /// <param name="obj">The instance to deserialize.</param>
    /// <param name="stream">The serialized data stream.</param>
    /// <param name="modifier">The serialization modifier. Cannot be null.</param>
    static void Deserialize(SceneObject* obj, ISerializable::DeserializeStream& stream, ISerializeModifier* modifier);

    /// <summary>
    /// Synchronizes the prefab instances. Prefabs may have new objects added or some removed so deserialized instances need to synchronize with it. Handles also changing prefab object parent in the instance.
    /// </summary>
    /// <remarks>
    /// Should be called after scene objects deserialization and PostLoad event when scene objects hierarchy is ready (parent-child relation exists). But call it before Init and BeginPlay events.
    /// </remarks>
    /// <param name="sceneObjects">The loaded scene objects. Collection will be modified after usage.</param>
    /// <param name="actorToRemovedObjectsData">Maps the loaded actor object to the json data with the RemovedObjects array (used to skip restoring objects removed per prefab instance).</param>
    /// <param name="modifier">The objects deserialization modifier. Collection will be modified after usage.</param>
    static void SynchronizePrefabInstances(Array<SceneObject*>& sceneObjects, const ActorToRemovedObjectsDataLookup& actorToRemovedObjectsData, ISerializeModifier* modifier);

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

private:

    static void SynchronizeNewPrefabInstance(Prefab* prefab, Actor* actor, const Guid& prefabObjectId, Array<SceneObject*>& sceneObjects, ISerializeModifier* modifier);
};
