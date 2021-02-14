// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Actor.h"
#include "ActorsCache.h"
#include "Level.h"
#include "SceneObjectsFactory.h"
#include "Scene/Scene.h"
#include "Prefabs/Prefab.h"
#include "Prefabs/PrefabManager.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/LayersTagsSettings.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Content/Content.h"
#include "Engine/Core/Cache.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"
#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "Engine/Graphics/RenderView.h"
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

Actor::Actor(const SpawnParams& params)
    : SceneObject(params)
    , _isActive(true)
    , _isActiveInHierarchy(true)
    , _isPrefabRoot(false)
    , _isEnabled(false)
    , _layer(0)
    , _tag(ACTOR_TAG_INVALID)
    , _scene(nullptr)
    , _staticFlags(StaticFlags::FullyStatic)
    , _localTransform(Transform::Identity)
    , _transform(Transform::Identity)
    , _sphere(BoundingSphere::Empty)
    , _box(BoundingBox::Empty)
    , HideFlags(HideFlags::None)
{
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
        Children[i]->SetSceneInHierarchy(scene);
    }
}

void Actor::OnEnableInHierarchy()
{
    OnEnable();

    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children[i]->OnEnableInHierarchy();
    }
}

void Actor::OnDisableInHierarchy()
{
    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children[i]->OnDisableInHierarchy();
    }

    OnDisable();
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
            _parent = nullptr;
            _scene = nullptr;
        }
    }
    else if (_parent)
    {
        // Unlink from the parent
        _parent->Children.RemoveKeepOrder(this);
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
        auto child = Children[i];
        ASSERT(child->_parent == this);
        child->_parent = nullptr;
        child->DeleteObject();
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
        auto script = Scripts[i];
        ASSERT(script->_parent == this);
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
    // Check if value won't change
    if (_parent == value)
        return;
    if (!IsInMainThread())
    {
        LOG(Error, "Editing scene hierarchy is only allowed on a main thread.");
        return;
    }

    // Peek the previous state
    const Transform prevTransform = _transform;
    const bool wasActiveInTree = IsActiveInHierarchy();
    const auto prevParent = _parent;
    const auto prevScene = _scene;
    const auto newScene = value ? value->_scene : nullptr;

    // Detect it actor is not in a game but new parent is already in a game (we should spawn it)
    const bool isBeingSpawned = !IsDuringPlay() && newScene && value->IsDuringPlay();

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
    }

    // Set value
    _parent = value;

    // Link to the new one
    if (_parent)
    {
        _parent->Children.Add(this);
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
        {
            _parent->GetTransform().WorldToLocal(prevTransform, _localTransform);
        }
        else
        {
            _localTransform = prevTransform;
        }
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
        PostSpawn();
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

    // Cache data
    auto& parentChildren = _parent->Children;
    const int32 currentIndex = parentChildren.Find(this);
    ASSERT(currentIndex != INVALID_INDEX);

    // Check if index will change
    if (currentIndex != index)
    {
        parentChildren.RemoveAtKeepOrder(currentIndex);

        // Check if index is invalid
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

        // Fire event
        OnOrderInParentChanged();
    }
}

Actor* Actor::GetChild(const StringView& name) const
{
    for (int32 i = 0; i < Children.Count(); i++)
    {
        if (Children[i]->GetName() == name)
            return Children[i];
    }

    return nullptr;
}

Actor* Actor::GetChild(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    for (auto child : Children)
    {
        if (child->GetClass()->IsSubClassOf(type))
            return child;
    }
    return nullptr;
}

Array<Actor*> Actor::GetChildren(const MClass* type) const
{
    Array<Actor*> result;
    for (auto child : Children)
        if (child->GetClass()->IsSubClassOf(type))
            result.Add(child);
    return result;
}

Actor* Actor::GetChildByPrefabObjectId(const Guid& prefabObjectId) const
{
    Actor* result = nullptr;
    for (int32 i = 0; i < Children.Count(); i++)
    {
        if (Children[i]->GetPrefabObjectID() == prefabObjectId)
        {
            result = Children[i];
            break;
        }
    }
    return result;
}

bool Actor::HasTag(const StringView& tag) const
{
    return HasTag() && tag == LayersAndTagsSettings::Instance()->Tags[_tag];
}

const String& Actor::GetLayerName() const
{
    const auto settings = LayersAndTagsSettings::Instance();
    return settings->Layers[_layer];
}

const String& Actor::GetTag() const
{
    if (HasTag())
    {
        const auto settings = LayersAndTagsSettings::Instance();
        return settings->Tags[_tag];
    }
    return String::Empty;
}

void Actor::SetLayer(int32 layerIndex)
{
    layerIndex = Math::Min<int32>(layerIndex, 31);
    if (layerIndex == _layer)
        return;

    _layer = layerIndex;
    OnLayerChanged();
}

void Actor::SetTagIndex(int32 tagIndex)
{
    if (tagIndex == ACTOR_TAG_INVALID)
    {
    }
    else if (LayersAndTagsSettings::Instance()->Tags.IsEmpty())
    {
        tagIndex = ACTOR_TAG_INVALID;
    }
    else
    {
        tagIndex = tagIndex < 0 ? ACTOR_TAG_INVALID : Math::Min(tagIndex, LayersAndTagsSettings::Instance()->Tags.Count() - 1);
    }
    if (tagIndex == _tag)
        return;

    _tag = tagIndex;
    OnTagChanged();
}

void Actor::SetTag(const StringView& tagName)
{
    int32 tagIndex;
    if (tagName.IsEmpty())
    {
        tagIndex = ACTOR_TAG_INVALID;
    }
    else
    {
        tagIndex = LayersAndTagsSettings::Instance()->Tags.Find(tagName);
        if (tagIndex == -1)
        {
            LOG(Error, "Cannot change actor tag. Given value is invalid.");
            return;
        }
    }

    SetTagIndex(tagIndex);
}

void Actor::SetName(const StringView& value)
{
    // Validate name
    if (value.Length() == 0)
    {
        LOG(Warning, "Cannot change actor '{0}' name. Name cannot be empty.", ToString());
        return;
    }

    // Check if name won't change
    if (_name == value)
        return;

    // Set name
    _name = value;

    // Fire events
    if (GetScene())
        Level::callActorEvent(Level::ActorEventType::OnActorNameChanged, this, nullptr);
}

Script* Actor::GetScript(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    for (auto script : Scripts)
    {
        if (script->GetClass()->IsSubClassOf(type))
            return script;
    }
    return nullptr;
}

Array<Script*> Actor::GetScripts(const MClass* type) const
{
    Array<Script*> result;
    for (auto script : Scripts)
        if (script->GetClass()->IsSubClassOf(type))
            result.Add(script);
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
    _staticFlags = value;
}

void Actor::SetTransform(const Transform& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Transform::NearEqual(_transform, value))
    {
        if (_parent)
            _parent->GetTransform().WorldToLocal(value, _localTransform);
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
            _localTransform.Translation = _parent->GetTransform().WorldToLocal(value);
        else
            _localTransform.Translation = value;
        OnTransformChanged();
    }
}

void Actor::SetOrientation(const Quaternion& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Quaternion::NearEqual(_transform.Orientation, value))
    {
        if (_parent)
        {
            _localTransform.Orientation = _parent->GetOrientation();
            _localTransform.Orientation.Invert();
            Quaternion::Multiply(_localTransform.Orientation, value, _localTransform.Orientation);
            _localTransform.Orientation.Normalize();
        }
        else
        {
            _localTransform.Orientation = value;
        }
        OnTransformChanged();
    }
}

void Actor::SetScale(const Vector3& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Vector3::NearEqual(_transform.Scale, value))
    {
        if (_parent)
            Vector3::Divide(value, _parent->GetScale(), _localTransform.Scale);
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

void Actor::SetDirection(const Vector3& value)
{
    CHECK(!value.IsNanOrInfinity());
    Quaternion orientation;
    if (Vector3::Dot(value, Vector3::Up) >= 0.999f)
    {
        Quaternion::RotationAxis(Vector3::Left, PI_HALF, orientation);
    }
    else
    {
        const Vector3 right = Vector3::Cross(value, Vector3::Up);
        const Vector3 up = Vector3::Cross(right, value);
        Quaternion::LookRotation(value, up, orientation);
    }
    SetOrientation(orientation);
}

void Actor::SetLocalTransform(const Transform& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Transform::NearEqual(_localTransform, value))
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

    if (!Quaternion::NearEqual(_localTransform.Orientation, v))
    {
        _localTransform.Orientation = v;
        OnTransformChanged();
    }
}

void Actor::SetLocalScale(const Vector3& value)
{
    CHECK(!value.IsNanOrInfinity());
    if (!Vector3::NearEqual(_localTransform.Scale, value))
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
    _transform.GetWorld(worldToLocal);
    worldToLocal.Invert();
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

void Actor::PostLoad()
{
    // Cache scene
    if (_parent)
        _scene = _parent->GetScene();

    // Cache flag
    _isActiveInHierarchy = _isActive && (_parent == nullptr || _parent->IsActiveInHierarchy());

    // Use lazy creation for the managed instance, just register the object
    if (!IsRegistered())
        RegisterObject();
}

void Actor::PostSpawn()
{
    // Cache scene
    if (_parent)
        _scene = _parent->GetScene();

    // Cache flag
    _isActiveInHierarchy = _isActive && (_parent == nullptr || _parent->IsActiveInHierarchy());

    // Create managed object
    if (!HasManagedInstance())
        CreateManaged();

    // Init scripts
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        Scripts[i]->PostSpawn();
    }

    // Init children
    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children[i]->PostSpawn();
    }
}

void Actor::BeginPlay(SceneBeginData* data)
{
    // Perform additional verification
    ASSERT(!IsDuringPlay());
#if BUILD_DEBUG
    for (int32 i = 0; i < Children.Count(); i++)
    {
        ASSERT(Children[i]->IsDuringPlay() == IsDuringPlay());
    }
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        ASSERT(Scripts[i]->IsDuringPlay() == IsDuringPlay());
    }
#endif

    // Set flag
    Flags |= ObjectFlags::IsDuringPlay;

    OnBeginPlay();

    // Update scripts
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        if (!Scripts[i]->IsDuringPlay())
            Scripts[i]->BeginPlay(data);
    }

    // Update children
    for (int32 i = 0; i < Children.Count(); i++)
    {
        if (!Children[i]->IsDuringPlay())
            Children[i]->BeginPlay(data);
    }

    // Fire events for scripting
    for (auto* script : Scripts)
    {
        CHECK_EXECUTE_IN_EDITOR
        {
            script->OnAwake();
        }
    }
    if (IsActiveInHierarchy() && GetScene() && !_isEnabled)
    {
        OnEnable();
    }
}

void Actor::EndPlay()
{
    // Perform additional verification
    ASSERT(IsDuringPlay());
#if BUILD_DEBUG
    for (int32 i = 0; i < Children.Count(); i++)
    {
        ASSERT(Children[i]->IsDuringPlay() == IsDuringPlay());
    }
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        ASSERT(Scripts[i]->IsDuringPlay() == IsDuringPlay());
    }
#endif

    // Fire event for scripting
    if (IsActiveInHierarchy() && GetScene())
    {
        ASSERT(GetScene());
        OnDisable();
    }

    OnEndPlay();

    // Clear flag
    Flags &= ~ObjectFlags::IsDuringPlay;

    // Call event deeper
    for (int32 i = 0; i < Children.Count(); i++)
    {
        Children[i]->EndPlay();
    }

    // Fire event for scripting
    for (auto* script : Scripts)
    {
        CHECK_EXECUTE_IN_EDITOR
        {
            script->OnDestroy();
        }
    }

    // Inform attached scripts
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        Scripts[i]->EndPlay();
    }

    // Cleanup managed object
    DestroyManaged();
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
    if (!other || _tag != other->_tag)
    {
        stream.JKEY("Tag");
        stream.String(GetTag());
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
            if (GetChildByPrefabObjectId(prefabObjectId) == nullptr)
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
            if (GetScriptByPrefabObjectId(prefabObjectId) == nullptr)
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

    Guid parentId = Guid::Empty;
    DESERIALIZE_MEMBER(ParentID, parentId);
    const auto parent = Scripting::FindObject<Actor>(parentId);
    if (_parent != parent)
    {
        if (_parent)
            _parent->Children.RemoveKeepOrder(this);
        _parent = parent;
        if (_parent)
            _parent->Children.Add(this);
    }
    else if (!parent && parentId.IsValid())
    {
        LOG(Warning, "Missing parent actor {0} for \'{1}\'", parentId, ToString());
    }

    // StaticFlags update - added StaticFlags::Navigation
    // [Deprecated on 17.05.2020, expires on 17.05.2021]
    if (modifier->EngineBuild < 6178 && (int32)_staticFlags == (1 + 2 + 4))
    {
        _staticFlags |= StaticFlags::Navigation;
    }

    // Resolve tag (it may be missing in the current configuration
    const auto tag = stream.FindMember("Tag");
    if (tag != stream.MemberEnd())
    {
        if (tag->value.IsString() && tag->value.GetStringLength())
        {
            const String tagName = tag->value.GetText();
            _tag = LayersAndTagsSettings::Instance()->GetOrAddTag(tagName);
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
    ASSERT(!_isEnabled);
    _isEnabled = true;

    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto script = Scripts[i];
        if (script->GetEnabled() && !script->_wasStartCalled)
            script->Start();
    }

    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        auto script = Scripts[i];
        if (script->GetEnabled() && !script->_wasEnableCalled)
            script->Enable();
    }
}

void Actor::OnDisable()
{
    ASSERT(_isEnabled);
    _isEnabled = false;

    for (int32 i = Scripts.Count() - 1; i >= 0; i--)
    {
        auto script = Scripts[i];
        if (script->GetEnabled() && script->_wasEnableCalled)
            script->Disable();
    }
}

void Actor::OnTransformChanged()
{
    ASSERT_LOW_LAYER(!_localTransform.IsNanOrInfinity());

    if (_parent)
    {
        _parent->GetTransform().LocalToWorld(_localTransform, _transform);
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

BoundingBox Actor::GetBoxWithChildren() const
{
    BoundingBox result = GetBox();

    for (int32 i = 0; i < Children.Count(); i++)
    {
        BoundingBox::Merge(result, Children[i]->GetBoxWithChildren(), result);
    }

    return result;
}

#if USE_EDITOR

BoundingBox Actor::GetEditorBoxChildren() const
{
    BoundingBox result = GetEditorBox();

    for (int32 i = 0; i < Children.Count(); i++)
    {
        BoundingBox::Merge(result, Children[i]->GetEditorBoxChildren(), result);
    }

    return result;
}

#endif

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

void Actor::DrawGeneric(RenderContext& renderContext)
{
    // Generic drawing uses only GBuffer Fill Pass and simple frustum culling (see SceneRendering for more optimized drawing)
    if (renderContext.View.Pass & DrawPass::GBuffer)
    {
        Draw(renderContext);
    }
}

void Actor::DrawHierarchy(RenderContext& renderContext)
{
    // Draw actor itself
    DrawGeneric(renderContext);

    // Draw children
    if (renderContext.View.IsOfflinePass)
    {
        for (int32 i = 0; i < Children.Count(); i++)
        {
            auto child = Children[i];
            if (child->GetIsActive() && child->GetStaticFlags() & renderContext.View.StaticFlagsMask)
            {
                child->DrawHierarchy(renderContext);
            }
        }
    }
    else
    {
        for (int32 i = 0; i < Children.Count(); i++)
        {
            auto child = Children[i];
            if (child->GetIsActive())
            {
                child->DrawHierarchy(renderContext);
            }
        }
    }
}

#if USE_EDITOR

void Actor::OnDebugDraw()
{
    for (auto* script : Scripts)
        if (script->GetEnabled())
            script->OnDebugDraw();

    for (auto& child : Children)
    {
        if (child->GetIsActive())
            child->OnDebugDraw();
    }
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

Script* Actor::GetScriptByPrefabObjectId(const Guid& prefabObjectId) const
{
    Script* result = nullptr;
    for (int32 i = 0; i < Scripts.Count(); i++)
    {
        if (Scripts[i]->GetPrefabObjectID() == prefabObjectId)
        {
            result = Scripts[i];
            break;
        }
    }
    return result;
}

Actor* Actor::FindActor(const StringView& name) const
{
    Actor* result = nullptr;
    if (StringUtils::Compare(*_name, *name) == 0)
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

Actor* Actor::FindActor(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    if (GetClass()->IsSubClassOf(type))
        return const_cast<Actor*>(this);
    for (auto child : Children)
    {
        const auto actor = child->FindActor(type);
        if (actor)
            return actor;
    }
    return nullptr;
}

Script* Actor::FindScript(const MClass* type) const
{
    CHECK_RETURN(type, nullptr);
    for (auto script : Scripts)
    {
        if (script->GetClass()->IsSubClassOf(type))
            return script;
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

bool Actor::IntersectsItself(const Ray& ray, float& distance, Vector3& normal)
{
    return GetBox().Intersects(ray, distance, normal);
}

Actor* Actor::Intersects(const Ray& ray, float& distance, Vector3& normal)
{
    if (!_isActive)
        return nullptr;

    // Check itself
    bool result = IntersectsItself(ray, distance, normal);
    Actor* minTarget = result ? (Actor*)this : nullptr;
    float minDistance = result ? distance : MAX_float;
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
    const Vector3 direction = worldPos - _transform.Translation;
    if (direction.LengthSquared() < ZeroTolerance)
        return;

    const Vector3 newForward = Vector3::Normalize(direction);
    const Vector3 oldForward = _transform.Orientation * Vector3::Forward;

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
        Quaternion::GetRotationFromTo(oldForward, newForward, rotQuat, Vector3::Zero);
        orientation = rotQuat * _transform.Orientation;
    }

    SetOrientation(orientation);
}

void Actor::LookAt(const Vector3& worldPos, const Vector3& worldUp)
{
    const Vector3 direction = worldPos - _transform.Translation;
    if (direction.LengthSquared() < ZeroTolerance)
        return;
    const Vector3 forward = Vector3::Normalize(direction);
    const Vector3 up = Vector3::Normalize(worldUp);

    if (Math::IsOne(Vector3::Dot(forward, up)))
    {
        LookAt(worldPos);
        return;
    }

    Quaternion orientation;
    Quaternion::LookRotation(direction, up, orientation);
    SetOrientation(orientation);
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

    // Deserialize objects
    Scripting::ObjectsLookupIdMapping.Set(&modifier->IdsMapping);
    auto startPos = stream.GetPosition();
    for (int32 i = 0; i < objectsCount; i++)
    {
        // Buffer
        int32 bufferSize;
        stream.ReadInt32(&bufferSize);
        const char* buffer = (const char*)stream.GetPositionHandle();
        stream.Read(bufferSize);

        // Order in parent
        int32 orderInParent;
        stream.ReadInt32(&orderInParent);
        order.At(i) = orderInParent;

        // Load JSON 
        rapidjson_flax::Document document;
        document.Parse(buffer, bufferSize);
        if (document.HasParseError())
        {
            Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
            return true;
        }

        // Create object
        auto obj = SceneObjectsFactory::Spawn(document, modifier);
        if (obj == nullptr)
        {
            LOG(Warning, "Cannot create object.");
            return true;
        }
        obj->RegisterObject();

        // Add to results
        sceneObjects->At(i) = obj;
        Actor* actor = dynamic_cast<Actor*>(obj);
        if (actor)
        {
            output.Add(actor);
        }
    }
    stream.SetPosition(startPos);
    for (int32 i = 0; i < objectsCount; i++)
    {
        // Buffer
        int32 bufferSize;
        stream.ReadInt32(&bufferSize);
        const char* buffer = (const char*)stream.GetPositionHandle();
        stream.Read(bufferSize);

        // Order in parent
        int32 orderInParent;
        stream.ReadInt32(&orderInParent);
        order.Add(orderInParent);

        // Load JSON 
        rapidjson_flax::Document document;
        document.Parse(buffer, bufferSize);
        if (document.HasParseError())
        {
            Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
            return true;
        }

        // Deserialize object
        auto obj = sceneObjects->At(i);
        SceneObjectsFactory::Deserialize(obj, document, modifier);
    }
    Scripting::ObjectsLookupIdMapping.Set(nullptr);

    // Link objects
    for (int32 i = 0; i < objectsCount; i++)
    {
        SceneObject* obj = sceneObjects->At(i);
        obj->PostLoad();
    }

    // Update objects order
    for (int32 i = 0; i < objectsCount; i++)
    {
        SceneObject* obj = sceneObjects->At(i);
        obj->SetOrderInParent(order[i]);
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
    rapidjson_flax::StringBuffer buffer;
    CompactJsonWriter writer(buffer);
    writer.SceneObject(this);
    return String(buffer.GetString());
}

void Actor::FromJson(const StringAnsiView& json)
{
    // Load JSON
    rapidjson_flax::Document document;
    document.Parse(json.Get(), json.Length());
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
