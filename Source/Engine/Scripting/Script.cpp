// Copyright (c) Wojciech Figat. All rights reserved.

#include "Script.h"
#include "Engine/Core/Log.h"
#if USE_EDITOR
#include "Internal/StdTypesContainer.h"
#include "ManagedCLR/MClass.h"
#include "Editor/Editor.h"
#endif
#include "Scripting.h"
#include "Engine/Level/Actor.h"
#include "Engine/Level/Level.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Threading/Threading.h"

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
    , _tickLateFixedUpdate(false)
    , _wasAwakeCalled(false)
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

bool Script::IsEnabledInHierarchy() const
{
    return _enabled && (_parent == nullptr || _parent->IsActiveInHierarchy());
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
        if (!value && _parent->IsDuringPlay() && _parent->IsActiveInHierarchy() && GetEnabled() && _wasEnableCalled)
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
        Initialize();
        {
            SceneBeginData beginData;
            BeginPlay(&beginData);
            beginData.OnDone();
        }

        // Fire events for scripting
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
    while (typeHandle != Script::TypeInitializer)
    {
        auto& type = typeHandle.GetType();
        if (type.Script.ScriptVTable)
        {
            _tickUpdate |= type.Script.ScriptVTable[8] != nullptr;
            _tickLateUpdate |= type.Script.ScriptVTable[9] != nullptr;
            _tickFixedUpdate |= type.Script.ScriptVTable[10] != nullptr;
            _tickLateFixedUpdate |= type.Script.ScriptVTable[11] != nullptr;
        }
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
    // Call OnDisable
    if (_wasEnableCalled)
    {
        Disable();
    }

    // Call OnDestroy
    if (_wasAwakeCalled)
    {
        _wasAwakeCalled = false;
        CHECK_EXECUTE_IN_EDITOR
        {
            OnDestroy();
        }
    }

    // End play
    if (IsDuringPlay())
    {
        EndPlay();
    }

    // Unlink from parent
    SetParent(nullptr);

    // Base
    SceneObject::OnDeleteObject();
}

const Guid& Script::GetSceneObjectId() const
{
    return GetID();
}

void Script::Initialize()
{
    ASSERT(!IsDuringPlay());

    if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType | ObjectFlags::IsCustomScriptingType))
        SetupType();

    // Use lazy creation for the managed instance, just register the object
    if (!IsRegistered())
        RegisterObject();

    // Call OnAwake
    if (!_wasAwakeCalled)
    {
        _wasAwakeCalled = true;
        CHECK_EXECUTE_IN_EDITOR
        {
            OnAwake();
        }
    }
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
    //DestroyManaged();
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
                    SetParent(parent, false);
                }
                else
                {
                    if (_parent)
                        _parent->Scripts.RemoveKeepOrder(this);
                    _parent = parent;
                    if (_parent)
                        _parent->Scripts.Add(this);
                }
            }
            else if (!parent && parentId.IsValid())
            {
                LOG(Warning, "Missing parent actor {0} for \'{1}\'", parentId, ToString());
            }
        }
    }
}
