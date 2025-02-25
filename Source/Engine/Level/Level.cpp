// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Level.h"
#include "ActorsCache.h"
#include "LargeWorlds.h"
#include "SceneQuery.h"
#include "SceneObjectsFactory.h"
#include "Scene/Scene.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"
#include "Engine/Core/ObjectsRemovalService.h"
#include "Engine/Core/Config/LayersTagsSettings.h"
#include "Engine/Core/Types/LayersMask.h"
#include "Engine/Core/Types/Stopwatch.h"
#include "Engine/Debug/Exceptions/ArgumentException.h"
#include "Engine/Debug/Exceptions/ArgumentNullException.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/JobSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Engine/Time.h"
#include "Engine/Scripting/ManagedCLR/MAssembly.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
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
#include "Engine/Serialization/JsonSerializer.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#endif

#if USE_LARGE_WORLDS
bool LargeWorlds::Enable = true;
#else
bool LargeWorlds::Enable = false;
#endif

void LargeWorlds::UpdateOrigin(Vector3& origin, const Vector3& position)
{
    if (Enable)
    {
        constexpr Real chunkSizeInv = 1.0 / ChunkSize;
        constexpr Real chunkSizeHalf = ChunkSize * 0.5;
        origin = Vector3(Int3((position - chunkSizeHalf) * chunkSizeInv)) * ChunkSize;
    }
}

bool LayersMask::HasLayer(const StringView& layerName) const
{
    return HasLayer(Level::GetLayerIndex(layerName));
}

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

#if USE_EDITOR

struct ScriptsReloadObject
{
    StringAnsi TypeName;
    ScriptingObject** Object;
    Array<byte> Data;
};

#endif

namespace LevelImpl
{
    Array<SceneAction*> _sceneActions;
    CriticalSection _sceneActionsLocker;
    DateTime _lastSceneLoadTime(0);
#if USE_EDITOR
    Array<ScriptsReloadObject> ScriptsReloadObjects;
#endif

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
        : EngineService(TEXT("Scene Manager"), 200)
    {
    }

    void Update() override;
    void LateUpdate() override;
    void FixedUpdate() override;
    void LateFixedUpdate() override;
    void Dispose() override;
};

LevelService LevelServiceInstanceService;

CriticalSection Level::ScenesLock;
Array<Scene*> Level::Scenes;
bool Level::TickEnabled = true;
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
#if USE_EDITOR
Action Level::ScriptsReloadStart;
Action Level::ScriptsReload;
Action Level::ScriptsReloaded;
Action Level::ScriptsReloadEnd;
#endif
String Level::Layers[32];

bool LevelImpl::spawnActor(Actor* actor, Actor* parent)
{
    if (actor == nullptr)
    {
        Log::ArgumentNullException(TEXT("Cannot spawn null actor."));
        return true;
    }

    if (actor->GetType().ManagedClass->IsAbstract())
    {
        Log::Exception(TEXT("Cannot spawn abstract actor type."));
        return true;
    }

    if (actor->Is<Scene>())
    {
        // Spawn scene
        actor->InitializeHierarchy();
        actor->OnTransformChanged();
        {
            SceneBeginData beginData;
            actor->BeginPlay(&beginData);
            beginData.OnDone();
        }
        CallSceneEvent(SceneEventType::OnSceneLoaded, (Scene*)actor, actor->GetID());
    }
    else
    {
        // Spawn actor
        if (Level::Scenes.IsEmpty())
        {
            Log::InvalidOperationException(TEXT("Cannot spawn actor. No scene loaded."));
            return true;
        }
        if (parent == nullptr)
            parent = Level::Scenes[0];

        actor->SetPhysicsScene(parent->GetPhysicsScene());
        actor->SetParent(parent, true, true);
    }

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

void LayersAndTagsSettings::Apply()
{
    // Note: we cannot remove tags/layers at runtime so this should deserialize them in additive mode
    // Tags/Layers are stored as index in actors so collection change would break the linkage
    for (auto& tag : Tags)
    {
        Tags::Get(tag);
    }
    for (int32 i = 0; i < ARRAY_COUNT(Level::Layers); i++)
    {
        const auto& src = Layers[i];
        auto& dst = Level::Layers[i];
        if (dst.IsEmpty() || !src.IsEmpty())
            dst = src;
    }
}

#define TICK_LEVEL(tickingStage, name) \
    PROFILE_CPU_NAMED(name); \
    ScopeLock lock(Level::ScenesLock); \
    auto& scenes = Level::Scenes; \
    if (!Time::GetGamePaused() && Level::TickEnabled) \
    { \
        for (int32 i = 0; i < scenes.Count(); i++) \
        { \
            if (scenes[i]->GetIsActive()) \
                scenes[i]->Ticking.tickingStage.Tick(); \
        } \
    }
#if USE_EDITOR
#define TICK_LEVEL_EDITOR(tickingStage) \
    else if (!Editor::IsPlayMode) \
    { \
        for (int32 i = 0; i < scenes.Count(); i++) \
        { \
            if (scenes[i]->GetIsActive()) \
                scenes[i]->Ticking.tickingStage.TickExecuteInEditor(); \
        } \
    }
#else
#define TICK_LEVEL_EDITOR(tickingStage)
#endif

void LevelService::Update()
{
    TICK_LEVEL(Update, "Level::Update")
    TICK_LEVEL_EDITOR(Update)
}

void LevelService::LateUpdate()
{
    TICK_LEVEL(LateUpdate, "Level::LateUpdate")
    TICK_LEVEL_EDITOR(LateUpdate)
    flushActions();
}

void LevelService::FixedUpdate()
{
    TICK_LEVEL(FixedUpdate, "Level::FixedUpdate")
    TICK_LEVEL_EDITOR(FixedUpdate)
}

void LevelService::LateFixedUpdate()
{
    TICK_LEVEL(LateFixedUpdate, "Level::LateFixedUpdate")
    TICK_LEVEL_EDITOR(LateFixedUpdate)
}

#undef TICK_LEVEL
#undef TICK_LEVEL_EDITOR

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

void Level::DrawActors(RenderContextBatch& renderContextBatch, byte category)
{
    PROFILE_CPU();

    //ScopeLock lock(ScenesLock);

    for (Scene* scene : Scenes)
    {
        if (scene->IsActiveInHierarchy())
            scene->Rendering.Draw(renderContextBatch, (SceneRendering::DrawCategory)category);
    }
}

void Level::CollectPostFxVolumes(RenderContext& renderContext)
{
    PROFILE_CPU();

    //ScopeLock lock(ScenesLock);

    for (Scene* scene : Scenes)
    {
        if (scene->IsActiveInHierarchy())
            scene->Rendering.CollectPostFxVolumes(renderContext);
    }
}

class LoadSceneAction : public SceneAction
{
public:
    Guid SceneId;
    AssetReference<JsonAsset> SceneAsset;

    LoadSceneAction(const Guid& sceneId, JsonAsset* sceneAsset)
    {
        SceneId = sceneId;
        SceneAsset = sceneAsset;
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
        if (Level::loadScene(SceneAsset))
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
    Guid TargetScene;

    UnloadSceneAction(Scene* scene)
    {
        TargetScene = scene->GetID();
    }

    bool Do() const override
    {
        auto scene = Level::FindScene(TargetScene);
        if (!scene)
            return true;
        return unloadScene(scene);
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

        PROFILE_CPU_NAMED("Level.ReloadScripts");
        LOG(Info, "Scripts reloading start");
        const auto startTime = DateTime::NowUTC();

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
        Level::ScriptsReloaded();

        // Restore objects
        for (auto& e : ScriptsReloadObjects)
        {
            const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(e.TypeName);
            *e.Object = ScriptingObject::NewObject(typeHandle);
            if (!*e.Object)
            {
                LOG(Warning, "Failed to restore hot-reloaded object of type {0}.", String(e.TypeName));
                continue;
            }
            auto* serializable = ScriptingObject::ToInterface<ISerializable>(*e.Object);
            if (serializable && e.Data.HasItems())
            {
                JsonSerializer::LoadFromBytes(serializable, e.Data, FLAXENGINE_VERSION_BUILD);
            }
        }
        ScriptsReloadObjects.Clear();

        // Restore scenes (from memory)
        for (int32 i = 0; i < scenesCount; i++)
        {
            LOG(Info, "Restoring scene {0}", scenes[i].Name);

            // Parse json
            const auto& sceneData = scenes[i].Data;
            ISerializable::SerializeDocument document;
            {
                PROFILE_CPU_NAMED("Json.Parse");
                document.Parse(sceneData.GetString(), sceneData.GetSize());
            }
            if (document.HasParseError())
            {
                LOG(Error, "Failed to deserialize scene {0}. Result: {1}", scenes[i].Name, GetParseError_En(document.GetParseError()));
                return true;
            }

            // Load scene
            if (Level::loadScene(document))
            {
                LOG(Error, "Failed to deserialize scene {0}", scenes[i].Name);
                CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, scenes[i].ID);
                return true;
            }
        }
        scenes.Resize(0);

        // Fire event
        LOG(Info, "Scripts reloading end. Total time: {0}ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
        Level::ScriptsReloadEnd();

        return false;
    }
};

void Level::ScriptsReloadRegisterObject(ScriptingObject*& obj)
{
    if (!obj)
        return;
    auto& e = ScriptsReloadObjects.AddOne();
    e.Object = &obj;
    e.TypeName = obj->GetType().Fullname;
    if (auto* serializable = ScriptingObject::ToInterface<ISerializable>(obj))
        e.Data = JsonSerializer::SaveToBytes(serializable);
    ScriptingObject* o = obj;
    obj = nullptr;
    o->DeleteObjectNow();
}

#endif

class SpawnActorAction : public SceneAction
{
public:
    ScriptingObjectReference<Actor> TargetActor;
    ScriptingObjectReference<Actor> ParentActor;

    SpawnActorAction(Actor* actor, Actor* parent)
        : TargetActor(actor)
        , ParentActor(parent)
    {
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
        : TargetActor(actor)
    {
    }

    bool Do() const override
    {
        return deleteActor(TargetActor);
    }
};

void LevelImpl::CallSceneEvent(SceneEventType eventType, Scene* scene, Guid sceneId)
{
    PROFILE_CPU_NAMED("Level::CallSceneEvent");

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

int32 Level::GetNonEmptyLayerNamesCount()
{
    int32 result = 31;
    while (result >= 0 && Layers[result].IsEmpty())
        result--;
    return result + 1;
}

int32 Level::GetLayerIndex(const StringView& layer)
{
    int32 result = -1;
    for (int32 i = 0; i < 32; i++)
    {
        if (Layers[i] == layer)
        {
            result = i;
            break;
        }
    }
    return result;
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

    // Remove from scenes list
    Level::Scenes.Remove(scene);

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneUnloaded, scene, sceneId);

    // Simple enqueue scene root object to be deleted
    scene->DeleteObject();

    // Force flush deleted objects so we actually delete unloaded scene objects (prevent from reloading their managed objects, etc.)
    ObjectsRemovalService::Flush();

    return false;
}

bool LevelImpl::unloadScenes()
{
    auto scenes = Level::Scenes;
    for (int32 i = scenes.Count() - 1; i >= 0; i--)
    {
        if (unloadScene(scenes[i]))
            return true;
    }
    return false;
}

bool Level::loadScene(JsonAsset* sceneAsset)
{
    // Keep reference to the asset (prevent unloading during action)
    AssetReference<JsonAsset> ref = sceneAsset;
    if (sceneAsset == nullptr || sceneAsset->WaitForLoaded())
    {
        LOG(Error, "Cannot load scene asset.");
        return true;
    }

    return loadScene(*sceneAsset->Data, sceneAsset->DataEngineBuild);
}

bool Level::loadScene(const BytesContainer& sceneData, Scene** outScene)
{
    if (sceneData.IsInvalid())
    {
        LOG(Error, "Missing scene data.");
        return true;
    }

    // Parse scene JSON file
    rapidjson_flax::Document document;
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse(sceneData.Get<char>(), sceneData.Length());
    }
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
        return true;
    }

    ScopeLock lock(ScenesLock);
    return loadScene(document, outScene);
}

bool Level::loadScene(rapidjson_flax::Document& document, Scene** outScene)
{
    auto data = document.FindMember("Data");
    if (data == document.MemberEnd())
    {
        LOG(Error, "Missing Data member.");
        return true;
    }
    const int32 saveEngineBuild = JsonTools::GetInt(document, "EngineBuild", 0);
    return loadScene(data->value, saveEngineBuild, outScene);
}

bool Level::loadScene(rapidjson_flax::Value& data, int32 engineBuild, Scene** outScene)
{
    PROFILE_CPU_NAMED("Level.LoadScene");
    if (outScene)
        *outScene = nullptr;
    LOG(Info, "Loading scene...");
    Stopwatch stopwatch;
    _lastSceneLoadTime = DateTime::Now();

    // Here whole scripting backend should be loaded for current project
    // Later scripts will setup attached scripts and restore initial vars
    if (!Scripting::HasGameModulesLoaded())
    {
        LOG(Error, "Cannot load scene without game modules loaded.");
#if USE_EDITOR
        if (!CommandLine::Options.Headless.IsTrue())
        {
            if (ScriptsBuilder::LastCompilationFailed())
                MessageBox::Show(TEXT("Scripts compilation failed. Cannot load scene without game script modules. Please fix the compilation issues. See logs for more info."), TEXT("Failed to compile scripts"), MessageBoxButtons::OK, MessageBoxIcon::Error);
            else
                MessageBox::Show(TEXT("Failed to load scripts. Cannot load scene without game script modules. See logs for more info."), TEXT("Missing game modules"), MessageBoxButtons::OK, MessageBoxIcon::Error);
        }
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

    // Peek scene node value (it's the first actor serialized)
    auto sceneId = JsonTools::GetGuid(data[0], "ID");
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
    scene->RegisterObject();
    scene->Deserialize(data[0], modifier.Value);

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneLoading, scene, sceneId);

    // Get any injected children of the scene.
    Array<Actor*> injectedSceneChildren = scene->Children;

    // Loaded scene objects list
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    const int32 dataCount = (int32)data.Size();
    sceneObjects->Resize(dataCount);
    sceneObjects->At(0) = scene;

    // Spawn all scene objects
    SceneObjectsFactory::Context context(modifier.Value);
    context.Async = JobSystem::GetThreadsCount() > 1 && dataCount > 10;
    {
        PROFILE_CPU_NAMED("Spawn");
        SceneObject** objects = sceneObjects->Get();
        if (context.Async)
        {
            ScenesLock.Unlock(); // Unlock scenes from Main Thread so Job Threads can use it to safely setup actors hierarchy (see Actor::Deserialize)
            JobSystem::Execute([&](int32 i)
            {
                i++; // Start from 1. at index [0] was scene
                auto& stream = data[i];
                auto obj = SceneObjectsFactory::Spawn(context, stream);
                objects[i] = obj;
                if (obj)
                {
                    obj->RegisterObject();
#if USE_EDITOR
                    // Auto-create C# objects for all actors in Editor during scene load when running in async (so main thread already has all of them)
                    obj->CreateManaged();
#endif
                }
                else
                    SceneObjectsFactory::HandleObjectDeserializationError(stream);
            }, dataCount - 1);
            ScenesLock.Lock();
        }
        else
        {
            for (int32 i = 1; i < dataCount; i++) // start from 1. at index [0] was scene
            {
                auto& stream = data[i];
                auto obj = SceneObjectsFactory::Spawn(context, stream);
                sceneObjects->At(i) = obj;
                if (obj)
                    obj->RegisterObject();
                else
                    SceneObjectsFactory::HandleObjectDeserializationError(stream);
            }
        }
    }

    // Capture prefab instances in a scene to restore any missing objects (eg. newly added objects to prefab that are missing in scene file)
    SceneObjectsFactory::PrefabSyncData prefabSyncData(*sceneObjects.Value, data, modifier.Value);
    SceneObjectsFactory::SetupPrefabInstances(context, prefabSyncData);
    // TODO: resave and force sync scenes during game cooking so this step could be skipped in game
    SceneObjectsFactory::SynchronizeNewPrefabInstances(context, prefabSyncData);

    // /\ all above this has to be done on an any thread
    // \/ all below this has to be done on multiple threads at once

    // Load all scene objects
    {
        PROFILE_CPU_NAMED("Deserialize");
        SceneObject** objects = sceneObjects->Get();
        bool wasAsync = context.Async;
        context.Async = false; // TODO: before doing full async for scene objects fix:
        // TODO: - fix Actor's Scripts and Children order when loading objects data out of order via async jobs
        // TODO: - add _loadNoAsync flag to SceneObject or Actor to handle non-async loading for those types (eg. UIControl/UICanvas)
        if (context.Async)
        {
            ScenesLock.Unlock(); // Unlock scenes from Main Thread so Job Threads can use it to safely setup actors hierarchy (see Actor::Deserialize)
            JobSystem::Execute([&](int32 i)
            {
                i++; // Start from 1. at index [0] was scene
                auto obj = objects[i];
                if (obj)
                {
                    auto& idMapping = Scripting::ObjectsLookupIdMapping.Get();
                    idMapping = &context.GetModifier()->IdsMapping;
                    SceneObjectsFactory::Deserialize(context, obj, data[i]);
                    idMapping = nullptr;
                }
            }, dataCount - 1);
            ScenesLock.Lock();
        }
        else
        {
            Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
            for (int32 i = 1; i < dataCount; i++) // start from 1. at index [0] was scene
            {
                auto& objData = data[i];
                auto obj = objects[i];
                if (obj)
                    SceneObjectsFactory::Deserialize(context, obj, objData);
            }
            Scripting::ObjectsLookupIdMapping.Set(nullptr);
        }
        context.Async = wasAsync;
    }

    // /\ all above this has to be done on multiple threads at once
    // \/ all below this has to be done on an any thread

    // Add injected children of scene (via OnSceneLoading) into sceneObjects to be initialized
    for (auto child : injectedSceneChildren)
    {
        Array<SceneObject*> injectedSceneObjects;
        injectedSceneObjects.Add(child);
        SceneQuery::GetAllSceneObjects(child, injectedSceneObjects);
        for (auto o : injectedSceneObjects)
        {
            if (!o->IsRegistered())
                o->RegisterObject();
            sceneObjects->Add(o);
        }
    }

    // Synchronize prefab instances (prefab may have objects removed or reordered so deserialized instances need to synchronize with it)
    // TODO: resave and force sync scenes during game cooking so this step could be skipped in game
    SceneObjectsFactory::SynchronizePrefabInstances(context, prefabSyncData);

    // Cache transformations
    {
        PROFILE_CPU_NAMED("Cache Transform");

        scene->OnTransformChanged();
    }

    // Initialize scene objects
    {
        PROFILE_CPU_NAMED("Initialize");

        SceneObject** objects = sceneObjects->Get();
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = objects[i];
            if (obj)
            {
                obj->Initialize();

                // Delete objects without parent
                if (i != 0 && obj->GetParent() == nullptr)
                {
                    LOG(Warning, "Scene object {0} {1} has missing parent object after load. Removing it.", obj->GetID(), obj->ToString());
                    obj->DeleteObject();
                }
            }
        }
        prefabSyncData.InitNewObjects();
    }

    // /\ all above this has to be done on an any thread
    // \/ all below this has to be done on a main thread

    // Link scene and call init
    {
        PROFILE_CPU_NAMED("BeginPlay");

        ScopeLock lock(ScenesLock);
        Scenes.Add(scene);
        SceneBeginData beginData;
        scene->BeginPlay(&beginData);
        beginData.OnDone();
    }

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneLoaded, scene, sceneId);

    stopwatch.Stop();
    LOG(Info, "Scene loaded in {0}ms", stopwatch.GetMilliseconds());
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
    ASSERT(scene && EnumHasNoneFlags(scene->Flags, ObjectFlags::WasMarkedToDelete));
    auto sceneId = scene->GetID();

    LOG(Info, "Saving scene {0} to \'{1}\'", scene->GetName(), path);
    Stopwatch stopwatch;

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

    stopwatch.Stop();
    LOG(Info, "Scene saved! Time {0}ms", stopwatch.GetMilliseconds());

#if USE_EDITOR
    // Reload asset at the target location if is loaded
    Asset* asset = Content::GetAsset(sceneId);
    if (!asset)
        asset = Content::GetAsset(path);
    if (asset)
        asset->Reload();
#endif

    // Fire event
    CallSceneEvent(SceneEventType::OnSceneSaved, scene, sceneId);

    return false;
}

bool LevelImpl::saveScene(Scene* scene, rapidjson_flax::StringBuffer& outBuffer, bool prettyJson)
{
    PROFILE_CPU_NAMED("Level.SaveScene");
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
        SceneObject** objects = allObjects.Get();
        for (int32 i = 0; i < allObjects.Count(); i++)
            writer.SceneObject(objects[i]);
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
    Stopwatch stopwatch;
    LOG(Info, "Saving scene {0} to bytes", scene->GetName());

    // Serialize to json
    if (saveScene(scene, outData, prettyJson))
    {
        CallSceneEvent(SceneEventType::OnSceneSaveError, scene, scene->GetID());
        return true;
    }

    stopwatch.Stop();
    LOG(Info, "Scene saved! Time {0}ms", stopwatch.GetMilliseconds());

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

bool Level::LoadScene(const Guid& id)
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
    ScopeLock lock(ScenesLock);
    if (loadScene(sceneAsset))
    {
        LOG(Error, "Failed to deserialize scene {0}", id);
        CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, id);
        return true;
    }
    return false;
}

Scene* Level::LoadSceneFromBytes(const BytesContainer& data)
{
    Scene* scene = nullptr;
    if (loadScene(data, &scene))
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
    _sceneActions.Enqueue(New<LoadSceneAction>(id, sceneAsset));

    return false;
}

bool Level::UnloadScene(Scene* scene)
{
    return unloadScene(scene);
}

void Level::UnloadSceneAsync(Scene* scene)
{
    CHECK(scene);
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
    return Scripting::TryFindObject<Actor>(id);
}

Actor* Level::FindActor(const StringView& name)
{
    Actor* result = nullptr;
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
        result = Scenes[i]->FindActor(name);
    return result;
}

Actor* Level::FindActor(const MClass* type, bool activeOnly)
{
    CHECK_RETURN(type, nullptr);
    Actor* result = nullptr;
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
        result = Scenes[i]->FindActor(type, activeOnly);
    return result;
}

Actor* Level::FindActor(const MClass* type, const StringView& name)
{
    CHECK_RETURN(type, nullptr);
    Actor* result = nullptr;
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
        result = Scenes[i]->FindActor(type, name);
    return result;
}

Actor* FindActorRecursive(Actor* node, const Tag& tag, bool activeOnly)
{
    if (activeOnly && !node->GetIsActive())
        return nullptr;
    if (node->HasTag(tag))
        return node;
    Actor* result = nullptr;
    for (Actor* child : node->Children)
    {
        result = FindActorRecursive(child, tag, activeOnly);
        if (result)
            break;
    }
    return result;
}

Actor* FindActorRecursiveByType(Actor* node, const MClass* type, const Tag& tag, bool activeOnly)
{
    CHECK_RETURN(type, nullptr);
    if (activeOnly && !node->GetIsActive())
        return nullptr;
    if (node->HasTag(tag) && (node->GetClass()->IsSubClassOf(type) || node->GetClass()->HasInterface(type)))
        return node;
    Actor* result = nullptr;
    for (Actor* child : node->Children)
    {
        result = FindActorRecursiveByType(child, type, tag, activeOnly);
        if (result)
            break;
    }
    return result;
}

void FindActorsRecursive(Actor* node, const Tag& tag, const bool activeOnly, Array<Actor*>& result)
{
    if (activeOnly && !node->GetIsActive())
        return;
    if (node->HasTag(tag))
        result.Add(node);
    for (Actor* child : node->Children)
        FindActorsRecursive(child, tag, activeOnly, result);
}

void FindActorsRecursiveByParentTags(Actor* node, const Array<Tag>& tags, const bool activeOnly, Array<Actor*>& result)
{
    if (activeOnly && !node->GetIsActive())
        return;
    for (Tag tag : tags)
    {
        if (node->HasTag(tag))
        {
            result.Add(node);
            break;
        }
    }
    for (Actor* child : node->Children)
        FindActorsRecursiveByParentTags(child, tags, activeOnly, result);
}

Actor* Level::FindActor(const Tag& tag, bool activeOnly, Actor* root)
{
    PROFILE_CPU();
    if (root)
        return FindActorRecursive(root, tag, activeOnly);
    Actor* result = nullptr;
    for (Scene* scene : Scenes)
    {
        result = FindActorRecursive(scene, tag, activeOnly);
        if (result)
            break;
    }
    return result;
}

Actor* Level::FindActor(const MClass* type, const Tag& tag, bool activeOnly, Actor* root)
{
    CHECK_RETURN(type, nullptr);
    if (root)
        return FindActorRecursiveByType(root, type, tag, activeOnly);
    Actor* result = nullptr;
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; result == nullptr && i < Scenes.Count(); i++)
        result = Scenes[i]->FindActor(type, tag, activeOnly);
    return result;
}

void FindActorRecursive(Actor* node, const Tag& tag, Array<Actor*>& result)
{
    if (node->HasTag(tag))
        result.Add(node);
    for (Actor* child : node->Children)
        FindActorRecursive(child, tag, result);
}

Array<Actor*> Level::FindActors(const Tag& tag, const bool activeOnly, Actor* root)
{
    PROFILE_CPU();
    Array<Actor*> result;
    if (root)
    {
        FindActorsRecursive(root, tag, activeOnly, result);
    }
    else
    {
        ScopeLock lock(ScenesLock);
        for (Scene* scene : Scenes)
            FindActorsRecursive(scene, tag, activeOnly, result);
    }
    return result;
}

Array<Actor*> Level::FindActorsByParentTag(const Tag& parentTag, const bool activeOnly, Actor* root)
{
    PROFILE_CPU();
    Array<Actor*> result;
    const Array<Tag> subTags = Tags::GetSubTags(parentTag);

    if (subTags.Count() == 0)
    {
        return result;
    }
    if (subTags.Count() == 1)
    {
        result = FindActors(subTags[0], activeOnly, root);
        return result;
    }

    if (root)
    {
        FindActorsRecursiveByParentTags(root, subTags, activeOnly, result);
    }
    else
    {
        ScopeLock lock(ScenesLock);
        for (Scene* scene : Scenes)
            FindActorsRecursiveByParentTags(scene, subTags, activeOnly, result);
    }

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
    void GetActors(const MClass* type, bool isInterface, Actor* actor, bool activeOnly, Array<Actor*>& result)
    {
        if (activeOnly && !actor->GetIsActive())
            return;
        if ((!isInterface && actor->GetClass()->IsSubClassOf(type)) ||
            (isInterface && actor->GetClass()->HasInterface(type)))
            result.Add(actor);
        for (auto child : actor->Children)
            GetActors(type, isInterface, child, activeOnly, result);
    }

    void GetScripts(const MClass* type, bool isInterface, Actor* actor, Array<Script*>& result)
    {
        for (auto script : actor->Scripts)
        {
            if ((!isInterface && script->GetClass()->IsSubClassOf(type)) ||
                (isInterface && script->GetClass()->HasInterface(type)))
                result.Add(script);
        }
        for (auto child : actor->Children)
            GetScripts(type, isInterface, child, result);
    }
}

Array<Actor*> Level::GetActors(const MClass* type, bool activeOnly)
{
    Array<Actor*> result;
    CHECK_RETURN(type, result);
    ScopeLock lock(ScenesLock);
    for (int32 i = 0; i < Scenes.Count(); i++)
        ::GetActors(type, type->IsInterface(), Scenes[i], activeOnly, result);
    return result;
}

Array<Script*> Level::GetScripts(const MClass* type, Actor* root)
{
    Array<Script*> result;
    CHECK_RETURN(type, result);
    ScopeLock lock(ScenesLock);
    const bool isInterface = type->IsInterface();
    if (root)
        ::GetScripts(type, isInterface, root, result);
    else for (int32 i = 0; i < Scenes.Count(); i++)
        ::GetScripts(type, isInterface, Scenes[i], result);
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
