// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Level.h"
#include "ActorsCache.h"
#include "SceneQuery.h"
#include "SceneObjectsFactory.h"
#include "Scene/Scene.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Debug/Exceptions/ArgumentException.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Engine/Time.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Serialization/JsonTools.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Prefabs/Prefab.h"
#if USE_EDITOR
#include "Editor/Editor.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Engine/CommandLine.h"
#endif

enum class SceneEventType
{
    OnSceneSaving = 0,
    OnSceneSaved = 1,
    OnSceneSaveError = 2,
    OnSceneLoading = 3,
    OnSceneLoaded = 4,
    OnSceneLoadError = 5,
    OnSceneUnloading = 6,
    OnSceneUnloaded = 7,
};

class SceneAction
{
public:

    virtual ~SceneAction()
    {
    }

    virtual bool CanDo() const
    {
        return true;
    }

    virtual bool Do() const
    {
        return true;
    }
};

namespace LevelImpl
{
    Array<SceneAction*> _sceneActions;
    CriticalSection _sceneActionsLocker;
    DateTime _lastSceneLoadTime(0);

    void CallSceneEvent(SceneEventType eventType, Scene* scene, Guid sceneId);

    void flushActions();
    bool unloadScene(Scene* scene);
    bool unloadScenes();
    bool saveScene(Scene* scene);
    bool saveScene(Scene* scene, const String& path);
    bool saveScene(Scene* scene, rapidjson_flax::StringBuffer& outBuffer, bool prettyJson);
    bool saveScene(Scene* scene, rapidjson_flax::StringBuffer& outBuffer, JsonWriter& writer);
    bool spawnActor(Actor* actor, Actor* parent);
    bool deleteActor(Actor* actor);
}

using namespace LevelImpl;

class LevelService : public EngineService
{
public:

    LevelService()
        : EngineService(TEXT("Scene Manager"), 30)
    {
    }

    void Update() override;
    void LateUpdate() override;
    void FixedUpdate() override;
    void Dispose() override;
};

LevelService LevelServiceInstanceService;

CriticalSection Level::ScenesLock;
Array<Scene*> Level::Scenes;
Delegate<Actor*> Level::ActorSpawned;
Delegate<Actor*> Level::ActorDeleted;
Delegate<Actor*, Actor*> Level::ActorParentChanged;
Delegate<Actor*> Level::ActorOrderInParentChanged;
Delegate<Actor*> Level::ActorNameChanged;
Delegate<Actor*> Level::ActorActiveChanged;
Delegate<Scene*, const Guid&> Level::SceneSaving;
Delegate<Scene*, const Guid&> Level::SceneSaved;
Delegate<Scene*, const Guid&> Level::SceneSaveError;
Delegate<Scene*, const Guid&> Level::SceneLoading;
Delegate<Scene*, const Guid&> Level::SceneLoaded;
Delegate<Scene*, const Guid&> Level::SceneLoadError;
Delegate<Scene*, const Guid&> Level::SceneUnloading;
Delegate<Scene*, const Guid&> Level::SceneUnloaded;
Action Level::ScriptsReloadStart;
Action Level::ScriptsReload;
Action Level::ScriptsReloadEnd;

bool LevelImpl::spawnActor(Actor* actor, Actor* parent)
{
    if (actor == nullptr)
    {
        Log::ArgumentNullException(TEXT("Cannot spawn null actor."));
        return true;
    }
    if (Level::Scenes.IsEmpty())
    {
        Log::InvalidOperationException(TEXT("Cannot spawn actor. No scene loaded."));
        return true;
    }
    if (parent == nullptr)
        parent = Level::Scenes[0];

    actor->SetParent(parent, true, true);

    return false;
}

bool LevelImpl::deleteActor(Actor* actor)
{
    if (actor == nullptr)
    {
        Log::ArgumentNullException(TEXT("Cannot delete null actor."));
        return true;
    }

    actor->DeleteObject();

    return false;
}

void LevelService::Update()
{
    PROFILE_CPU();

    ScopeLock lock(Level::ScenesLock);
    auto& scenes = Level::Scenes;

    // Update all actors
    if (!Time::GetGamePaused())
    {
        for (int32 i = 0; i < scenes.Count(); i++)
        {
            if (scenes[i]->GetIsActive())
                scenes[i]->Ticking.Update.Tick();
        }
    }
#if USE_EDITOR
    else if (!Editor::IsPlayMode)
    {
        // Run event for script executed in editor
        for (int32 i = 0; i < scenes.Count(); i++)
        {
            if (scenes[i]->GetIsActive())
                scenes[i]->Ticking.Update.TickEditorScripts();
        }
    }
#endif
}

void LevelService::LateUpdate()
{
    PROFILE_CPU();

    ScopeLock lock(Level::ScenesLock);
    auto& scenes = Level::Scenes;

    // Update all actors
    if (!Time::GetGamePaused())
    {
        for (int32 i = 0; i < scenes.Count(); i++)
        {
            if (scenes[i]->GetIsActive())
                scenes[i]->Ticking.LateUpdate.Tick();
        }
    }
#if USE_EDITOR
    else if (!Editor::IsPlayMode)
    {
        // Run event for script executed in editor
        for (int32 i = 0; i < scenes.Count(); i++)
        {
            if (scenes[i]->GetIsActive())
                scenes[i]->Ticking.LateUpdate.TickEditorScripts();
        }
    }
#endif

    // Flush actions
    flushActions();
}

void LevelService::FixedUpdate()
{
    PROFILE_CPU();

    ScopeLock lock(Level::ScenesLock);
    auto& scenes = Level::Scenes;

    // Update all actors
    if (!Time::GetGamePaused())
    {
        for (int32 i = 0; i < scenes.Count(); i++)
        {
            if (scenes[i]->GetIsActive())
                scenes[i]->Ticking.FixedUpdate.Tick();
        }
    }
#if USE_EDITOR
    else if (!Editor::IsPlayMode)
    {
        // Run event for script executed in editor
        for (int32 i = 0; i < scenes.Count(); i++)
        {
            if (scenes[i]->GetIsActive())
                scenes[i]->Ticking.FixedUpdate.TickEditorScripts();
        }
    }
#endif
}

void LevelService::Dispose()
{
    ScopeLock lock(_sceneActionsLocker);

    // Unload scenes
    unloadScenes();

    // Ensure that all scenes and actors has been destroyed (we don't leak!)
    ASSERT(Level::Scenes.IsEmpty());
}

bool Level::IsAnyActorInGame()
{
    ScopeLock lock(ScenesLock);

    for (int32 i = 0; i < Scenes.Count(); i++)
    {
        if (Scenes[i]->Children.HasItems())
            return true;
    }

    return false;
}

bool Level::IsAnyActionPending()
{
    _sceneActionsLocker.Lock();
    const bool result = _sceneActions.HasItems();
    _sceneActionsLocker.Unlock();
    return result;
}

DateTime Level::GetLastSceneLoadTime()
{
    return _lastSceneLoadTime;
}

bool Level::SpawnActor(Actor* actor, Actor* parent)
{
    ASSERT(actor);
    ScopeLock lock(_sceneActionsLocker);
    return spawnActor(actor, parent);
}

bool Level::DeleteActor(Actor* actor)
{
    ASSERT(actor);
    ScopeLock lock(_sceneActionsLocker);
    return deleteActor(actor);
}

void Level::CallBeginPlay(Actor* obj)
{
    if (obj && !obj->IsDuringPlay())
    {
        SceneBeginData beginData;
        obj->BeginPlay(&beginData);
        beginData.OnDone();
    }
}

void Level::DrawActors(RenderContext& renderContext)
{
    PROFILE_CPU();

    //ScopeLock lock(ScenesLock);

    for (int32 i = 0; i < Scenes.Count(); i++)
    {
        Scenes[i]->Rendering.Draw(renderContext);
    }
}

void Level::CollectPostFxVolumes(RenderContext& renderContext)
{
    PROFILE_CPU();

    //ScopeLock lock(ScenesLock);

    for (int32 i = 0; i < Scenes.Count(); i++)
    {
        Scenes[i]->Rendering.CollectPostFxVolumes(renderContext);
    }
}

class LoadSceneAction : public SceneAction
{
public:

    Guid SceneId;
    AssetReference<JsonAsset> SceneAsset;
    bool AutoInitialize;

    LoadSceneAction(const Guid& sceneId, JsonAsset* sceneAsset, bool autoInitialize)
    {
        SceneId = sceneId;
        SceneAsset = sceneAsset;
        AutoInitialize = autoInitialize;
    }

    bool CanDo() const override
    {
        return SceneAsset == nullptr || SceneAsset->IsLoaded();
    }

    bool Do() const override
    {
        // Now to deserialize scene in a proper way we need to load scripting
        if (!Scripting::IsEveryAssemblyLoaded())
        {
            LOG(Error, "Scripts must be compiled without any errors in order to load a scene.");
#if USE_EDITOR
            Platform::Error(TEXT("Scripts must be compiled without any errors in order to load a scene. Please fix it."));
#endif
            CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, SceneId);
            return true;
        }

        // Load scene
        if (Level::loadScene(SceneAsset.Get(), AutoInitialize))
        {
            LOG(Error, "Failed to deserialize scene {0}", SceneId);
            CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, SceneId);
            return true;
        }

        return false;
    }
};

class UnloadSceneAction : public SceneAction
{
public:

    Scene* TargetScene;

    UnloadSceneAction(Scene* scene)
    {
        TargetScene = scene;
    }

    bool Do() const override
    {
        return unloadScene(TargetScene);
    }
};

class UnloadScenesAction : public SceneAction
{
public:

    UnloadScenesAction()
    {
    }

    bool Do() const override
    {
        return unloadScenes();
    }
};

class SaveSceneAction : public SceneAction
{
public:

    Scene* TargetScene;
    bool PrettyJson;

    SaveSceneAction(Scene* scene, bool prettyJson = true)
    {
        TargetScene = scene;
        PrettyJson = prettyJson;
    }

    bool Do() const override
    {
        if (saveScene(TargetScene))
        {
            LOG(Error, "Failed to save scene {0}", TargetScene ? TargetScene->GetName() : String::Empty);
            return true;
        }

        return false;
    }
};

#if USE_EDITOR

class ReloadScriptsAction : public SceneAction
{
public:

    ReloadScriptsAction()
    {
    }

    bool Do() const override
    {
        // Reloading scripts workflow:
        // - save scenes (to temporary files)
        // - unload scenes
        // - unload user assemblies
        // - load user assemblies
        // - load scenes (from temporary files)
        // Note: we don't want to override original scene files

        // If no scene loaded just reload scripting
        if (!Level::IsAnySceneLoaded())
        {
            // Reload scripting
            LOG(Info, "No scenes loaded, performing fast scripts reload");
            Scripting::Reload(false);
            return false;
        }

        // Cache data
        struct SceneData
        {
            Guid ID;
            String Name;
            rapidjson_flax::StringBuffer Data;

            SceneData() = default;

            SceneData(const SceneData& other)
            {
                CRASH;
            }

            const SceneData& operator=(SceneData&) const
            {
                CRASH;
                return *this;
            }

            void Init(Scene* scene)
            {
                ID = scene->GetID();
                Name = scene->GetName();
            }
        };
        const int32 scenesCount = Level::Scenes.Count();
        Array<SceneData> scenes;
        scenes.Resize(scenesCount);
        for (int32 i = 0; i < scenesCount; i++)
            scenes[i].Init(Level::Scenes[i]);

        // Fire event
        LOG(Info, "Scripts reloading start");
        const auto startTime = DateTime::NowUTC();
        Level::ScriptsReloadStart();

        // Save scenes (to memory)
        for (int32 i = 0; i < scenesCount; i++)
        {
            const auto scene = Level::Scenes[i];
            LOG(Info, "Caching scene {0}", scenes[i].Name);

            // Serialize to json
            if (saveScene(scene, scenes[i].Data, false))
            {
                LOG(Error, "Failed to save scene '{0}' for scripts reload.", scenes[i].Name);
                CallSceneEvent(SceneEventType::OnSceneSaveError, scene, scene->GetID());
                return true;
            }
            CallSceneEvent(SceneEventType::OnSceneSaved, scene, scene->GetID());
        }

        // Unload scenes
        unloadScenes();

        // Reload scripting
        Level::ScriptsReload();
        Scripting::Reload();

        // Restore scenes (from memory)
        LOG(Info, "Loading temporary scenes");
        for (int32 i = 0; i < scenesCount; i++)
        {
            // Parse json
            const auto& sceneData = scenes[i].Data;
            ISerializable::SerializeDocument document;
            document.Parse(sceneData.GetString(), sceneData.GetSize());
            if (document.HasParseError())
            {
                LOG(Error, "Failed to deserialize scene {0}. Result: {1}", scenes[i].Name, GetParseError_En(document.GetParseError()));
                return true;
            }

            // Load scene
            if (Level::loadScene(document, false))
            {
                LOG(Error, "Failed to deserialize scene {0}", scenes[i].Name);
                CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, scenes[i].ID);
                return true;
            }
        }
        scenes.Resize(0);

        // Initialize scenes (will link references and create managed objects using new assembly)
        LOG(Info, "Prepare scene objects");
        {
            SceneBeginData beginData;
            for (int32 i = 0; i < Level::Scenes.Count(); i++)
            {
                Level::Scenes[i]->BeginPlay(&beginData);
            }
            beginData.OnDone();
        }

        // Fire event
        const auto endTime = DateTime::NowUTC();
        LOG(Info, "Scripts reloading end. Total time: {0}ms", static_cast<int32>((endTime - startTime).GetTotalMilliseconds()));
        Level::ScriptsReloadEnd();

        return false;
    }
};

#endif

class SpawnActorAction : public SceneAction
{
public:

    ScriptingObjectReference<Actor> TargetActor;
    ScriptingObjectReference<Actor> ParentActor;

    SpawnActorAction(Actor* actor, Actor* parent)
    {
        TargetActor = actor;
        ParentActor = parent;
    }

    bool Do() const override
    {
        return spawnActor(TargetActor, ParentActor);
    }
};

class DeleteActorAction : public SceneAction
{
public:

    ScriptingObjectReference<Actor> TargetActor;

    DeleteActorAction(Actor* actor)
    {
        TargetActor = actor;
    }

    bool Do() const override
    {
        return deleteActor(TargetActor);
    }
};

void LevelImpl::CallSceneEvent(SceneEventType eventType, Scene* scene, Guid sceneId)
{
    PROFILE_CPU();

    // Call event
    const auto scriptsDomain = Scripting::GetScriptsDomain();
    if (scriptsDomain != nullptr)
        scriptsDomain->Dispatch();
    switch (eventType)
    {
    case SceneEventType::OnSceneSaving:
        Level::SceneSaving(scene, sceneId);
        break;
    case SceneEventType::OnSceneSaved:
        Level::SceneSaved(scene, sceneId);
        break;
    case SceneEventType::OnSceneSaveError:
        Level::SceneSaveError(scene, sceneId);
        break;
    case SceneEventType::OnSceneLoading:
        Level::SceneLoading(scene, sceneId);
        break;
    case SceneEventType::OnSceneLoaded:
        Level::SceneLoaded(scene, sceneId);
        break;
    case SceneEventType::OnSceneLoadError:
        Level::SceneLoadError(scene, sceneId);
        break;
    case SceneEventType::OnSceneUnloading:
        Level::SceneUnloading(scene, sceneId);
        break;
    case SceneEventType::OnSceneUnloaded:
        Level::SceneUnloaded(scene, sceneId);
        break;
    }
}

void Level::callActorEvent(ActorEventType eventType, Actor* a, Actor* b)
{
    PROFILE_CPU();

    ASSERT(a);

    // Call event
    const auto scriptsDomain = Scripting::GetScriptsDomain();
    if (scriptsDomain != nullptr)
        scriptsDomain->Dispatch();
    switch (eventType)
    {
    case ActorEventType::OnActorSpawned:
        ActorSpawned(a);
        break;
    case ActorEventType::OnActorDeleted:
        ActorDeleted(a);
        break;
    case ActorEventType::OnActorParentChanged:
        ActorParentChanged(a, b);
        break;
    case ActorEventType::OnActorOrderInParentChanged:
        ActorOrderInParentChanged(a);
        break;
    case ActorEventType::OnActorNameChanged:
        ActorNameChanged(a);
        break;
    case ActorEventType::OnActorActiveChanged:
        ActorActiveChanged(a);
        break;
    }
}

void LevelImpl::flushActions()
{
    ScopeLock lock(_sceneActionsLocker);

    while (_sceneActions.HasItems() && _sceneActions.First()->CanDo())
    {
        const auto action = _sceneActions.Dequeue();
        action->Do();
        Delete(action);
    }
}

bool LevelImpl::unloadScene(Scene* scene)
{
    if (scene == nullptr)
    {
        Log::ArgumentNullException();
        return true;
    }
    const auto sceneId = scene->GetID();

    PROFILE_CPU_NAMED("Level.UnloadScene");

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneUnloading, scene, sceneId);

    // Call end play
    if (scene->IsDuringPlay())
        scene->EndPlay();

    // Simple enqueue scene root object to be deleted
    Level::Scenes.Remove(scene);
    scene->DeleteObject();

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneUnloaded, nullptr, sceneId);

    // Force flush deleted objects so we actually delete unloaded scene objects (prevent from reloading their managed objects, etc.)
    ObjectsRemovalService::Flush();

    return false;
}

bool LevelImpl::unloadScenes()
{
    auto scenes = Level::Scenes;
    for (int32 i = 0; i < scenes.Count(); i++)
    {
        if (unloadScene(scenes[i]))
            return true;
    }

    return false;
}

bool Level::loadScene(const Guid& sceneId, bool autoInitialize)
{
    const auto sceneAsset = Content::LoadAsync<JsonAsset>(sceneId);
    return loadScene(sceneAsset, autoInitialize);
}

bool Level::loadScene(const String& scenePath, bool autoInitialize)
{
    LOG(Info, "Loading scene from file. Path: \'{0}\'", scenePath);

    // Check for missing file
    if (!FileSystem::FileExists(scenePath))
    {
        LOG(Error, "Missing scene file.");
        return true;
    }

    // Load file
    BytesContainer sceneData;
    if (File::ReadAllBytes(scenePath, sceneData))
    {
        LOG(Error, "Cannot load data from file.");
        return true;
    }

    return loadScene(sceneData, autoInitialize);
}

bool Level::loadScene(JsonAsset* sceneAsset, bool autoInitialize)
{
    // Keep reference to the asset (prevent unloading during action)
    AssetReference<JsonAsset> ref = sceneAsset;

    // Wait for loaded
    if (sceneAsset == nullptr || sceneAsset->WaitForLoaded())
    {
        LOG(Error, "Cannot load scene asset.");
        return true;
    }

    return loadScene(*sceneAsset->Data, sceneAsset->DataEngineBuild, autoInitialize);
}

bool Level::loadScene(const BytesContainer& sceneData, bool autoInitialize, Scene** outScene)
{
    if (sceneData.IsInvalid())
    {
        LOG(Error, "Missing scene data.");
        return true;
    }

    // Parse scene JSON file
    rapidjson_flax::Document document;
    document.Parse(sceneData.Get<char>(), sceneData.Length());
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
        return true;
    }

    return loadScene(document, autoInitialize, outScene);
}

bool Level::loadScene(rapidjson_flax::Document& document, bool autoInitialize, Scene** outScene)
{
    auto data = document.FindMember("Data");
    if (data == document.MemberEnd())
    {
        LOG(Error, "Missing Data member.");
        return true;
    }

    const int32 saveEngineBuild = JsonTools::GetInt(document, "EngineBuild", 0);

    return loadScene(data->value, saveEngineBuild, autoInitialize, outScene);
}

bool Level::loadScene(rapidjson_flax::Value& data, int32 engineBuild, bool autoInitialize, Scene** outScene)
{
    PROFILE_CPU_NAMED("Level.LoadScene");

    if (outScene)
        *outScene = nullptr;

    LOG(Info, "Loading scene...");
    const DateTime startTime = DateTime::NowUTC();
    _lastSceneLoadTime = startTime;

    // Here whole scripting backend should be loaded for current project
    // Later scripts will setup attached scripts and restore initial vars
    if (!Scripting::HasGameModulesLoaded())
    {
        LOG(Error, "Cannot load scene without game modules loaded.");
#if USE_EDITOR
        if (!CommandLine::Options.Headless.IsTrue())
            MessageBox::Show(TEXT("Cannot load scene without game script modules. Please fix the compilation issues."), TEXT("Missing game modules"), MessageBoxButtons::OK, MessageBoxIcon::Error);
#endif
        return true;
    }

    // Peek meta
    if (engineBuild < 6000)
    {
        LOG(Error, "Invalid serialized engine build.");
        return true;
    }
    if (!data.IsArray())
    {
        LOG(Error, "Invalid Data member.");
        return true;
    }
    int32 objectsCount = data.Size();

    // Peek scene node value (it's the first actor serialized)
    auto& sceneValue = data[0];
    auto sceneId = JsonTools::GetGuid(sceneValue, "ID");
    if (!sceneId.IsValid())
    {
        LOG(Error, "Invalid scene id.");
        return true;
    }
    auto modifier = Cache::ISerializeModifier.Get();
    modifier->EngineBuild = engineBuild;

    // Skip is that scene is already loaded
    if (FindScene(sceneId) != nullptr)
    {
        LOG(Info, "Scene {0} is already loaded.", sceneId);
        return false;
    }

    // Create scene actor
    // Note: the first object in the scene file data is a Scene Actor
    auto scene = New<Scene>(ScriptingObjectSpawnParams(sceneId, Scene::TypeInitializer));
    scene->LoadTime = startTime;
    scene->RegisterObject();
    scene->Deserialize(sceneValue, modifier.Value);

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneLoading, scene, sceneId);

    // Maps the loaded actor object to the json data with the RemovedObjects array (used to skip restoring objects removed per prefab instance)
    SceneObjectsFactory::ActorToRemovedObjectsDataLookup actorToRemovedObjectsData;

    // Loaded scene objects list
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->Resize(objectsCount);
    sceneObjects->At(0) = scene;

    {
        PROFILE_CPU_NAMED("Spawn");

        // Spawn all scene objects
        for (int32 i = 1; i < objectsCount; i++) // start from 1. at index [0] was scene
        {
            auto& objData = data[i];
            auto obj = SceneObjectsFactory::Spawn(objData, modifier.Value);
            sceneObjects->At(i) = obj;
            if (obj)
            {
                // Register object so it can be later referenced during deserialization (FindObject will work to link references between objects)
                obj->RegisterObject();

                // Special case for actors
                if (auto actor = dynamic_cast<Actor*>(obj))
                {
                    // Check for RemovedObjects listing
                    const auto removedObjects = SERIALIZE_FIND_MEMBER(objData, "RemovedObjects");
                    if (removedObjects != objData.MemberEnd())
                    {
                        actorToRemovedObjectsData.Add(actor, &removedObjects->value);
                    }
                }
            }
        }
    }

    // /\ all above this has to be done on an any thread
    // \/ all below this has to be done on multiple threads at once

    {
        PROFILE_CPU_NAMED("Deserialize");

        // TODO: at this point we would probably spawn a few thread pool tasks which will load deserialize scene object but only if scene is big enough

        // Load all scene objects
        Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
        for (int32 i = 1; i < objectsCount; i++) // start from 1. at index [0] was scene
        {
            auto& objData = data[i];
            auto obj = sceneObjects->At(i);
            if (obj)
            {
                SceneObjectsFactory::Deserialize(obj, objData, modifier.Value);
            }
            else
            {
                SceneObjectsFactory::HandleObjectDeserializationError(objData);

                // Try to log some useful info about missing object (eg. it's parent name for faster fixing)
                const auto parentIdMember = objData.FindMember("ParentID");
                if (parentIdMember != objData.MemberEnd() && parentIdMember->value.IsString())
                {
                    Guid parentId = JsonTools::GetGuid(parentIdMember->value);
                    Actor* parent = Scripting::FindObject<Actor>(parentId);
                    if (parent)
                    {
                        LOG(Warning, "Parent actor of the missing object: {0}", parent->GetName());
                    }
                }
            }
        }
        Scripting::ObjectsLookupIdMapping.Set(nullptr);
    }

    // /\ all above this has to be done on multiple threads at once
    // \/ all below this has to be done on an any thread

    // Call post load event to connect all scene actors
    {
        PROFILE_CPU_NAMED("Post Load");

        for (int32 i = 0; i < objectsCount; i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            if (obj)
                obj->PostLoad();
        }
    }

    // Synchronize prefab instances (prefab may have new objects added or some removed so deserialized instances need to synchronize with it)
    // TODO: resave and force sync scenes during game cooking so this step could be skipped in game
    Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
    SceneObjectsFactory::SynchronizePrefabInstances(*sceneObjects.Value, actorToRemovedObjectsData, modifier.Value);
    Scripting::ObjectsLookupIdMapping.Set(nullptr);

    // Delete objects without parent
    for (int32 i = 1; i < objectsCount; i++)
    {
        SceneObject* obj = sceneObjects->At(i);
        if (obj && obj->GetParent() == nullptr)
        {
            sceneObjects->At(i) = nullptr;
            LOG(Warning, "Scene object {0} {1} has missing parent object after load. Removing it.", obj->GetID(), obj->ToString());
            obj->DeleteObject();
        }
    }

    // Cache transformations
    {
        PROFILE_CPU_NAMED("Cache Transform");

        scene->OnTransformChanged();
    }

    // /\ all above this has to be done on an any thread
    // \/ all below this has to be done on a main thread

    // Link scene and call init
    {
        PROFILE_CPU_NAMED("BeginPlay");

        Scenes.Add(scene);

        if (autoInitialize)
        {
            SceneBeginData beginData;
            scene->BeginPlay(&beginData);
            beginData.OnDone();
        }
        else
        {
            // Send warning to log just in case (easier to track if scene will be loaded without init)
            // Why? Because we can load collection of scenes and then call for all of them init so references between objects in a different scenes will be resolved without leaks.
            //LOG(Warning, "Scene \'{0}:{1}\', has been loaded but not initialized. Remember to call OnBeginPlay().", scene->GetName(), scene->GetID());
        }
    }

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneLoaded, scene, sceneId);

    LOG(Info, "Scene loaded in {0} ms", (int32)(DateTime::NowUTC() - startTime).GetTotalMilliseconds());

    if (outScene)
        *outScene = scene;

    return false;
}

bool LevelImpl::saveScene(Scene* scene)
{
#if USE_EDITOR
    const auto path = scene->GetPath();
    if (path.IsEmpty())
    {
        LOG(Error, "Missing scene path.");
        return true;
    }

    return saveScene(scene, path);
#else
    LOG(Error, "Cannot save data to the cooked content.");
    return false;
#endif
}

bool LevelImpl::saveScene(Scene* scene, const String& path)
{
    ASSERT(scene);
    auto sceneId = scene->GetID();

    LOG(Info, "Saving scene {0} to \'{1}\'", scene->GetName(), path);
    const DateTime startTime = DateTime::NowUTC();
    scene->SaveTime = startTime;

    // Serialize to json
    rapidjson_flax::StringBuffer buffer;
    if (saveScene(scene, buffer, true) && buffer.GetSize() > 0)
    {
        CallSceneEvent(SceneEventType::OnSceneSaveError, scene, sceneId);
        return true;
    }

    // Save json to file
    if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
    {
        LOG(Error, "Cannot save scene file");
        CallSceneEvent(SceneEventType::OnSceneSaveError, scene, sceneId);
        return true;
    }

    LOG(Info, "Scene saved! Time {0} ms", Math::CeilToInt((float)(DateTime::NowUTC() - startTime).GetTotalMilliseconds()));

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneSaved, scene, sceneId);

    return false;
}

bool LevelImpl::saveScene(Scene* scene, rapidjson_flax::StringBuffer& outBuffer, bool prettyJson)
{
    if (prettyJson)
    {
        PrettyJsonWriter writerObj(outBuffer);
        return saveScene(scene, outBuffer, writerObj);
    }
    else
    {
        CompactJsonWriter writerObj(outBuffer);
        return saveScene(scene, outBuffer, writerObj);
    }
}

bool LevelImpl::saveScene(Scene* scene, rapidjson_flax::StringBuffer& outBuffer, JsonWriter& writer)
{
    PROFILE_CPU_NAMED("Level.SaveScene");

    ASSERT(scene);
    const auto sceneId = scene->GetID();

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneSaving, scene, sceneId);

    // Get all objects in the scene
    Array<SceneObject*> allObjects;
    SceneQuery::GetAllSerializableSceneObjects(scene, allObjects);

    // Serialize to json
    writer.StartObject();
    {
        PROFILE_CPU_NAMED("Serialize");

        // Json resource header
        writer.JKEY("ID");
        writer.Guid(sceneId);
        writer.JKEY("TypeName");
        writer.String("FlaxEngine.SceneAsset");
        writer.JKEY("EngineBuild");
        writer.Int(FLAXENGINE_VERSION_BUILD);

        // Json resource data
        writer.JKEY("Data");
        writer.StartArray();
        for (int32 i = 0; i < allObjects.Count(); i++)
        {
            writer.SceneObject(allObjects[i]);
        }
        writer.EndArray();
    }
    writer.EndObject();

    return false;
}

bool Level::SaveScene(Scene* scene, bool prettyJson)
{
    ScopeLock lock(_sceneActionsLocker);
    return SaveSceneAction(scene, prettyJson).Do();
}

bool Level::SaveSceneToBytes(Scene* scene, rapidjson_flax::StringBuffer& outData, bool prettyJson)
{
    ASSERT(scene);
    ScopeLock lock(_sceneActionsLocker);

    LOG(Info, "Saving scene {0} to bytes", scene->GetName());
    const DateTime startTime = DateTime::NowUTC();
    scene->SaveTime = startTime;

    // Serialize to json
    if (saveScene(scene, outData, prettyJson))
    {
        CallSceneEvent(SceneEventType::OnSceneSaveError, scene, scene->GetID());
        return true;
    }

    // Info
    LOG(Info, "Scene saved! Time {0} ms", Math::CeilToInt(static_cast<float>((DateTime::NowUTC()- startTime).GetTotalMilliseconds())));

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneSaved, scene, scene->GetID());

    return false;
}

Array<byte> Level::SaveSceneToBytes(Scene* scene, bool prettyJson)
{
    Array<byte> data;
    rapidjson_flax::StringBuffer sceneData;
    if (!SaveSceneToBytes(scene, sceneData, prettyJson))
    {
        data.Set((const byte*)sceneData.GetString(), (int32)sceneData.GetSize() * sizeof(rapidjson_flax::StringBuffer::Ch));
    }
    return data;
}

void Level::SaveSceneAsync(Scene* scene)
{
    ScopeLock lock(_sceneActionsLocker);
    _sceneActions.Enqueue(New<SaveSceneAction>(scene));
}

bool Level::SaveAllScenes()
{
    ScopeLock lock(_sceneActionsLocker);
    for (int32 i = 0; i < Scenes.Count(); i++)
    {
        if (SaveSceneAction(Scenes[i]).Do())
            return true;
    }
    return false;
}

void Level::SaveAllScenesAsync()
{
    ScopeLock lock(_sceneActionsLocker);
    for (int32 i = 0; i < Scenes.Count(); i++)
        _sceneActions.Enqueue(New<SaveSceneAction>(Scenes[i]));
}

bool Level::LoadScene(const Guid& id, bool autoInitialize)
{
    // Check ID
    if (!id.IsValid())
    {
        Log::ArgumentException();
        return true;
    }

    // Skip is that scene is already loaded
    if (FindScene(id) != nullptr)
    {
        LOG(Info, "Scene {0} is already loaded.", id);
        return false;
    }

    // Now to deserialize scene in a proper way we need to load scripting
    if (!Scripting::IsEveryAssemblyLoaded())
    {
#if USE_EDITOR
        LOG(Error, "Scripts must be compiled without any errors in order to load a scene. Please fix it.");
#else
        LOG(Warning, "Scripts must be compiled without any errors in order to load a scene.");
#endif
        return true;
    }

    // Preload scene asset
    const auto sceneAsset = Content::LoadAsync<JsonAsset>(id);
    if (sceneAsset == nullptr)
    {
        LOG(Error, "Cannot load scene asset.");
        return true;
    }

    // Load scene
    if (loadScene(sceneAsset, autoInitialize))
    {
        LOG(Error, "Failed to deserialize scene {0}", id);
        CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, id);
        return true;
    }
    return false;
}

Scene* Level::LoadSceneFromBytes(const BytesContainer& data, bool autoInitialize)
{
    Scene* scene = nullptr;
    if (loadScene(data, autoInitialize, &scene))
    {
        LOG(Error, "Failed to deserialize scene from bytes");
        CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, Guid::Empty);
    }
    return scene;
}

bool Level::LoadSceneAsync(const Guid& id)
{
    // Check ID
    if (!id.IsValid())
    {
        Log::ArgumentException();
        return true;
    }

    // Preload scene asset
    const auto sceneAsset = Content::LoadAsync<JsonAsset>(id);
    if (sceneAsset == nullptr)
    {
        LOG(Error, "Cannot load scene asset.");
        return true;
    }

    ScopeLock lock(_sceneActionsLocker);
    _sceneActions.Enqueue(New<LoadSceneAction>(id, sceneAsset, true));

    return false;
}

bool Level::UnloadScene(Scene* scene)
{
    return unloadScene(scene);
}

void Level::UnloadSceneAsync(Scene* scene)
{
    ScopeLock lock(_sceneActionsLocker);
    _sceneActions.Enqueue(New<UnloadSceneAction>(scene));
}

bool Level::UnloadAllScenes()
{
    ScopeLock lock(_sceneActionsLocker);
    return unloadScenes();
}

void Level::UnloadAllScenesAsync()
{
    ScopeLock lock(_sceneActionsLocker);
    _sceneActions.Enqueue(New<UnloadScenesAction>());
}

#if USE_EDITOR

void Level::ReloadScriptsAsync()
{
    ScopeLock lock(_sceneActionsLocker);
    _sceneActions.Enqueue(New<ReloadScriptsAction>());
}

#endif

Actor* Level::FindActor(const Guid& id)
{
    return Scripting::FindObject<Actor>(id);
}

Actor* Level::FindActor(const StringView& name)
{
    Actor* result = nullptr;

    ScopeLock lock(ScenesLock);

    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
    {
        result = Scenes[i]->FindActor(name);
    }

    return result;
}

Actor* Level::FindActor(const MClass* type)
{
    CHECK_RETURN(type, nullptr);
    Actor* result = nullptr;
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
        result = Scenes[i]->FindActor(type);
    return result;
}

Script* Level::FindScript(const MClass* type)
{
    CHECK_RETURN(type, nullptr);
    Script* result = nullptr;
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
        result = Scenes[i]->FindScript(type);
    return result;
}

namespace
{
    void GetActors(const MClass* type, Actor* actor, Array<Actor*>& result)
    {
        if (actor->GetClass()->IsSubClassOf(type))
            result.Add(actor);
        for (auto child : actor->Children)
            GetActors(type, child, result);
    }

    void GetScripts(const MClass* type, Actor* actor, Array<Script*>& result)
    {
        for (auto script : actor->Scripts)
            if (script->GetClass()->IsSubClassOf(type))
                result.Add(script);
        for (auto child : actor->Children)
            GetScripts(type, child, result);
    }
}

Array<Actor*> Level::GetActors(const MClass* type)
{
    Array<Actor*> result;
    CHECK_RETURN(type, result);
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; i < Scenes.Count(); i++)
        ::GetActors(type, Scenes[i], result);
    return result;
}

Array<Script*> Level::GetScripts(const MClass* type)
{
    Array<Script*> result;
    CHECK_RETURN(type, result);
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; i < Scenes.Count(); i++)
        ::GetScripts(type, Scenes[i], result);
    return result;
}

Scene* Level::FindScene(const Guid& id)
{
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; i < Scenes.Count(); i++)
        if (Scenes[i]->GetID() == id)
            return Scenes[i];
    return nullptr;
}

void Level::GetScenes(Array<Scene*>& scenes)
{
    ScopeLock lock(ScenesLock);

    scenes = Scenes;
}

void Level::GetScenes(Array<Actor*>& scenes)
{
    ScopeLock lock(ScenesLock);

    scenes.Clear();
    scenes.EnsureCapacity(Scenes.Count());

    for (int32 i = 0; i < Scenes.Count(); i++)
        scenes.Add(Scenes[i]);
}

void Level::GetScenes(Array<Guid>& scenes)
{
    ScopeLock lock(ScenesLock);

    scenes.Clear();
    scenes.EnsureCapacity(Scenes.Count());

    for (int32 i = 0; i < Scenes.Count(); i++)
        scenes.Add(Scenes[i]->GetID());
}

void FillTree(Actor* node, Array<Actor*>& result)
{
    result.Add(node->Children);
    for (int i = 0; i < node->Children.Count(); i++)
    {
        FillTree(node->Children[i], result);
    }
}

void Level::ConstructSolidActorsTreeList(const Array<Actor*>& input, Array<Actor*>& output)
{
    for (int32 i = 0; i < input.Count(); i++)
    {
        auto target = input[i];

        // Check if has been already added
        if (output.Contains(target))
            continue;

        // Add whole child tree to the results
        output.Add(target);
        FillTree(target, output);
    }
}

void Level::ConstructParentActorsTreeList(const Array<Actor*>& input, Array<Actor*>& output)
{
    // Build solid part of the tree
    Array<Actor*> fullTree;
    ConstructSolidActorsTreeList(input, fullTree);

    for (int32 i = 0; i < input.Count(); i++)
    {
        Actor* target = input[i];

        // If there is no target node parent in the solid tree list,
        // then it means it's a local root node and can be added to the results.
        if (!fullTree.Contains(target->GetParent()))
            output.Add(target);
    }
}
