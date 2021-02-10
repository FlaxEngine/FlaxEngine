// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Scene.h"
#include "Engine/Level/Level.h"
#include "Engine/Content/AssetInfo.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Factories/JsonAssetFactory.h"
#include "Engine/Physics/Colliders/MeshCollider.h"
#include "Engine/Level/Actors/StaticModel.h"
#include "Engine/Level/ActorsCache.h"
#include "Engine/Navigation/NavigationSettings.h"
#include "Engine/Navigation/NavMeshBoundsVolume.h"
#include "Engine/Navigation/NavMesh.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Serialization/Serialization.h"

REGISTER_JSON_ASSET(SceneAsset, "FlaxEngine.SceneAsset");

SceneAsset::SceneAsset(const SpawnParams& params, const AssetInfo* info)
    : JsonAsset(params, info)
{
}

#define CSG_COLLIDER_NAME TEXT("CSG.Collider")
#define CSG_MODEL_NAME TEXT("CSG.Model")

Scene::Scene(const SpawnParams& params)
    : Actor(params)
    , Rendering(this)
    , Ticking(this)
    , LightmapsData(this)
    , CSGData(this)
{
    // Default name
    _name = TEXT("Scene");

    // Link events
    CSGData.CollisionData.Changed.Bind<Scene, &Scene::OnCsgCollisionDataChanged>(this);
    CSGData.Model.Changed.Bind<Scene, &Scene::OnCsgModelChanged>(this);
#if COMPILE_WITH_CSG_BUILDER
    CSGData.PostCSGBuild.Bind<Scene, &Scene::OnCSGBuildEnd>(this);
#endif
}

Scene::~Scene()
{
}

LightmapSettings Scene::GetLightmapSettings() const
{
    return Info.LightmapSettings;
}

void Scene::SetLightmapSettings(const LightmapSettings& value)
{
    Info.LightmapSettings = value;
}

BoundingBox Scene::GetNavigationBounds()
{
    if (NavigationVolumes.IsEmpty())
        return BoundingBox::Empty;
    PROFILE_CPU_NAMED("GetNavigationBounds");
    auto box = NavigationVolumes[0]->GetBox();
    for (int32 i = 1; i < NavigationVolumes.Count(); i++)
        BoundingBox::Merge(box, NavigationVolumes[i]->GetBox(), box);
    return box;
}

NavMeshBoundsVolume* Scene::FindNavigationBoundsOverlap(const BoundingBox& bounds)
{
    NavMeshBoundsVolume* result = nullptr;
    for (int32 i = 0; i < NavigationVolumes.Count(); i++)
    {
        if (NavigationVolumes[i]->GetBox().Intersects(bounds))
        {
            result = NavigationVolumes[i];
            break;
        }
    }
    return result;
}

void Scene::ClearLightmaps()
{
    LightmapsData.ClearLightmaps();
}

void Scene::BuildCSG(float timeoutMs)
{
    CSGData.BuildCSG(timeoutMs);
}

#if USE_EDITOR

String Scene::GetPath() const
{
    AssetInfo info;
    if (Content::GetAssetInfo(GetID(), info))
    {
        return info.Path;
    }

    return String::Empty;
}

String Scene::GetFilename() const
{
    return StringUtils::GetFileNameWithoutExtension(GetPath());
}

String Scene::GetDataFolderPath() const
{
    return Globals::ProjectContentFolder / TEXT("SceneData") / GetFilename();
}

#endif

MeshCollider* Scene::TryGetCsgCollider()
{
    MeshCollider* result = nullptr;
    for (int32 i = 0; i < Children.Count(); i++)
    {
        const auto collider = dynamic_cast<MeshCollider*>(Children[i]);
        if (collider && collider->GetName() == CSG_COLLIDER_NAME)
        {
            result = collider;
            break;
        }
    }
    return result;
}

StaticModel* Scene::TryGetCsgModel()
{
    StaticModel* result = nullptr;
    for (int32 i = 0; i < Children.Count(); i++)
    {
        const auto model = dynamic_cast<StaticModel*>(Children[i]);
        if (model && model->GetName() == CSG_MODEL_NAME)
        {
            result = model;
            break;
        }
    }
    return result;
}

void Scene::CreateCsgCollider()
{
    // Create collider
    auto result = New<MeshCollider>();
    result->SetStaticFlags(StaticFlags::FullyStatic);
    result->SetName(CSG_COLLIDER_NAME);
    result->CollisionData = CSGData.CollisionData;
    result->HideFlags |= HideFlags::DontSelect;

    // Link it
    if (IsDuringPlay())
    {
        result->SetParent(this, false);
    }
    else
    {
        result->_parent = this;
        result->_scene = this;
        Children.Add(result);
        result->CreateManaged();
    }
}

void Scene::CreateCsgModel()
{
    // Create model
    auto result = New<StaticModel>();
    result->SetStaticFlags(StaticFlags::FullyStatic);
    result->SetName(CSG_MODEL_NAME);
    result->Model = CSGData.Model;
    result->HideFlags |= HideFlags::DontSelect;

    // Link it
    if (IsDuringPlay())
    {
        result->SetParent(this, false);
    }
    else
    {
        result->_parent = this;
        result->_scene = this;
        Children.Add(result);
        result->CreateManaged();
    }
}

void Scene::OnCsgCollisionDataChanged()
{
    // Use it only in play mode
    if (!IsDuringPlay())
        return;

    // Try to find CSG collider
    auto collider = TryGetCsgCollider();
    if (collider)
    {
        // Update the collision asset
        collider->CollisionData = CSGData.CollisionData;
    }
    else if (CSGData.CollisionData)
    {
        // Create collider
        CreateCsgCollider();
    }
}

void Scene::OnCsgModelChanged()
{
    // Use it only in play mode
    if (!IsDuringPlay())
        return;

    // Try to find CSG model
    auto model = TryGetCsgModel();
    if (model)
    {
        // Update the model asset
        model->Model = CSGData.Model;
    }
    else if (CSGData.Model)
    {
        // Create model
        CreateCsgModel();
    }
}

void Scene::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Scene);

    // Update scene info object
    SaveTime = DateTime::NowUTC();

    LightmapsData.SaveLightmaps(Info.Lightmaps);
    Info.Serialize(stream, other ? &other->Info : nullptr);

    if (CSGData.HasData())
    {
        stream.JKEY("CSG");
        stream.Object(&CSGData, other ? &other->CSGData : nullptr);
    }
}

void Scene::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    Info.Deserialize(stream, modifier);
    LightmapsData.LoadLightmaps(Info.Lightmaps);
    CSGData.DeserializeIfExists(stream, "CSG", modifier);

    // [Deprecated on 13.01.2021, expires on 13.01.2023]
    if (modifier->EngineBuild <= 6215 && NavigationMeshes.IsEmpty())
    {
        const auto e = SERIALIZE_FIND_MEMBER(stream, "NavMesh");
        if (e != stream.MemberEnd())
        {
            // Upgrade from old single hidden navmesh data into NavMesh actors on a scene
            AssetReference<RawDataAsset> dataAsset;
            Serialization::Deserialize(e->value, dataAsset, modifier);
            const auto settings = NavigationSettings::Get();
            if (dataAsset && settings->NavMeshes.HasItems())
            {
                auto navMesh = New<NavMesh>();
                navMesh->SetStaticFlags(StaticFlags::FullyStatic);
                navMesh->SetName(TEXT("NavMesh.") + settings->NavMeshes[0].Name);
                navMesh->DataAsset = dataAsset;
                navMesh->Properties = settings->NavMeshes[0];
                if (IsDuringPlay())
                {
                    navMesh->SetParent(this, false);
                }
                else
                {
                    navMesh->_parent = this;
                    navMesh->_scene = this;
                    Children.Add(navMesh);
                    navMesh->CreateManaged();
                }
            }
        }
    }
}

void Scene::OnDeleteObject()
{
    // Cleanup
    LightmapsData.UnloadLightmaps();
    CSGData.Model = nullptr;
    CSGData.CollisionData = nullptr;

    // Base
    Actor::OnDeleteObject();
}

void Scene::PostLoad()
{
    // Initialize
    _parent = nullptr;
    _scene = this;

    // Base
    Actor::PostLoad();
}

void Scene::PostSpawn()
{
    // Initialize
    _parent = nullptr;
    _scene = this;

    // Base
    Actor::PostSpawn();
}

void Scene::BeginPlay(SceneBeginData* data)
{
    // Base
    Actor::BeginPlay(data);

    // Check if has CSG collision and create collider if need to (before play mode enter)
    if (CSGData.CollisionData)
    {
        const auto collider = TryGetCsgCollider();
        if (collider == nullptr)
            CreateCsgCollider();
    }

    // Check if has CSG model and create model if need to (before play mode enter)
    if (CSGData.Model)
    {
        const auto model = TryGetCsgModel();
        if (model == nullptr)
            CreateCsgModel();
    }
}

void Scene::EndPlay()
{
    // Improve scene cleanup performance by removing all data from scene rendering and ticking containers
    Ticking.Clear();
    Rendering.Clear();

    // Base
    Actor::EndPlay();
}

void Scene::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
