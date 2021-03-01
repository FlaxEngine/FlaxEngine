// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Script.h"
#include "Engine/Core/Log.h"
#include "ManagedSerialization.h"
#if USE_EDITOR
#include "StdTypesContainer.h"
#include "ManagedCLR/MClass.h"
#include "Editor/Editor.h"
#endif
#include "Scripting.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Threading/ThreadLocal.h"

#if USE_EDITOR
#define CHECK_EXECUTE_IN_EDITOR if (Editor::IsPlayMode || _executeInEditor)
#else
#define CHECK_EXECUTE_IN_EDITOR
#endif

Script::Script(const SpawnParams& params)
    : SceneObject(params)
    , _enabled(true)
    , _tickFixedUpdate(false)
    , _tickUpdate(false)
    , _tickLateUpdate(false)
    , _wasStartCalled(false)
    , _wasEnableCalled(false)
{
#if USE_EDITOR
    _executeInEditor = GetClass()->HasAttribute(StdTypesContainer::Instance()->ExecuteInEditModeAttribute);
#endif
}

void Script::SetEnabled(bool value)
{
    // Check if value will change
    if (GetEnabled() != value)
    {
        // Change state
        _enabled = value;

        // Call event for the script
        if (_parent == nullptr || (_parent->IsDuringPlay() && _parent->IsActiveInHierarchy()))
        {
            if (value)
            {
                if (!_wasEnableCalled)
                {
                    Start();
                    Enable();
                }
            }
            else if (_wasEnableCalled)
            {
                Disable();
            }
        }
    }
}

Actor* Script::GetActor() const
{
    return _parent;
}

void Script::SetActor(Actor* value)
{
    SetParent(value, true);
}

void Script::SetParent(Actor* value, bool canBreakPrefabLink)
{
    // Check if value won't change
    if (_parent == value)
        return;
    if (IsDuringPlay() && !IsInMainThread())
    {
        LOG(Error, "Editing scene hierarchy is only allowed on a main thread.");
        return;
    }

    // Unlink from the old one
    if (_parent)
    {
        if (!value && _parent->IsDuringPlay() && _parent->IsActiveInHierarchy() && GetEnabled())
        {
            // Call disable when script is removed from actor (new actor is null)
            Disable();
        }

        _parent->Scripts.RemoveKeepOrder(this);
    }

    // Set value
    const auto previous = _parent;
    _parent = value;

    // Link to the new one
    if (_parent)
    {
        _parent->Scripts.Add(this);
    }

    // Break prefab link for prefab instance objects
    if (HasPrefabLink() && IsDuringPlay() && canBreakPrefabLink)
    {
        BreakPrefabLink();
    }

    // Check if actor is already in play but script isn't
    if (value && value->IsDuringPlay() && !IsDuringPlay())
    {
        // Prepare for gameplay
        PostSpawn();
        {
            SceneBeginData beginData;
            BeginPlay(&beginData);
            beginData.OnDone();
        }

        // Fire events for scripting
        CHECK_EXECUTE_IN_EDITOR
        {
            OnAwake();
        }
        if (GetEnabled())
        {
            Start();
            Enable();
        }
    }
    else if (!previous && value && value->IsDuringPlay() && value->IsActiveInHierarchy() && GetEnabled())
    {
        // Call enable when script is added to actor (previous actor was null)
        Enable();
    }
}

int32 Script::GetOrderInParent() const
{
    return _parent ? _parent->Scripts.Find((Script*)this) : INVALID_INDEX;
}

void Script::SetOrderInParent(int32 index)
{
    if (!_parent)
        return;

    // Cache data
    auto& parentScripts = _parent->Scripts;
    const int32 currentIndex = parentScripts.Find(this);
    ASSERT(currentIndex != INVALID_INDEX);

    // Check if index will change
    if (currentIndex != index)
    {
        parentScripts.RemoveAtKeepOrder(currentIndex);

        // Check if index is invalid
        if (index < 0 || index >= parentScripts.Count())
        {
            // Append at the end
            parentScripts.Add(this);
        }
        else
        {
            // Change order
            parentScripts.Insert(index, this);
        }
    }
}

void Script::SetupType()
{
    // Enable tick functions based on the method overriden in C# or Visual Script
    ScriptingTypeHandle typeHandle = GetTypeHandle();
    _tickUpdate = _tickLateUpdate = _tickFixedUpdate = 0;
    while (typeHandle != Script::TypeInitializer)
    {
        auto& type = typeHandle.GetType();
        _tickUpdate |= type.Script.ScriptVTable[8] != nullptr;
        _tickLateUpdate |= type.Script.ScriptVTable[9] != nullptr;
        _tickFixedUpdate |= type.Script.ScriptVTable[10] != nullptr;
        typeHandle = type.GetBaseType();
    }
}

void Script::Start()
{
    if (_wasStartCalled)
        return;

    _wasStartCalled = 1;
    CHECK_EXECUTE_IN_EDITOR
    {
        OnStart();
    }
}

void Script::Enable()
{
    ASSERT(GetEnabled());
    ASSERT(!_wasEnableCalled);

    if (_parent && _parent->GetScene())
    {
        _parent->GetScene()->Ticking.AddScript(this);
        _wasEnableCalled = 1;
    }

    CHECK_EXECUTE_IN_EDITOR
    {
        OnEnable();
    }
}

void Script::Disable()
{
    ASSERT(_wasEnableCalled);

    CHECK_EXECUTE_IN_EDITOR
    {
        OnDisable();
    }

    if (_parent && _parent->GetScene())
    {
        _wasEnableCalled = 0;
        _parent->GetScene()->Ticking.RemoveScript(this);
    }
}

String Script::ToString() const
{
    const auto& type = GetType();
    return type.ToString();
}

void Script::OnDeleteObject()
{
    // Ensure to unlink from the parent (it will call Disable event if required)
    SetParent(nullptr);

    // Check if remove object from game
    if (IsDuringPlay())
    {
        CHECK_EXECUTE_IN_EDITOR
        {
            OnDestroy();
        }
        EndPlay();
    }

    // Base
    SceneObject::OnDeleteObject();
}

const Guid& Script::GetSceneObjectId() const
{
    return GetID();
}

void Script::PostLoad()
{
    if (Flags & ObjectFlags::IsManagedType || Flags & ObjectFlags::IsCustomScriptingType)
        SetupType();

    // Use lazy creation for the managed instance, just register the object
    if (!IsRegistered())
        RegisterObject();
}

void Script::PostSpawn()
{
    if (Flags & ObjectFlags::IsManagedType || Flags & ObjectFlags::IsCustomScriptingType)
        SetupType();

    // Create managed object
    if (!HasManagedInstance())
        CreateManaged();
}

void Script::BeginPlay(SceneBeginData* data)
{
    // Perform additional verification
    ASSERT(!IsDuringPlay());

    // Set flag
    Flags |= ObjectFlags::IsDuringPlay;
}

void Script::EndPlay()
{
    // Clear flag
    Flags &= ~ObjectFlags::IsDuringPlay;

    // Cleanup managed object
    DestroyManaged();
    if (IsRegistered())
        UnregisterObject();
}

void Script::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    SceneObject::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(Script);

    SERIALIZE_BIT_MEMBER(Enabled, _enabled);
}

void Script::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    SceneObject::Deserialize(stream, modifier);

    DESERIALIZE_BIT_MEMBER(Enabled, _enabled);
    DESERIALIZE_MEMBER(PrefabID, _prefabID);

    Guid parentId = Guid::Empty;
    DESERIALIZE_MEMBER(ParentID, parentId);
    const auto parent = Scripting::FindObject<Actor>(parentId);
    if (_parent != parent)
    {
        if (_parent)
            _parent->Scripts.RemoveKeepOrder(this);
        _parent = parent;
        if (_parent)
            _parent->Scripts.Add(this);
    }
    else if (!parent && parentId.IsValid())
    {
        LOG(Warning, "Missing parent actor {0} for \'{1}\'", parentId, ToString());
    }
}
