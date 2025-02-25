// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Serialization/JsonFwd.h"
#include "Types.h"

class JsonWriter;
class Engine;
struct RenderView;
struct RenderContext;
struct RenderContextBatch;
struct Tag;

/// <summary>
/// The scene manager that contains the loaded scenes collection and spawns/deleted actors.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Level
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Level);
    friend Engine;
    friend Actor;
    friend PrefabManager;
    friend Prefab;
    friend PrefabInstanceData;
    friend class LoadSceneAction;
#if USE_EDITOR
    friend class ReloadScriptsAction;
#endif

public:
    /// <summary>
    /// The scenes collection lock.
    /// </summary>
    static CriticalSection ScenesLock;

    /// <summary>
    /// The loaded scenes collection.
    /// </summary>
    API_FIELD(ReadOnly) static Array<Scene*> Scenes;

    /// <summary>
    /// True if game objects (actors and scripts) can receive a tick during engine Update/LateUpdate/FixedUpdate events. Can be used to temporarily disable gameplay logic updating.
    /// </summary>
    API_FIELD() static bool TickEnabled;

public:
    /// <summary>
    /// Occurs when new actor gets spawned to the game.
    /// </summary>
    API_EVENT() static Delegate<Actor*> ActorSpawned;

    /// <summary>
    /// Occurs when actor is removed from the game.
    /// </summary>
    API_EVENT() static Delegate<Actor*> ActorDeleted;

    /// <summary>
    /// Occurs when actor parent gets changed. Arguments: actor and previous parent actor.
    /// </summary>
    API_EVENT() static Delegate<Actor*, Actor*> ActorParentChanged;

    /// <summary>
    /// Occurs when actor index in parent actor children gets changed.
    /// </summary>
    API_EVENT() static Delegate<Actor*> ActorOrderInParentChanged;

    /// <summary>
    /// Occurs when actor name gets changed.
    /// </summary>
    API_EVENT() static Delegate<Actor*> ActorNameChanged;

    /// <summary>
    /// Occurs when actor active state gets modified.
    /// </summary>
    API_EVENT() static Delegate<Actor*> ActorActiveChanged;

public:
    /// <summary>
    /// Checks if any scene has been loaded. Loaded scene means deserialized and added to the scenes collection.
    /// </summary>
    API_PROPERTY() FORCE_INLINE static bool IsAnySceneLoaded()
    {
        return Scenes.HasItems();
    }

    /// <summary>
    /// Checks if any scene has any actor
    /// </summary>
    /// <returns>True if any scene as any actor, otherwise false</returns>
    API_PROPERTY() static bool IsAnyActorInGame();

    /// <summary>
    /// Checks if any scene action is pending
    /// </summary>
    /// <returns>True if scene action will be performed during next update, otherwise false</returns>
    API_PROPERTY() static bool IsAnyActionPending();

    /// <summary>
    /// Gets the last scene load time (in UTC).
    /// </summary>
    /// <returns>Last scene load time</returns>
    API_PROPERTY() static DateTime GetLastSceneLoadTime();

    /// <summary>
    /// Gets the scenes count.
    /// </summary>
    API_PROPERTY() static int32 GetScenesCount()
    {
        return Scenes.Count();
    }

    /// <summary>
    /// Gets the scene.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The scene object (loaded).</returns>
    API_FUNCTION() static Scene* GetScene(int32 index)
    {
        CHECK_RETURN(index < GetScenesCount(), nullptr);
        return Scenes[index];
    }

public:
    /// <summary>
    /// Spawn actor on the scene
    /// </summary>
    /// <param name="actor">Actor to spawn</param>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    API_FUNCTION() FORCE_INLINE static bool SpawnActor(Actor* actor)
    {
        return SpawnActor(actor, nullptr);
    }

    /// <summary>
    /// Spawns actor on the scene.
    /// </summary>
    /// <param name="actor">The actor to spawn.</param>
    /// <param name="parent">The parent actor (will link spawned actor with this parent).</param>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool SpawnActor(Actor* actor, Actor* parent);

    /// <summary>
    /// Deletes actor from the scene.
    /// </summary>
    /// <param name="actor">The actor to delete.</param>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    static bool DeleteActor(Actor* actor);

    /// <summary>
    /// Calls BeginPlay event for the given actor.
    /// </summary>
    /// <remarks>
    /// Use it for custom actors created from code but not added to the any scene. Otherwise there may be some internal state issues. Some actor types cache data on play mode begin so it's important to call this event for them.
    /// </remarks>
    /// <param name="obj">The actor to initialize.</param>
    static void CallBeginPlay(Actor* obj);

public:
    /// <summary>
    /// Draws all the actors.
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch.</param>
    /// <param name="category">The actors category to draw (see SceneRendering::DrawCategory).</param>
    static void DrawActors(RenderContextBatch& renderContextBatch, byte category = 0);

    /// <summary>
    /// Collects all the post fx volumes.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    static void CollectPostFxVolumes(RenderContext& renderContext);

public:
    /// <summary>
    /// Fired when scene starts saving.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneSaving;

    /// <summary>
    /// Fired when scene gets saved.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneSaved;

    /// <summary>
    /// Fired when scene gets saving error.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneSaveError;

    /// <summary>
    /// Fired when scene starts loading.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneLoading;

    /// <summary>
    /// Fired when scene gets loaded.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneLoaded;

    /// <summary>
    /// Fired when scene cannot be loaded (argument is error number).
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneLoadError;

    /// <summary>
    /// Fired when scene gets unloading.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneUnloading;

    /// <summary>
    /// Fired when scene gets unloaded.
    /// </summary>
    API_EVENT() static Delegate<Scene*, const Guid&> SceneUnloaded;

#if USE_EDITOR

    /// <summary>
    /// Fired when scene starts reloading scripts.
    /// </summary>
    API_EVENT() static Action ScriptsReloadStart;

    /// <summary>
    /// Fired when scene reloads scripts (scenes serialized and unloaded). All user objects should be cleaned up.
    /// </summary>
    API_EVENT() static Action ScriptsReload;

    /// <summary>
    /// Fired when scene reloaded scripts (new scripting is loaded). All user objects can be restored before scenes reloading.
    /// </summary>
    API_EVENT() static Action ScriptsReloaded;

    /// <summary>
    /// Fired when scene ends reloading scripts.
    /// </summary>
    API_EVENT() static Action ScriptsReloadEnd;

    /// <summary>
    /// Adds object to preserve during scripts reload. Called during ScriptsReloadStart event to serialize and destroy the object that should be restored when scripts reload ends.
    /// </summary>
    /// <param name="obj">Reference to the object to preserve during the scripting reload.</param>
    static void ScriptsReloadRegisterObject(ScriptingObject*& obj);

#endif

public:
    /// <summary>
    /// Saves scene to the asset.
    /// </summary>
    /// <param name="scene">Scene to serialize.</param>
    /// <param name="prettyJson">True if use pretty Json format writer, otherwise will use the compact Json format writer that packs data to use less memory and perform the action faster.</param>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool SaveScene(Scene* scene, bool prettyJson = true);

    /// <summary>
    /// Saves scene to the bytes.
    /// </summary>
    /// <param name="scene">Scene to serialize.</param>
    /// <param name="outData">The result data.</param>
    /// <param name="prettyJson">True if use pretty Json format writer, otherwise will use the compact Json format writer that packs data to use less memory and perform the action faster.</param>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    static bool SaveSceneToBytes(Scene* scene, rapidjson_flax::StringBuffer& outData, bool prettyJson = true);

    /// <summary>
    /// Saves scene to the bytes.
    /// </summary>
    /// <param name="scene">Scene to serialize.</param>
    /// <param name="prettyJson">True if use pretty Json format writer, otherwise will use the compact Json format writer that packs data to use less memory and perform the action faster.</param>
    /// <returns>The result data or empty if failed.</returns>
    API_FUNCTION() static Array<byte> SaveSceneToBytes(Scene* scene, bool prettyJson = true);

    /// <summary>
    /// Saves scene to the asset. Done in the background.
    /// </summary>
    /// <param name="scene">Scene to serialize.</param>
    API_FUNCTION() static void SaveSceneAsync(Scene* scene);

    /// <summary>
    /// Saves all scenes to the assets.
    /// </summary>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool SaveAllScenes();

    /// <summary>
    /// Saves all scenes to the assets. Done in the background.
    /// </summary>
    API_FUNCTION() static void SaveAllScenesAsync();

    /// <summary>
    /// Loads scene from the asset.
    /// </summary>
    /// <param name="id">Scene ID</param>
    /// <returns>True if loading cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool LoadScene(const Guid& id);

    /// <summary>
    /// Loads scene from the bytes.
    /// </summary>
    /// <param name="data">The scene data to load.</param>
    /// <returns>Loaded scene object, otherwise null if cannot load data (then see log for more information).</returns>
    API_FUNCTION() static Scene* LoadSceneFromBytes(const BytesContainer& data);

    /// <summary>
    /// Loads scene from the asset. Done in the background.
    /// </summary>
    /// <param name="id">Scene ID</param>
    /// <returns>True if loading cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool LoadSceneAsync(const Guid& id);

    /// <summary>
    /// Unloads given scene.
    /// </summary>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool UnloadScene(Scene* scene);

    /// <summary>
    /// Unloads given scene. Done in the background.
    /// </summary>
    API_FUNCTION() static void UnloadSceneAsync(Scene* scene);

    /// <summary>
    /// Unloads all scenes.
    /// </summary>
    /// <returns>True if action cannot be done, otherwise false.</returns>
    API_FUNCTION() static bool UnloadAllScenes();

    /// <summary>
    /// Unloads all scenes. Done in the background.
    /// </summary>
    API_FUNCTION() static void UnloadAllScenesAsync();

#if USE_EDITOR

    /// <summary>
    /// Reloads scripts. Done in the background.
    /// </summary>
    static void ReloadScriptsAsync();

#endif

public:
    /// <summary>
    /// Tries to find actor with the given ID. It's very fast O(1) lookup.
    /// </summary>
    /// <param name="id">The id.</param>
    /// <returns>Found actor or null.</returns>
    API_FUNCTION() static Actor* FindActor(const Guid& id);

    /// <summary>
    /// Tries to find the actor with the given name.
    /// </summary>
    /// <param name="name">The name of the actor.</param>
    /// <returns>Found actor or null.</returns>
    API_FUNCTION() static Actor* FindActor(const StringView& name);

    /// <summary>
    /// Tries to find the actor of the given type in all the loaded scenes.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <param name="activeOnly">Finds only an active actor.</param>
    /// <returns>Found actor or null.</returns>
    API_FUNCTION() static Actor* FindActor(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, bool activeOnly = false);

    /// <summary>
    /// Tries to find the actor of the given type and name in all the loaded scenes.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <param name="name">The name of the actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    API_FUNCTION() static Actor* FindActor(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, const StringView& name);

    /// <summary>
    /// Tries to find the actor with the given tag (returns the first one found).
    /// </summary>
    /// <param name="tag">The tag of the actor to search for.</param>
    /// <param name="activeOnly">Finds only an active actor.</param>
    /// <param name="root">The custom root actor to start searching from (hierarchical), otherwise null to search all loaded scenes.</param>
    /// <returns>Found actor or null.</returns>
    API_FUNCTION() static Actor* FindActor(const Tag& tag, bool activeOnly = false, Actor* root = nullptr);

    /// <summary>
    /// Tries to find the actor of the given type and tag in all the loaded scenes.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <param name="tag">The tag of the actor to search for.</param>
    /// <param name="activeOnly">Finds only an active actor.</param>
    /// <param name="root">The custom root actor to start searching from (hierarchical), otherwise null to search all loaded scenes.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    API_FUNCTION() static Actor* FindActor(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, const Tag& tag, bool activeOnly = false, Actor* root = nullptr);

    /// <summary>
    /// Tries to find the actors with the given tag (returns all found).
    /// </summary>
    /// <param name="tag">The tag of the actor to search for.</param>
    /// <param name="activeOnly">Find only active actors.</param>
    /// <param name="root">The custom root actor to start searching from (hierarchical), otherwise null to search all loaded scenes.</param>
    /// <returns>Found actors or empty if none.</returns>
    API_FUNCTION() static Array<Actor*> FindActors(const Tag& tag, const bool activeOnly = false, Actor* root = nullptr);

    /// <summary>
    /// Search actors using a parent parentTag.
    /// </summary>
    /// <param name="parentTag">The tag to search actors with subtags belonging to this tag</param>
    /// <param name="activeOnly">Find only active actors.</param>
    /// <param name="root">The custom root actor to start searching from (hierarchical), otherwise null to search all loaded scenes.</param>
    /// <returns>Returns all actors that have subtags belonging to the given parent parentTag</returns>
    API_FUNCTION() static Array<Actor*> FindActorsByParentTag(const Tag& parentTag, const bool activeOnly = false, Actor* root = nullptr);

    /// <summary>
    /// Tries to find the actor of the given type in all the loaded scenes.
    /// </summary>
    /// <returns>Actor instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE static T* FindActor()
    {
        return (T*)FindActor(T::GetStaticClass());
    }

    /// <summary>
    /// Tries to find the actor of the given type and name in all the loaded scenes.
    /// </summary>
    /// <param name="name">The name of the actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE static T* FindActor(const StringView& name)
    {
        return (T*)FindActor(T::GetStaticClass(), name);
    }

    /// <summary>
    /// Tries to find the actor of the given type and tag in a root actor or all the loaded scenes.
    /// <param name="tag">The tag of the actor to search for.</param>
    /// <param name="root">The custom root actor to start searching from (hierarchical), otherwise null to search all loaded scenes.</param>
    /// </summary>
    /// <returns>Actor instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE static T* FindActor(const Tag& tag, Actor* root = nullptr)
    {
        return (T*)FindActor(T::GetStaticClass(), tag, root);
    }

    /// <summary>
    /// Tries to find the script of the given type in all the loaded scenes.
    /// </summary>
    /// <param name="type">Type of the script to search for. Includes any scripts derived from the type. Supports interface types.</param>
    /// <returns>Found script or null.</returns>
    API_FUNCTION() static Script* FindScript(API_PARAM(Attributes="TypeReference(typeof(Script))") const MClass* type);

    /// <summary>
    /// Tries to find the script of the given type in all the loaded scenes.
    /// </summary>
    /// <returns>Script instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE static T* FindScript()
    {
        return (T*)FindScript(T::GetStaticClass());
    }

    /// <summary>
    /// Finds all the actors of the given type in all the loaded scenes.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type.</param>
    /// <param name="activeOnly">Finds only active actors in the scene.</param>
    /// <returns>Found actors list.</returns>
    API_FUNCTION() static Array<Actor*> GetActors(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, bool activeOnly = false);

    /// <summary>
    /// Finds all the scripts of the given type in an actor or all the loaded scenes.
    /// </summary>
    /// <param name="type">Type of the script to search for. Includes any scripts derived from the type.</param>
    /// <param name="root">The root to find scripts. If null, will search in all scenes.</param>
    /// <returns>Found scripts list.</returns>
    API_FUNCTION() static Array<Script*> GetScripts(API_PARAM(Attributes="TypeReference(typeof(Script))") const MClass* type, Actor* root = nullptr);

    /// <summary>
    /// Tries to find scene with given ID.
    /// </summary>
    /// <param name="id">Scene id.</param>
    /// <returns>Found scene or null.</returns>
    API_FUNCTION() static Scene* FindScene(const Guid& id);

public:
    /// <summary>
    /// Gets the scenes.
    /// </summary>
    /// <param name="scenes">The scenes.</param>
    static void GetScenes(Array<Scene*>& scenes);

    /// <summary>
    /// Gets the scenes (as actors).
    /// </summary>
    /// <param name="scenes">The scenes.</param>
    static void GetScenes(Array<Actor*>& scenes);

    /// <summary>
    /// Gets the scene ids.
    /// </summary>
    /// <param name="scenes">The scene ids.</param>
    static void GetScenes(Array<Guid>& scenes);

public:
    /// <summary>
    /// Construct valid and solid list with actors from input list with whole tree for them (valid for fast serialization)
    /// </summary>
    /// <param name="input">Input array of actors</param>
    /// <param name="output">Output linear array of actors from up to down the tree with all children of given input actors</param>
    static void ConstructSolidActorsTreeList(const Array<Actor*>& input, Array<Actor*>& output);

    /// <summary>
    /// Construct list with actors from input list that contains only parent actors (no children)
    /// </summary>
    /// <param name="input">Input array of actors</param>
    /// <param name="output">Output array with only parents</param>
    static void ConstructParentActorsTreeList(const Array<Actor*>& input, Array<Actor*>& output);

public:
    /// <summary>
    /// The layers names.
    /// </summary>
    static String Layers[32];

    /// <summary>
    /// Gets the amount of non empty layer names (from the beginning, trims the last ones).
    /// </summary>
    /// <returns>The layers count.</returns>
    static int32 GetNonEmptyLayerNamesCount();

    /// <summary>
    /// Gets the zero-based index of the layer.
    /// </summary>
    API_FUNCTION() static int32 GetLayerIndex(const StringView& layer);

private:
    // Actor API
    enum class ActorEventType
    {
        OnActorSpawned = 0,
        OnActorDeleted = 1,
        OnActorParentChanged = 2,
        OnActorOrderInParentChanged = 3,
        OnActorNameChanged = 4,
        OnActorActiveChanged = 5,
    };

    static void callActorEvent(ActorEventType eventType, Actor* a, Actor* b);

    // All loadScene assume that ScenesLock has been taken by the calling thread
    static bool loadScene(JsonAsset* sceneAsset);
    static bool loadScene(const BytesContainer& sceneData, Scene** outScene = nullptr);
    static bool loadScene(rapidjson_flax::Document& document, Scene** outScene = nullptr);
    static bool loadScene(rapidjson_flax::Value& data, int32 engineBuild, Scene** outScene = nullptr);
};
