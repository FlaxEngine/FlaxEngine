// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Serialization/ISerializable.h"
#include "Engine/Core/Collections/Array.h"

class SceneTicking;
class ScriptsFactory;
class Actor;
class Script;
class Joint;
class Scene;
class Prefab;
class PrefabInstanceData;
class SceneObject;
class PrefabManager;
class Level;

/// <summary>
/// Scene objects setup data container used for BeginPlay callback.
/// </summary>
class SceneBeginData
{
public:

    /// <summary>
    /// The joints to create after setup.
    /// </summary>
    Array<Joint*> JointsToCreate;

    /// <summary>
    /// Called when scene objects setup is done.
    /// </summary>
    void OnDone();
};

/// <summary>
/// The actors collection lookup type (id -> actor).
/// </summary>
typedef Dictionary<Guid, Actor*, HeapAllocation> ActorsLookup;

#define DECLARE_SCENE_OBJECT(type) \
    DECLARE_SCRIPTING_TYPE(type)

#define DECLARE_SCENE_OBJECT_ABSTRACT(type) \
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(type); \
    explicit type(const SpawnParams& params)

#define DECLARE_SCENE_OBJECT_NO_SPAWN(type) \
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(type)

/// <summary>
/// Base class for objects that are parts of the scene (actors and scripts).
/// </summary>
API_CLASS(Abstract, NoSpawn) class FLAXENGINE_API SceneObject : public PersistentScriptingObject, public ISerializable
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(SceneObject);
    friend PrefabInstanceData;
    friend Actor;
    friend Level;
    friend ScriptsFactory;
    friend SceneTicking;
public:
    typedef PersistentScriptingObject Base;

    // Scene Object lifetime flow:
    // - Create
    // - Is created from code:
    //    - Post Spawn
    // - otherwise:
    //   - Deserialize (more than once for prefab instances)
    //   - Post Load
    // - Begin Play
    // - End Play
    // - Destroy

protected:

    Actor* _parent;
    Guid _prefabID;
    Guid _prefabObjectID;

    /// <summary>
    /// Initializes a new instance of the <see cref="SceneObject"/> class.
    /// </summary>
    /// <param name="params">The object initialization parameters.</param>
    SceneObject(const SpawnParams& params);

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="SceneObject"/> class.
    /// </summary>
    ~SceneObject();

public:

    /// <summary>
    /// Determines whether object is during play (spawned/loaded and fully initialized).
    /// </summary>
    /// <returns><c>true</c> if object is during play; otherwise, <c>false</c>.</returns>
    FORCE_INLINE bool IsDuringPlay() const
    {
        return (Flags & ObjectFlags::IsDuringPlay) != 0;
    }

    /// <summary>
    /// Returns true if object has a parent assigned.
    /// </summary>
    /// <returns>True if has parent, otherwise false.</returns>
    API_PROPERTY() FORCE_INLINE bool HasParent() const
    {
        return _parent != nullptr;
    }

    /// <summary>
    /// Gets the parent actor (or null if object has no parent).
    /// </summary>
    /// <returns>The parent actor.</returns>
    API_PROPERTY(Attributes="HideInEditor")
    FORCE_INLINE Actor* GetParent() const
    {
        return _parent;
    }

    /// <summary>
    /// Sets the parent actor.
    /// </summary>
    /// <param name="value">The new parent.</param>
    API_PROPERTY() FORCE_INLINE void SetParent(Actor* value)
    {
        SetParent(value, true);
    }

    /// <summary>
    /// Sets the parent actor.
    /// </summary>
    /// <param name="value">The new parent.</param>
    /// <param name="canBreakPrefabLink">True if can break prefab link on changing the parent.</param>
    API_FUNCTION() virtual void SetParent(Actor* value, bool canBreakPrefabLink) = 0;

    /// <summary>
    /// Gets the scene object ID.
    /// </summary>
    /// <returns>The scene object ID.</returns>
    virtual const Guid& GetSceneObjectId() const = 0;

    /// <summary>
    /// Gets zero-based index in parent actor children list (scripts or child actors).
    /// </summary>
    /// <returns>The order in parent.</returns>
    API_PROPERTY(Attributes="HideInEditor")
    virtual int32 GetOrderInParent() const = 0;

    /// <summary>
    /// Sets zero-based index in parent actor children list (scripts or child actors).
    /// </summary>
    /// <param name="index">The new order in parent.</param>
    API_PROPERTY() virtual void SetOrderInParent(int32 index) = 0;

public:

    /// <summary>
    /// Gets a value indicating whether this object has a valid linkage to the prefab asset.
    /// </summary>
    /// <returns>True if actor has valid prefab link, otherwise false.</returns>
    API_PROPERTY() FORCE_INLINE bool HasPrefabLink() const
    {
        return _prefabID.IsValid();
    }

    /// <summary>
    /// Gets the prefab asset ID. Empty if no prefab link exists.
    /// </summary>
    /// <returns>The prefab asset ID.</returns>
    API_PROPERTY() FORCE_INLINE Guid GetPrefabID() const
    {
        return _prefabID;
    }

    /// <summary>
    /// Gets the ID of the object within a prefab that is used for synchronization with this object. Empty if no prefab link exists.
    /// </summary>
    /// <returns>The prefab object ID.</returns>
    API_PROPERTY() FORCE_INLINE Guid GetPrefabObjectID() const
    {
        return _prefabObjectID;
    }

    /// <summary>
    /// Links scene object instance to the prefab asset and prefab object. Warning! This applies to the only this object (not scripts or child actors).
    /// </summary>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="prefabObjectId">The prefab object identifier.</param>
    API_FUNCTION() virtual void LinkPrefab(const Guid& prefabId, const Guid& prefabObjectId);

    /// <summary>
    /// Breaks the prefab linkage for this object, all its scripts, and all child actors.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate")
    virtual void BreakPrefabLink();

    /// <summary>
    /// Gets the path containing name of this object and all parent objects in tree hierarchy separated with custom separator character (/ by default). Can be used to identify this object in logs.
    /// </summary>
    /// <param name="separatorChar">The character to separate the names.</param>
    /// <returns>The full name path.</returns>
    API_FUNCTION() String GetNamePath(Char separatorChar = '/') const;

public:

    /// <summary>
    /// Called after whole scene or local group of scene objects deserialization.
    /// </summary>
    virtual void PostLoad() = 0;

    /// <summary>
    /// Called after spawning scene object to the level (similar to the PostLoad but only for spawned objects at runtime).
    /// </summary>
    virtual void PostSpawn() = 0;

    /// <summary>
    /// Called when adding object to the game.
    /// </summary>
    /// <param name="data">The initialization data (e.g. used to collect joints to link after begin).</param>
    virtual void BeginPlay(SceneBeginData* data) = 0;

    /// <summary>
    /// Called when removing object from the game.
    /// </summary>
    virtual void EndPlay() = 0;

public:

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
