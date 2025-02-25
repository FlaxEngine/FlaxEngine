// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Actor.h"
#include "ActorsCache.h"
#include "Level.h"
#include "SceneQuery.h"
#include "SceneObjectsFactory.h"
#include "Scene/Scene.h"
#include "Prefabs/Prefab.h"
#include "Prefabs/PrefabManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"
#include "Engine/Core/Math/Double4x4.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/RenderView.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"

#if USE_EDITOR
#include "Editor/Editor.h"
#define CHECK_EXECUTE_IN_EDITOR if (Editor::IsPlayMode || script->_executeInEditor)
#else
#define CHECK_EXECUTE_IN_EDITOR
#endif

#define ACTOR_ORIENTATION_EPSILON 0.000000001f

// Start loop over actor children/scripts from the beginning to account for any newly added or removed actors.
#define ACTOR_LOOP_START_MODIFIED_HIERARCHY() _isHierarchyDirty = false
#define ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY() if (_isHierarchyDirty) { _isHierarchyDirty = false; i = -1; }

namespace
{
    Actor* GetChildByPrefabObjectId(Actor* a, const Guid& prefabObjectId)
    {
        Actor* result = nullptr;
        for (int32 i = 0; i < a->Children.Count(); i++)
        {
            if (a->Children[i]->GetPrefabObjectID() == prefabObjectId)
            {
                result = a->Children[i];
                break;
            }
        }
        return result;
    }

    Script* GetScriptByPrefabObjectId(Actor* a, const Guid& prefabObjectId)
    {
        Script* result = nullptr;
        for (int32 i = 0; i < a->Scripts.Count(); i++)
        {
            if (a->Scripts[i]->GetPrefabObjectID() == prefabObjectId)
            {
                result = a->Scripts[i];
                break;
            }
        }
        return result;
    }
}

Actor::Actor(const SpawnParams& params)
    : SceneObject(params)
    , _isActive(true)
    , _isActiveInHierarchy(true)
    , _isPrefabRoot(false)
    , _isEnabled(false)
    , _isHierarchyDirty(false)
    , _layer(0)
    , _staticFlags(StaticFlags::FullyStatic)
    , _localTransform(Transform::Identity)
    , _transform(Transform::Identity)
    , _sphere(BoundingSphere::Empty)
    , _box(BoundingBox::Zero)
    , _scene(nullptr)
    , _physicsScene(nullptr)
    , HideFlags(HideFlags::None)
{
    _drawNoCulling = 0;
    _drawCategory = 0;
}

SceneRendering* Actor::GetSceneRendering() const
{
    return &GetScene()->Rendering;
}

void Actor::SetSceneInHierarchy(Scene* scene)
{
    _scene = scene;

    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children.Get()[i]->SetSceneInHierarchy(scene);
    }
}

void Actor::OnEnableInHierarchy()
{
    if (IsActiveInHierarchy() && GetScene() && !_isEnabled)
    {
        OnEnable();

        ACTOR_LOOP_START_MODIFIED_HIERARCHY();
        for (int32 i = 0; i < Children.Count(); i++)
        {
            Children.Get()[i]->OnEnableInHierarchy();
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }
}

void Actor::OnDisableInHierarchy()
{
    if (IsActiveInHierarchy() && GetScene() && _isEnabled)
    {
        ACTOR_LOOP_START_MODIFIED_HIERARCHY();
        for (int32 i = 0; i < Children.Count(); i++)
        {
            Children.Get()[i]->OnDisableInHierarchy();
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }

        OnDisable();
    }
}

bool Actor::IsSubClassOf(const Actor* object, const MClass* klass)
{
    return object->GetClass()->IsSubClassOf(klass);
}

bool Actor::IsSubClassOf(const Script* object, const MClass* klass)
{
    return object->GetClass()->IsSubClassOf(klass);
}

String Actor::ToString() const
{
    auto& type = GetType();
    return String::Format(TEXT("{0} ({1}; {2})"), _name, type.ToString(), _id);
}

void Actor::OnDeleteObject()
{
    // Check if actor is still in game (eg. user deletes actor object via Object.Delete)
    if (IsDuringPlay())
    {
        // Check if parent is still during game (eg. user removes child actor but rest is still in game)
        const bool isParentInPlay = _parent && _parent->IsDuringPlay();
        if (isParentInPlay)
        {
            // Call event on object removed from the game (only from th top object, don't call it for children of the tree)
            Level::callActorEvent(Level::ActorEventType::OnActorDeleted, this, nullptr);
        }

        // Note: endPlay will remove managed instance
        EndPlay();

        if (isParentInPlay)
        {
            // Unlink from the parent
            _parent->Children.RemoveKeepOrder(this);
            _parent->_isHierarchyDirty = true;
            _parent = nullptr;
            _scene = nullptr;
        }
    }
    else if (_parent)
    {
        // Unlink from the parent
        _parent->Children.RemoveKeepOrder(this);
        _parent->_isHierarchyDirty = true;
        _parent = nullptr;
        _scene = nullptr;
    }

    // Ensure to exit gameplay in a valid way
    ASSERT(!IsDuringPlay());
#if BUILD_DEBUG || BUILD_DEVELOPMENT
    ASSERT(!_isEnabled);
#endif

    // Fire event
    Deleted(this);

    // Delete children
#if BUILD_DEBUG
    int32 callsCheck = Children.Count();
#endif
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto e = Children.Get()[i];
        ASSERT(e->_parent == this);
        e->_parent = nullptr;
        e->DeleteObject();
    }
#if BUILD_DEBUG
    ASSERT(callsCheck == Children.Count());
#endif
    Children.Clear();

    // Delete scripts
#if BUILD_DEBUG
    callsCheck = Scripts.Count();
#endif
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto script = Scripts.Get()[i];
        ASSERT(script->_parent == this);
        if (script->_wasAwakeCalled)
        {
            script->_wasAwakeCalled = false;
            CHECK_EXECUTE_IN_EDITOR
            {
                script->OnDestroy();
            }
        }
        script->_parent = nullptr;
        script->DeleteObject();
    }
#if BUILD_DEBUG
    ASSERT(callsCheck == Scripts.Count());
#endif
    Scripts.Clear();

    // Cleanup prefab link
    if (_isPrefabRoot)
    {
        _isPrefabRoot = 0;
#if USE_EDITOR
        ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
        PrefabManager::PrefabsReferences[_prefabID].Remove(this);
#endif
    }
    _prefabID = Guid::Empty;
    _prefabObjectID = Guid::Empty;

    // Base
    SceneObject::OnDeleteObject();
}

const Guid& Actor::GetSceneObjectId() const
{
    return GetID();
}

void Actor::SetParent(Actor* value, bool worldPositionsStays, bool canBreakPrefabLink)
{
    if (_parent == value)
        return;
#if USE_EDITOR || !BUILD_RELEASE
    if (Is<Scene>())
    {
        LOG(Error, "Cannot change parent of the Scene. Use Level to manage scenes.");
        return;
    }
#endif

    // Peek the previous state
    const Transform prevTransform = _transform;
    const bool wasActiveInTree = IsActiveInHierarchy();
    const auto prevParent = _parent;
    const auto prevScene = _scene;
    const auto newScene = value ? value->_scene : nullptr;

    // Detect it actor is not in a game but new parent is already in a game (we should spawn it)
    const bool isBeingSpawned = !IsDuringPlay() && newScene && value->IsDuringPlay();

    // Actors system doesn't support editing scene hierarchy from multiple threads
    if (!IsInMainThread() && (IsDuringPlay() || isBeingSpawned))
    {
        LOG(Error, "Editing scene hierarchy is only allowed on a main thread.");
        return;
    }

    // Handle changing scene (unregister from it)
    const bool isSceneChanging = prevScene != newScene;
    if (prevScene && isSceneChanging && wasActiveInTree)
    {
        OnDisableInHierarchy();
    }

    Level::ScenesLock.Lock();

    // Unlink from the old one
    if (_parent)
    {
        _parent->Children.RemoveKeepOrder(this);
        _parent->_isHierarchyDirty = true;
    }

    // Set value
    _parent = value;

    // Link to the new one
    if (_parent)
    {
        _parent->Children.Add(this);
        _parent->_isHierarchyDirty = true;
    }

    // Sync scene change if need to
    if (isSceneChanging)
    {
        SetSceneInHierarchy(newScene);
    }

    Level::ScenesLock.Unlock();

    // Cache flag
    _isActiveInHierarchy = _isActive && (_parent == nullptr || _parent->IsActiveInHierarchy());

    // Break prefab link for non-root prefab instance objects
    if (HasPrefabLink() && !_isPrefabRoot && IsDuringPlay() && canBreakPrefabLink)
    {
        BreakPrefabLink();
    }

    // Update the transform
    if (worldPositionsStays)
    {
        if (_parent)
            _parent->_transform.WorldToLocal(prevTransform, _localTransform);
        else
            _localTransform = prevTransform;
    }

    // Fire events
    OnParentChanged();
    if (wasActiveInTree != IsActiveInHierarchy())
        OnActiveInTreeChanged();
    OnTransformChanged();
    if (newScene && isSceneChanging && !isBeingSpawned && IsActiveInHierarchy())
    {
        // Handle scene changing c.d. (register to the new one)
        OnEnableInHierarchy();
    }
    //if (_isDuringPlay)
    if (!isBeingSpawned)
        Level::callActorEvent(Level::ActorEventType::OnActorParentChanged, this, prevParent);

    // Spawn
    if (isBeingSpawned)
    {
        ASSERT(_parent != nullptr && GetScene() != nullptr);

        // Fire events
        InitializeHierarchy();
        {
            SceneBeginData beginData;
            BeginPlay(&beginData);
            beginData.OnDone();
        }
        Level::callActorEvent(Level::ActorEventType::OnActorSpawned, this, nullptr);
    }
}

void Actor::SetParent(Actor* value, bool canBreakPrefabLink)
{
    SetParent(value, false, canBreakPrefabLink);
}

int32 Actor::GetOrderInParent() const
{
    return _parent ? _parent->Children.Find((Actor*)this) : INVALID_INDEX;
}

void Actor::SetOrderInParent(int32 index)
{
    if (!_parent)
        return;
    auto& parentChildren = _parent->Children;
    const int32 currentIndex = parentChildren.Find(this);
    ASSERT(currentIndex != INVALID_INDEX);

    // Check if index will change
    if (currentIndex != index)
    {
        parentChildren.RemoveAtKeepOrder(currentIndex);
        if (index < 0 || index >= parentChildren.Count())
        {
            // Append at the end
            parentChildren.Add(this);
        }
        else
        {
            // Change order
            parentChildren.Insert(index, this);
        }
        _parent->_isHierarchyDirty = true;

        // Fire event
        OnOrderInParentChanged();
    }
}

Actor* Actor::GetChild(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < Children.Count(), nullptr);
    return Children[index];
}

Actor* Actor::GetChild(const StringView& name) const
{
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto e = Children.Get()[i];
        if (e->GetName() == name)
            return e;
    }
    return nullptr;
}

Actor* Actor::GetChild(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    if (type->IsInterface())
    {
        for (auto child : Children)
        {
            if (child->GetClass()->HasInterface(type))
                return child;
        }
    }
    else
    {
        for (auto child : Children)
        {
            if (child->GetClass()->IsSubClassOf(type))
                return child;
        }
    }
    return nullptr;
}

Array<Actor*> Actor::GetChildren(const MClass* type) const
{
    Array<Actor*> result;
    if (type->IsInterface())
    {
        for (auto child : Children)
            if (child->GetClass()->HasInterface(type))
                result.Add(child);
    }
    else
    {
        for (auto child : Children)
            if (child->GetClass()->IsSubClassOf(type))
                result.Add(child);
    }
    return result;
}

void Actor::DestroyChildren(float timeLeft)
{
    PROFILE_CPU();
    Array<Actor*> children = Children;
    const bool useGameTime = timeLeft > ZeroTolerance;
    for (Actor* child : children)
    {
        child->SetParent(nullptr, false, false);
        child->DeleteObject(timeLeft, useGameTime);
    }
}

const String& Actor::GetLayerName() const
{
    return Level::Layers[_layer];
}

void Actor::SetLayerName(const StringView& value)
{
    for (int32 i = 0; i < 32; i++)
    {
        if (Level::Layers[i] == value)
        {
            SetLayer(i);
            return;
        }
    }
    LOG(Warning, "Unknown layer name '{0}'", value);
}

void Actor::SetLayerNameRecursive(const StringView& value)
{
    for (int32 i = 0; i < 32; i++)
    {
        if (Level::Layers[i] == value)
        {
            SetLayerRecursive(i);
            return;
        }
    }
    LOG(Warning, "Unknown layer name '{0}'", value);
}

bool Actor::HasTag() const
{
    return Tags.Count() != 0;
}

bool Actor::HasTag(const Tag& tag) const
{
    return Tags.Contains(tag);
}

bool Actor::HasTag(const StringView& tag) const
{
    return Tags.Contains(tag);
}

void Actor::AddTag(const Tag& tag)
{
    Tags.AddUnique(tag);
}

void Actor::AddTagRecursive(const Tag& tag)
{
    for (const auto& child : Children)
        child->AddTagRecursive(tag);
    Tags.AddUnique(tag);
}

void Actor::RemoveTag(const Tag& tag)
{
    Tags.Remove(tag);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

const String& Actor::GetTag() const
{
    return Tags.Count() != 0 ? Tags[0].ToString() : String::Empty;
}

void Actor::SetTag(const StringView& value)
{
    const Tag tag = Tags::Get(value);
    Tags.Set(&tag, 1);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

void Actor::SetLayer(int32 layerIndex)
{
    layerIndex = Math::Clamp(layerIndex, 0, 31);
    if (layerIndex == _layer)
        return;
    _layer = layerIndex;
    OnLayerChanged();
}

void Actor::SetLayerRecursive(int32 layerIndex)
{
    layerIndex = Math::Clamp(layerIndex, 0, 31);
    for (const auto& child : Children)
        child->SetLayerRecursive(layerIndex);
    if (layerIndex == _layer)
        return;
    _layer = layerIndex;
    OnLayerChanged();
}

void Actor::SetName(String&& value)
{
    if (_name == value)
        return;
    _name = MoveTemp(value);
    if (GetScene())
        Level::callActorEvent(Level::ActorEventType::OnActorNameChanged, this, nullptr);
}

void Actor::SetName(const StringView& value)
{
    if (_name == value)
        return;
    _name = value;
    if (GetScene())
        Level::callActorEvent(Level::ActorEventType::OnActorNameChanged, this, nullptr);
}

Script* Actor::GetScript(int32 index) const
{
    CHECK_RETURN(index >= 0 && index < Scripts.Count(), nullptr);
    return Scripts[index];
}

Script* Actor::GetScript(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    if (type->IsInterface())
    {
        for (auto script : Scripts)
        {
            if (script->GetClass()->HasInterface(type))
                return script;
        }
    }
    else
    {
        for (auto script : Scripts)
        {
            if (script->GetClass()->IsSubClassOf(type))
                return script;
        }
    }
    return nullptr;
}

Array<Script*> Actor::GetScripts(const MClass* type) const
{
    Array<Script*> result;
    if (type->IsInterface())
    {
        for (auto script : Scripts)
            if (script->GetClass()->HasInterface(type))
                result.Add(script);
    }
    else
    {
        for (auto script : Scripts)
            if (script->GetClass()->IsSubClassOf(type))
                result.Add(script);
    }
    return result;
}

void Actor::SetIsActive(bool value)
{
    if (value != GetIsActive())
    {
        _isActive = value;
        OnActiveChanged();
    }
}

void Actor::SetStaticFlags(StaticFlags value)
{
    if (_staticFlags == value)
        return;
    _staticFlags = value;
    OnStaticFlagsChanged();
}

void Actor::SetTransform(const Transform& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!(Vector3::NearEqual(_transform.Translation, value.Translation) && Quaternion::NearEqual(_transform.Orientation, value.Orientation, ACTOR_ORIENTATION_EPSILON) && Float3::NearEqual(_transform.Scale, value.Scale)))
    {
        if (_parent)
            _parent->_transform.WorldToLocal(value, _localTransform);
        else
            _localTransform = value;
        OnTransformChanged();
    }
}

void Actor::SetPosition(const Vector3& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Vector3::NearEqual(_transform.Translation, value))
    {
        if (_parent)
            _localTransform.Translation = _parent->_transform.WorldToLocal(value);
        else
            _localTransform.Translation = value;
        OnTransformChanged();
    }
}

void Actor::SetOrientation(const Quaternion& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Quaternion::NearEqual(_transform.Orientation, value, ACTOR_ORIENTATION_EPSILON))
    {
        if (_parent)
            _parent->_transform.WorldToLocal(value, _localTransform.Orientation);
        else
            _localTransform.Orientation = value;
        OnTransformChanged();
    }
}

void Actor::SetScale(const Float3& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Float3::NearEqual(_transform.Scale, value))
    {
        if (_parent)
            Float3::Divide(value, _parent->_transform.Scale, _localTransform.Scale);
        else
            _localTransform.Scale = value;
        OnTransformChanged();
    }
}

Matrix Actor::GetRotation() const
{
    Matrix result;
    Matrix::RotationQuaternion(_transform.Orientation, result);
    return result;
}

void Actor::SetRotation(const Matrix& value)
{
    Quaternion orientation;
    Quaternion::RotationMatrix(value, orientation);
    SetOrientation(orientation);
}

void Actor::SetDirection(const Float3& value)
{
    CHECK(!value.IsNanOrInfinity());
    Quaternion orientation;
    if (Float3::Dot(value, Float3::Up) >= 0.999f)
    {
        Quaternion::RotationAxis(Float3::Left, PI_HALF, orientation);
    }
    else
    {
        const Float3 right = Float3::Cross(value, Float3::Up);
        const Float3 up = Float3::Cross(right, value);
        Quaternion::LookRotation(value, up, orientation);
    }
    SetOrientation(orientation);
}

void Actor::ResetLocalTransform()
{
    SetLocalTransform(Transform::Identity);
}

void Actor::SetLocalTransform(const Transform& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!(Vector3::NearEqual(_localTransform.Translation, value.Translation) && Quaternion::NearEqual(_localTransform.Orientation, value.Orientation, ACTOR_ORIENTATION_EPSILON) && Float3::NearEqual(_localTransform.Scale, value.Scale)))
    {
        _localTransform = value;
        OnTransformChanged();
    }
}

void Actor::SetLocalPosition(const Vector3& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Vector3::NearEqual(_localTransform.Translation, value))
    {
        _localTransform.Translation = value;
        OnTransformChanged();
    }
}

void Actor::SetLocalOrientation(const Quaternion& value)
{
    CHECK(!value.IsNanOrInfinity());
    Quaternion v = value;
    v.Normalize();
    if (!Quaternion::NearEqual(_localTransform.Orientation, v, ACTOR_ORIENTATION_EPSILON))
    {
        _localTransform.Orientation = v;
        OnTransformChanged();
    }
}

void Actor::SetLocalScale(const Float3& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Float3::NearEqual(_localTransform.Scale, value))
    {
        _localTransform.Scale = value;
        OnTransformChanged();
    }
}

void Actor::AddMovement(const Vector3& translation, const Quaternion& rotation)
{
    Transform t;
    t.Translation = _transform.Translation + translation;
    t.Orientation = _transform.Orientation * rotation;
    t.Scale = _transform.Scale;
    SetTransform(t);
}

void Actor::GetWorldToLocalMatrix(Matrix& worldToLocal) const
{
    GetLocalToWorldMatrix(worldToLocal);
    worldToLocal.Invert();
}

void Actor::GetLocalToWorldMatrix(Matrix& localToWorld) const
{
#if 0
    _transform.GetWorld(localToWorld);
#else
    _localTransform.GetWorld(localToWorld);
    if (_parent)
    {
        Matrix parentToWorld;
        _parent->GetLocalToWorldMatrix(parentToWorld);
        localToWorld = localToWorld * parentToWorld;
    }
#endif
}

void Actor::GetWorldToLocalMatrix(Double4x4& worldToLocal) const
{
    GetLocalToWorldMatrix(worldToLocal);
    worldToLocal.Invert();
}

void Actor::GetLocalToWorldMatrix(Double4x4& localToWorld) const
{
#if 0
    _transform.GetWorld(localToWorld);
#else
    _localTransform.GetWorld(localToWorld);
    if (_parent)
    {
        Double4x4 parentToWorld;
        _parent->GetLocalToWorldMatrix(parentToWorld);
        localToWorld = localToWorld * parentToWorld;
    }
#endif
}

void Actor::LinkPrefab(const Guid& prefabId, const Guid& prefabObjectId)
{
    ASSERT(prefabId.IsValid());

#if USE_EDITOR
    if (_isPrefabRoot)
    {
        ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
        PrefabManager::PrefabsReferences[_prefabID].Remove(this);
    }
#endif

    // Link
    _prefabID = prefabId;
    _prefabObjectID = prefabObjectId;
    _isPrefabRoot = 0;

    if (_prefabID.IsValid() && _prefabObjectID.IsValid())
    {
        auto prefab = Content::LoadAsync<Prefab>(_prefabID);
        if (prefab == nullptr || prefab->WaitForLoaded())
        {
            _prefabID = Guid::Empty;
            _prefabObjectID = Guid::Empty;
            LOG(Warning, "Failed to load prefab linked to the actor.");
        }
        else if (prefab->GetRootObjectId() == _prefabObjectID)
        {
            _isPrefabRoot = 1;
#if USE_EDITOR
            ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
            PrefabManager::PrefabsReferences[_prefabID].Add(this);
#endif
        }
    }
}

void Actor::BreakPrefabLink()
{
#if USE_EDITOR
    if (_isPrefabRoot)
    {
        ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
        PrefabManager::PrefabsReferences[_prefabID].Remove(this);
    }
#endif

    // Invalidate link
    _prefabID = Guid::Empty;
    _prefabObjectID = Guid::Empty;
    _isPrefabRoot = 0;

    // Do for scripts
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        Scripts[i]->BreakPrefabLink();
    }

    // Do for children
    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children[i]->BreakPrefabLink();
    }
}

void Actor::Initialize()
{
    CHECK_DEBUG(!IsDuringPlay());

    // Cache
    if (_parent)
        _scene = _parent->GetScene();
    _isActiveInHierarchy = _isActive && (_parent == nullptr || _parent->IsActiveInHierarchy());

    // Use lazy creation for the managed instance, just register the object
    if (!IsRegistered())
        RegisterObject();
}

void Actor::BeginPlay(SceneBeginData* data)
{
    CHECK_DEBUG(!IsDuringPlay());

    // Set flag
    Flags |= ObjectFlags::IsDuringPlay;

    OnBeginPlay();

    // Update scripts
    ACTOR_LOOP_START_MODIFIED_HIERARCHY();
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto e = Scripts.Get()[i];
        if (!e->IsDuringPlay())
        {
            e->BeginPlay(data);
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }

    // Update children
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto e = Children.Get()[i];
        if (!e->IsDuringPlay())
        {
            e->BeginPlay(data);
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }

    // Fire events for scripting
    if (IsActiveInHierarchy() && GetScene() && !_isEnabled)
    {
        OnEnable();
    }
}

void Actor::EndPlay()
{
    CHECK_DEBUG(IsDuringPlay());

    // Fire event for scripting
    if (IsActiveInHierarchy() && GetScene())
    {
        ASSERT(GetScene());
        OnDisable();
    }

    for (auto* script : Scripts)
    {
        if (script->_wasAwakeCalled)
        {
            script->_wasAwakeCalled = false;
            CHECK_EXECUTE_IN_EDITOR
            {
                script->OnDestroy();
            }
        }
    }

    OnEndPlay();

    // Clear flag
    Flags &= ~ObjectFlags::IsDuringPlay;

    // Call event deeper
    ACTOR_LOOP_START_MODIFIED_HIERARCHY();
    for (int32 i = 0; i < Children.Count(); i++)
    {
        auto e = Children.Get()[i];
        if (e->IsDuringPlay())
        {
            e->EndPlay();
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }

    // Inform attached scripts
    ACTOR_LOOP_START_MODIFIED_HIERARCHY();
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto e = Scripts.Get()[i];
        if (e->IsDuringPlay())
        {
            e->EndPlay();
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }

    // Cleanup managed object
    //DestroyManaged();
    if (IsRegistered())
        UnregisterObject();
}

void Actor::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    SceneObject::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Actor);
    const bool isPrefabDiff = other && HasPrefabLink();

    SERIALIZE_BIT_MEMBER(IsActive, _isActive);
    SERIALIZE_MEMBER(Name, _name);
    SERIALIZE_MEMBER(Transform, _localTransform);
    SERIALIZE_MEMBER(StaticFlags, _staticFlags);
    SERIALIZE(HideFlags);
    SERIALIZE_MEMBER(Layer, _layer);
    if (!other || Tags != other->Tags)
    {
        if (Tags.Count() == 1)
        {
            stream.JKEY("Tag");
            stream.String(Tags.Get()->ToString());
        }
        else
        {
            stream.JKEY("Tags");
            stream.StartArray();
            for (auto& tag : Tags)
                stream.String(tag.ToString());
            stream.EndArray();
        }
    }

    if (isPrefabDiff)
    {
        // Prefab object instance may have removed child objects (actors/scripts)
        // Scene deserialization by default adds missing objects to synchronize changes applied to prefab but not applied to scene
        // In order to handle removed objects per instance we need to save the ids of the prefab object ids that are not used by this object

        bool hasRemovedObjects = false;
        for (int32 i = 0; i < other->Children.Count(); i++)
        {
            const Guid prefabObjectId = other->Children[i]->GetPrefabObjectID();
            if (GetChildByPrefabObjectId(this, prefabObjectId) == nullptr)
            {
                if (!hasRemovedObjects)
                {
                    hasRemovedObjects = true;
                    stream.JKEY("RemovedObjects");
                    stream.StartArray();
                }

                stream.Guid(prefabObjectId);
            }
        }
        for (int32 i = 0; i < other->Scripts.Count(); i++)
        {
            const Guid prefabObjectId = other->Scripts[i]->GetPrefabObjectID();
            if (GetScriptByPrefabObjectId(this, prefabObjectId) == nullptr)
            {
                if (!hasRemovedObjects)
                {
                    hasRemovedObjects = true;
                    stream.JKEY("RemovedObjects");
                    stream.StartArray();
                }

                stream.Guid(prefabObjectId);
            }
        }
        if (hasRemovedObjects)
        {
            stream.EndArray();
        }
    }
}

void Actor::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    SceneObject::Deserialize(stream, modifier);

    DESERIALIZE_BIT_MEMBER(IsActive, _isActive);
    DESERIALIZE_MEMBER(StaticFlags, _staticFlags);
    DESERIALIZE(HideFlags);
    DESERIALIZE_MEMBER(Layer, _layer);
    DESERIALIZE_MEMBER(Name, _name);
    DESERIALIZE_MEMBER(Transform, _localTransform);

    {
        const auto member = SERIALIZE_FIND_MEMBER(stream, "ParentID");
        if (member != stream.MemberEnd())
        {
            Guid parentId;
            Serialization::Deserialize(member->value, parentId, modifier);
            const auto parent = Scripting::FindObject<Actor>(parentId);
            if (_parent != parent)
            {
                if (IsDuringPlay())
                {
                    SetParent(parent, false, false);
                }
                else
                {
                    if (_parent)
                        _parent->Children.RemoveKeepOrder(this);
                    _parent = parent;
                    if (_parent)
                        _parent->Children.Add(this);
                    OnParentChanged();
                }
            }
            else if (!parent && parentId.IsValid())
            {
                if (_prefabObjectID.IsValid())
                    LOG(Warning, "Missing parent actor {0} for \'{1}\', prefab object {2}", parentId, ToString(), _prefabObjectID);
                else
                    LOG(Warning, "Missing parent actor {0} for \'{1}\'", parentId, ToString());
            }
        }
    }

    // StaticFlags update - added StaticFlags::Navigation
    // [Deprecated on 17.05.2020, expires on 17.05.2021]
    if (modifier->EngineBuild < 6178 && (int32)_staticFlags == (1 + 2 + 4))
        _staticFlags |= StaticFlags::Navigation;

    // StaticFlags update - added StaticFlags::Shadow
    // [Deprecated on 17.05.2020, expires on 17.05.2021]
    if (modifier->EngineBuild < 6601 && (int32)_staticFlags == (1 + 2 + 4 + 8))
        _staticFlags |= StaticFlags::Shadow;

    const auto tag = stream.FindMember("Tag");
    if (tag != stream.MemberEnd())
    {
        if (tag->value.IsString() && tag->value.GetStringLength())
        {
            Tags.Clear();
            Tags.Add(Tags::Get(tag->value.GetText()));
        }
    }
    else
    {
        const auto tags = stream.FindMember("Tags");
        if (tags != stream.MemberEnd() && tags->value.IsArray())
        {
            Tags.Clear();
            for (rapidjson::SizeType i = 0; i < tags->value.Size(); i++)
            {
                auto& e = tags->value[i];
                if (e.IsString() && e.GetStringLength())
                    Tags.Add(Tags::Get(e.GetText()));
            }
        }
    }

    {
        const auto member = stream.FindMember("PrefabID");
        if (member != stream.MemberEnd())
        {
#if USE_EDITOR
            if (_isPrefabRoot)
            {
                ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
                PrefabManager::PrefabsReferences[_prefabID].Remove(this);
            }
#endif

            Serialization::Deserialize(member->value, _prefabID, modifier);
            _isPrefabRoot = 0;

            auto prefab = Content::LoadAsync<Prefab>(_prefabID);
            if (prefab == nullptr || prefab->WaitForLoaded())
            {
                _prefabID = Guid::Empty;
                _prefabObjectID = Guid::Empty;
                LOG(Warning, "Failed to load prefab linked to the actor on load.");
            }
            else if (prefab->GetRootObjectId() == _prefabObjectID)
            {
                _isPrefabRoot = 1;
#if USE_EDITOR
                ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
                PrefabManager::PrefabsReferences[_prefabID].Add(this);
#endif
            }
        }
    }
}

void Actor::OnEnable()
{
    CHECK_DEBUG(!_isEnabled);
    _isEnabled = true;

    ACTOR_LOOP_START_MODIFIED_HIERARCHY();
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto script = Scripts[i];
        if (script->GetEnabled() && !script->_wasStartCalled)
        {
            script->Start();
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }

    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto script = Scripts[i];
        if (script->GetEnabled() && !script->_wasEnableCalled)
        {
            script->Enable();
            ACTOR_LOOP_CHECK_MODIFIED_HIERARCHY();
        }
    }
}

void Actor::OnDisable()
{
    CHECK_DEBUG(_isEnabled);
    _isEnabled = false;

    for (int32 i = Scripts.Count() - 1; i >= 0; i--)
    {
        auto script = Scripts[i];
        if (script->GetEnabled() && script->_wasEnableCalled)
            script->Disable();
    }
}

void Actor::OnParentChanged()
{
}

void Actor::OnTransformChanged()
{
    ASSERT_LOW_LAYER(!_localTransform.IsNanOrInfinity());

    if (_parent)
    {
        _parent->_transform.LocalToWorld(_localTransform, _transform);
    }
    else
    {
        _transform = _localTransform;
    }

    for (auto child : Children)
    {
        child->OnTransformChanged();
    }
}

void Actor::OnActiveChanged()
{
    const bool wasActiveInTree = IsActiveInHierarchy();
    _isActiveInHierarchy = _isActive && (_parent == nullptr || _parent->IsActiveInHierarchy());
    if (wasActiveInTree != IsActiveInHierarchy())
        OnActiveInTreeChanged();

    //if (GetScene())
    Level::callActorEvent(Level::ActorEventType::OnActorActiveChanged, this, nullptr);
}

void Actor::OnActiveInTreeChanged()
{
    if (IsDuringPlay() && GetScene())
    {
        if (IsActiveInHierarchy())
        {
            if (!_isEnabled)
                OnEnable();
        }
        else if (_isEnabled)
        {
            OnDisable();
        }
    }

    for (auto child : Children)
    {
        if (child->GetIsActive() && child->_isActiveInHierarchy != _isActiveInHierarchy)
        {
            child->_isActiveInHierarchy = _isActiveInHierarchy;
            child->OnActiveInTreeChanged();
        }
    }
}

void Actor::OnOrderInParentChanged()
{
    //if (GetScene())
    Level::callActorEvent(Level::ActorEventType::OnActorOrderInParentChanged, this, nullptr);
}

void Actor::OnStaticFlagsChanged()
{
}

void Actor::OnLayerChanged()
{
}

BoundingBox Actor::GetBoxWithChildren() const
{
    BoundingBox result = GetBox();
    for (int32 i = 0; i < Children.Count(); i++)
        BoundingBox::Merge(result, Children.Get()[i]->GetBoxWithChildren(), result);
    return result;
}

#if USE_EDITOR

BoundingBox Actor::GetEditorBox() const
{
    return GetBox();
}

BoundingBox Actor::GetEditorBoxChildren() const
{
    BoundingBox result = GetEditorBox();
    for (int32 i = 0; i < Children.Count(); i++)
        BoundingBox::Merge(result, Children.Get()[i]->GetEditorBoxChildren(), result);
    return result;
}

#endif

bool Actor::HasContentLoaded() const
{
    return true;
}

void Actor::UnregisterObjectHierarchy()
{
    if (IsRegistered())
        UnregisterObject();

    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        if (Scripts[i]->IsRegistered())
            Scripts[i]->UnregisterObject();
    }

    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children[i]->UnregisterObjectHierarchy();
    }
}

void Actor::InitializeHierarchy()
{
    Initialize();

    for (int32 i = 0; i < Scripts.Count(); i++)
        Scripts[i]->Initialize();

    for (int32 i = 0; i < Children.Count(); i++)
        Children[i]->InitializeHierarchy();
}

void Actor::Draw(RenderContext& renderContext)
{
}

void Actor::Draw(RenderContextBatch& renderContextBatch)
{
    // Default impl calls single-context
    for (RenderContext& renderContext : renderContextBatch.Contexts)
        Draw(renderContext);
}

#if USE_EDITOR

void Actor::OnDebugDraw()
{
    for (auto* script : Scripts)
        if (script->GetEnabled())
            script->OnDebugDraw();
}

void Actor::OnDebugDrawSelected()
{
    for (auto* script : Scripts)
        if (script->GetEnabled())
            script->OnDebugDrawSelected();
}

#endif

void Actor::ChangeScriptOrder(Script* script, int32 newIndex)
{
    int32 oldIndex = Scripts.Find(script);
    ASSERT(oldIndex != INVALID_INDEX);
    if (oldIndex == newIndex)
        return;

    Scripts.RemoveAtKeepOrder(oldIndex);

    // Check if index is invalid
    if (newIndex < 0 || newIndex >= Scripts.Count())
    {
        // Append at the end
        Scripts.Add(script);
    }
    else
    {
        // Change order
        Scripts.Insert(newIndex, script);
    }
}

Script* Actor::GetScriptByID(const Guid& id) const
{
    Script* result = nullptr;
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        if (Scripts[i]->GetID() == id)
        {
            result = Scripts[i];
            break;
        }
    }
    return result;
}

bool Actor::IsPrefabRoot() const
{
    return _isPrefabRoot != 0;
}

Actor* Actor::GetPrefabRoot()
{
    if (!HasPrefabLink())
        return nullptr;
    Actor* result = this;
    while (result && !result->IsPrefabRoot())
        result = result->GetParent();
    return result;
}

Actor* Actor::FindActor(const StringView& name) const
{
    Actor* result = nullptr;
    if (_name == name)
    {
        result = const_cast<Actor*>(this);
    }
    else
    {
        for (int32 i = 0; i < Children.Count(); i++)
        {
            result = Children[i]->FindActor(name);
            if (result)
                break;
        }
    }

    return result;
}

Actor* Actor::FindActor(const MClass* type, bool activeOnly) const
{
    CHECK_RETURN(type, nullptr);
    if (activeOnly && !_isActive)
        return nullptr;
    if ((GetClass()->IsSubClassOf(type) || GetClass()->HasInterface(type)))
        return const_cast<Actor*>(this);
    for (auto child : Children)
    {
        const auto actor = child->FindActor(type, activeOnly);
        if (actor)
            return actor;
    }
    return nullptr;
}

Actor* Actor::FindActor(const MClass* type, const StringView& name) const
{
    CHECK_RETURN(type, nullptr);
    if ((GetClass()->IsSubClassOf(type) || GetClass()->HasInterface(type)) && _name == name)
        return const_cast<Actor*>(this);
    for (auto child : Children)
    {
        const auto actor = child->FindActor(type, name);
        if (actor)
            return actor;
    }
    return nullptr;
}

Actor* Actor::FindActor(const MClass* type, const Tag& tag, bool activeOnly) const
{
    CHECK_RETURN(type, nullptr);
    if (activeOnly && !_isActive)
        return nullptr;
    if ((GetClass()->IsSubClassOf(type) || GetClass()->HasInterface(type)) && HasTag(tag))
        return const_cast<Actor*>(this);
    for (auto child : Children)
    {
        const auto actor = child->FindActor(type, tag, activeOnly);
        if (actor)
            return actor;
    }
    return nullptr;
}

Script* Actor::FindScript(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    if (type->IsInterface())
    {
        for (const auto script : Scripts)
        {
            if (script->GetClass()->HasInterface(type))
                return script;
        }
    }
    else
    {
        for (const auto script : Scripts)
        {
            if (script->GetClass()->IsSubClassOf(type))
                return script;
        }
    }
    for (auto child : Children)
    {
        const auto script = child->FindScript(type);
        if (script)
            return script;
    }
    return nullptr;
}

bool Actor::HasActorInHierarchy(Actor* a) const
{
    bool result = false;
    if (Children.Contains(a))
    {
        result = true;
    }
    else
    {
        for (int32 i = 0; i < Children.Count(); i++)
        {
            result = Children[i]->HasActorInHierarchy(a);
            if (result)
                break;
        }
    }
    return result;
}

bool Actor::HasActorInChildren(Actor* a) const
{
    return Children.Contains(a);
}

bool Actor::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return _box.Intersects(ray, distance, normal);
}

Actor* Actor::Intersects(const Ray& ray, Real& distance, Vector3& normal)
{
    if (!_isActive)
        return nullptr;

    // Check itself
    bool result = IntersectsItself(ray, distance, normal);
    Actor* minTarget = result ? (Actor*)this : nullptr;
    Real minDistance = result ? distance : MAX_Real;
    Vector3 minDistanceNormal = result ? normal : Vector3::Up;

    // Check all children
    for (int32 i = 0; i < Children.Count(); i++)
    {
        const auto target = Children[i]->Intersects(ray, distance, normal);
        if (target && minDistance > distance)
        {
            minDistanceNormal = normal;
            minDistance = distance;
            minTarget = target;
            result = true;
        }
    }

    distance = minDistance;
    normal = minDistanceNormal;
    return minTarget;
}

void Actor::LookAt(const Vector3& worldPos)
{
    const Quaternion orientation = LookingAt(worldPos);
    SetOrientation(orientation);
}

void Actor::LookAt(const Vector3& worldPos, const Vector3& worldUp)
{
    const Quaternion orientation = LookingAt(worldPos, worldUp);
    SetOrientation(orientation);
}

Quaternion Actor::LookingAt(const Vector3& worldPos) const
{
    const Vector3 direction = worldPos - _transform.Translation;
    if (direction.LengthSquared() < ZeroTolerance)
        return _parent->GetOrientation();

    const Float3 newForward = Vector3::Normalize(direction);
    const Float3 oldForward = _transform.Orientation * Vector3::Forward;

    Quaternion orientation;
    if ((newForward + oldForward).LengthSquared() < 0.00005f)
    {
        // 180 degree turn (infinite possible rotation axes)
        // Default to yaw i.e. use current Up
        orientation = Quaternion(-_transform.Orientation.Y, -_transform.Orientation.Z, _transform.Orientation.W, _transform.Orientation.X);
    }
    else
    {
        // Derive shortest arc to new direction
        Quaternion rotQuat;
        Quaternion::GetRotationFromTo(oldForward, newForward, rotQuat, Float3::Zero);
        orientation = rotQuat * _transform.Orientation;
    }

    return orientation;
}

Quaternion Actor::LookingAt(const Vector3& worldPos, const Vector3& worldUp) const
{
    const Vector3 direction = worldPos - _transform.Translation;
    if (direction.LengthSquared() < ZeroTolerance)
        return _parent->GetOrientation();
    const Float3 forward = Vector3::Normalize(direction);
    const Float3 up = Vector3::Normalize(worldUp);
    if (Math::IsOne(Float3::Dot(forward, up)))
    {
        return LookingAt(worldPos);
    }

    Quaternion orientation;
    Quaternion::LookRotation(direction, up, orientation);
    return orientation;
}

void WriteObjectToBytes(SceneObject* obj, rapidjson_flax::StringBuffer& buffer, MemoryWriteStream& output)
{
    // Create JSON
    CompactJsonWriter writer(buffer);

    writer.SceneObject(obj);

    // Write json to output
    // TODO: maybe compress json or use binary serialization
    output.WriteInt32((int32)buffer.GetSize());
    output.WriteBytes((byte*)buffer.GetString(), (int32)buffer.GetSize());

    // Store order in parent. Makes life easier for editor to sync objects order on undo/redo actions.
    output.WriteInt32(obj->GetOrderInParent());

    // Reuse string buffer
    buffer.Clear();
}

bool Actor::ToBytes(const Array<Actor*>& actors, MemoryWriteStream& output)
{
    PROFILE_CPU();
    if (actors.IsEmpty())
    {
        // Cannot serialize empty list
        return true;
    }

    // Collect object ids that exist in the serialized data to allow references mapping later
    Array<Guid> ids(Math::RoundUpToPowerOf2(actors.Count() * 2));
    for (int32 i = 0; i < actors.Count(); i++)
    {
        // By default we collect actors and scripts (they are ManagedObjects recognized by the id)

        auto actor = actors[i];
        if (!actor)
            continue;
        ids.Add(actor->GetID());
        for (int32 j = 0; j < actor->Scripts.Count(); j++)
        {
            const auto script = actor->Scripts[j];
            ids.Add(script->GetID());
        }
    }

    // Header
    output.WriteInt32(FLAXENGINE_VERSION_BUILD);

    // Serialized objects ids (for references mapping)
    output.WriteArray(ids);

    // Objects data
    rapidjson_flax::StringBuffer buffer;
    for (int32 i = 0; i < actors.Count(); i++)
    {
        Actor* actor = actors[i];
        if (!actor)
            continue;

        WriteObjectToBytes(actor, buffer, output);

        for (int32 j = 0; j < actor->Scripts.Count(); j++)
        {
            Script* script = actor->Scripts[j];

            WriteObjectToBytes(script, buffer, output);
        }
    }

    return false;
}

Array<byte> Actor::ToBytes(const Array<Actor*>& actors)
{
    Array<byte> data;
    MemoryWriteStream stream(Math::Min(actors.Count() * 256, 10 * 1024 * 1024));
    if (!ToBytes(actors, stream))
    {
        data.Set(stream.GetHandle(), stream.GetPosition());
    }
    return data;
}

bool Actor::FromBytes(const Span<byte>& data, Array<Actor*>& output, ISerializeModifier* modifier)
{
    PROFILE_CPU();
    output.Clear();

    ASSERT(modifier);
    if (data.Length() <= 0)
        return true;
    MemoryReadStream stream(data.Get(), data.Length());

    // Header
    int32 engineBuild;
    stream.ReadInt32(&engineBuild);
    if (engineBuild > FLAXENGINE_VERSION_BUILD || engineBuild < 6165)
    {
        LOG(Warning, "Unsupported actors data version.");
        return true;
    }

    // Serialized objects ids (for references mapping)
    Array<Guid> ids;
    stream.ReadArray(&ids);
    int32 objectsCount = ids.Count();
    if (objectsCount < 0)
        return true;

    // Prepare
    Array<int32> order;
    order.Resize(objectsCount);
    modifier->EngineBuild = engineBuild;
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->Resize(objectsCount);
    SceneObjectsFactory::Context context(modifier);

    // Deserialize objects
    Scripting::ObjectsLookupIdMapping.Set(&modifier->IdsMapping);
    auto startPos = stream.GetPosition();
    for (int32 i = 0; i < objectsCount; i++)
    {
        // Buffer
        int32 bufferSize;
        stream.ReadInt32(&bufferSize);
        const char* buffer = (const char*)stream.GetPositionHandle();
        stream.Move(bufferSize);

        // Order in parent
        int32 orderInParent;
        stream.ReadInt32(&orderInParent);
        order[i] = orderInParent;

        // Load JSON 
        rapidjson_flax::Document document;
        {
            PROFILE_CPU_NAMED("Json.Parse");
            document.Parse(buffer, bufferSize);
        }
        if (document.HasParseError())
        {
            Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
            return true;
        }

        // Create object
        auto obj = SceneObjectsFactory::Spawn(context, document);
        sceneObjects->At(i) = obj;
        if (obj == nullptr)
        {
            LOG(Warning, "Cannot create object.");
            continue;
        }
        obj->RegisterObject();

        // Add to results
        Actor* actor = dynamic_cast<Actor*>(obj);
        if (actor)
        {
            output.Add(actor);
        }
    }
    // TODO: optimize this to call json parsing only once per-object instead of twice (spawn + load)
    stream.SetPosition(startPos);
    for (int32 i = 0; i < objectsCount; i++)
    {
        // Buffer
        int32 bufferSize;
        stream.ReadInt32(&bufferSize);
        const char* buffer = (const char*)stream.GetPositionHandle();
        stream.Move(bufferSize);

        // Order in parent
        int32 orderInParent;
        stream.ReadInt32(&orderInParent);

        // Load JSON
        rapidjson_flax::Document document;
        {
            PROFILE_CPU_NAMED("Json.Parse");
            document.Parse(buffer, bufferSize);
        }
        if (document.HasParseError())
        {
            Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
            return true;
        }

        // Deserialize object
        auto obj = sceneObjects->At(i);
        if (obj)
            SceneObjectsFactory::Deserialize(context, obj, document);
        else
            SceneObjectsFactory::HandleObjectDeserializationError(document);
    }
    Scripting::ObjectsLookupIdMapping.Set(nullptr);

    // Update objects order
    //for (int32 i = 0; i < objectsCount; i++)
    {
        //SceneObject* obj = sceneObjects->At(i);
        // TODO: remove order from saved data?
        //obj->SetOrderInParent(order[i]);
    }

    // Call events (only for parents because they will propagate events down the tree)
    CollectionPoolCache<ActorsCache::ActorsListType>::ScopeCache parents = ActorsCache::ActorsListCache.Get();
    parents->EnsureCapacity(output.Count());
    Level::ConstructParentActorsTreeList(output, *parents);
    for (int32 i = 0; i < parents->Count(); i++)
    {
        // Break prefab links for actors from prefab but no a root ones (eg. when user duplicates a sub-prefab actor but not a root one)
        Actor* actor = parents->At(i);
        if (actor->HasPrefabLink() && !actor->IsPrefabRoot())
        {
            actor->BreakPrefabLink();
        }
    }
    for (int32 i = 0; i < parents->Count(); i++)
    {
        parents->At(i)->InitializeHierarchy();
    }
    for (int32 i = 0; i < parents->Count(); i++)
    {
        Actor* actor = parents->At(i);
        actor->OnTransformChanged();
    }

    // Initialize actor that are spawned to scene or create managed instanced for others
    for (int32 i = 0; i < parents->Count(); i++)
    {
        Actor* actor = parents->At(i);
        if (!actor->GetScene())
            continue;

        // Add to game
        SceneBeginData beginData;
        actor->BeginPlay(&beginData);
        beginData.OnDone();
        Level::callActorEvent(Level::ActorEventType::OnActorSpawned, actor, nullptr);
    }

    return false;
}

Array<Actor*> Actor::FromBytes(const Span<byte>& data)
{
    Array<Actor*> output;
    auto modifier = Cache::ISerializeModifier.Get();
    FromBytes(data, output, modifier.Value);
    return output;
}

Array<Actor*> Actor::FromBytes(const Span<byte>& data, const Dictionary<Guid, Guid>& idsMapping)
{
    Array<Actor*> output;
    auto modifier = Cache::ISerializeModifier.Get();
    modifier->IdsMapping = idsMapping;
    FromBytes(data, output, modifier.Value);
    return output;
}

Array<Guid> Actor::TryGetSerializedObjectsIds(const Span<byte>& data)
{
    PROFILE_CPU();
    Array<Guid> result;
    if (data.Length() > 0)
    {
        MemoryReadStream stream(data.Get(), data.Length());

        // Header
        int32 engineBuild;
        stream.ReadInt32(&engineBuild);
        if (engineBuild <= FLAXENGINE_VERSION_BUILD && engineBuild >= 6165)
        {
            // Serialized objects ids (for references mapping)
            stream.ReadArray(&result);
        }
    }

    return result;
}

String Actor::ToJson()
{
    PROFILE_CPU();
    rapidjson_flax::StringBuffer buffer;
    CompactJsonWriter writer(buffer);
    writer.SceneObject(this);
    String result;
    result.SetUTF8(buffer.GetString(), (int32)buffer.GetSize());
    return result;
}

void Actor::FromJson(const StringAnsiView& json)
{
    PROFILE_CPU();

    // Load JSON
    rapidjson_flax::Document document;
    {
        PROFILE_CPU_NAMED("Json.Parse");
        document.Parse(json.Get(), json.Length());
    }
    if (document.HasParseError())
    {
        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
        return;
    }

    // Deserialize object
    auto modifier = Cache::ISerializeModifier.Get();
    Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
    Deserialize(document, &*modifier);
    Scripting::ObjectsLookupIdMapping.Set(nullptr);
    OnTransformChanged();
}

Actor* Actor::Clone()
{
    // Collect actors to clone
    auto actors = ActorsCache::ActorsListCache.Get();
    actors->Add(this);
    SceneQuery::GetAllActors(this, *actors);

    // Serialize objects
    MemoryWriteStream stream;
    if (ToBytes(*actors, stream))
        return nullptr;

    // Remap object ids into a new ones
    auto modifier = Cache::ISerializeModifier.Get();
    for (int32 i = 0; i < actors->Count(); i++)
    {
        auto actor = actors->At(i);
        if (!actor)
            continue;
        modifier->IdsMapping.Add(actor->GetID(), Guid::New());
        for (int32 j = 0; j < actor->Scripts.Count(); j++)
        {
            const auto script = actor->Scripts[j];
            if (script)
                modifier->IdsMapping.Add(script->GetID(), Guid::New());
        }
    }

    // Deserialize objects
    Array<Actor*> output;
    if (FromBytes(ToSpan(stream.GetHandle(), (int32)stream.GetPosition()), output, modifier.Value) || output.IsEmpty())
        return nullptr;
    return output[0];
}

void Actor::SetPhysicsScene(PhysicsScene* scene)
{
    CHECK(scene);

    const auto previous = GetPhysicsScene();
    _physicsScene = scene;

    if (previous != _physicsScene)
    {
        OnPhysicsSceneChanged(previous);

        // cascade
        for (auto child : Children)
            child->SetPhysicsScene(scene);
    }
}

PhysicsScene* Actor::GetPhysicsScene() const
{
    return _physicsScene ? _physicsScene : Physics::DefaultScene;
}
