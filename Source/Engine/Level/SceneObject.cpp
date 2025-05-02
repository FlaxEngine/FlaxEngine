// Copyright (c) Wojciech Figat. All rights reserved.

#include "SceneObject.h"
#include "Engine/Core/Log.h"
#include "Engine/Physics/Joints/Joint.h"
#include "Engine/Content/Content.h"
#include "Engine/Level/Prefabs/Prefab.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/Internal/ManagedSerialization.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Serialization/Serialization.h"

void SceneBeginData::OnDone()
{
    for (int32 i = 0; i < JointsToCreate.Count(); i++)
    {
        JointsToCreate[i]->Create();
    }

    JointsToCreate.Clear();
}

SceneObject::SceneObject(const SpawnParams& params)
    : Base(params)
    , _parent(nullptr)
    , _prefabID(Guid::Empty)
    , _prefabObjectID(Guid::Empty)
{
}

SceneObject::~SceneObject()
{
}

void SceneObject::LinkPrefab(const Guid& prefabId, const Guid& prefabObjectId)
{
    ASSERT(prefabId.IsValid());

    // Link
    _prefabID = prefabId;
    _prefabObjectID = prefabObjectId;

    if (_prefabID.IsValid() && _prefabObjectID.IsValid())
    {
        auto prefab = Content::LoadAsync<Prefab>(_prefabID);
        if (prefab == nullptr || prefab->WaitForLoaded())
        {
            _prefabID = Guid::Empty;
            _prefabObjectID = Guid::Empty;
            LOG(Warning, "Failed to load prefab linked to the actor.");
        }
    }
}

void SceneObject::BreakPrefabLink()
{
    // Invalidate link
    _prefabID = Guid::Empty;
    _prefabObjectID = Guid::Empty;
}

String SceneObject::GetNamePath(Char separatorChar) const
{
    Array<StringView> names;
    const Actor* a = dynamic_cast<const Actor*>(this);
    if (!a)
        a = GetParent();
    while (a)
    {
        names.Add(a->GetName());
        a = a->GetParent();
    }
    if (names.IsEmpty())
        return String::Empty;
    int32 length = names.Count() - 1;
    for (int32 i = 0; i < names.Count(); i++)
        length += names[i].Length();
    if (length == 0)
        return String::Empty;
    String result;
    result.ReserveSpace(length);
    Char* ptr = result.Get();
    for (int32 i = names.Count() - 1; i >= 0; i--)
    {
        const String& name = names[i];
        Platform::MemoryCopy(ptr, name.Get(), name.Length() * sizeof(Char));
        ptr += name.Length();
        if (i != 0)
            *ptr++ = separatorChar;
    }
    *ptr = 0;
    return result.ToString();
}

void SceneObject::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(SceneObject);

    stream.JKEY("ID");
    stream.Guid(_id);

    if (other && HasPrefabLink())
    {
        stream.JKEY("PrefabID");
        stream.Guid(_prefabID);

        stream.JKEY("PrefabObjectID");
        stream.Guid(_prefabObjectID);
    }
    else
    {
        stream.JKEY("TypeName");
        stream.String(GetType().Fullname);
    }

    if (_parent)
    {
        stream.JKEY("ParentID");
        stream.Guid(_parent->GetID());
    }

#if !COMPILE_WITHOUT_CSHARP
    // Handle C# objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType))
    {
        stream.JKEY("V");
        if (other)
        {
            ManagedSerialization::SerializeDiff(stream, GetOrCreateManagedInstance(), other->GetOrCreateManagedInstance());
        }
        else
        {
            ManagedSerialization::Serialize(stream, GetOrCreateManagedInstance());
        }
    }
#endif

    // Handle custom scripting objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
    {
        stream.JKEY("D");
        _type.Module->SerializeObject(stream, this, other);
    }
}

void SceneObject::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // _id is deserialized by Actor/Script impl
    // _parent is deserialized by Actor/Script impl
    // _prefabID is deserialized by Actor/Script impl
    DESERIALIZE_MEMBER(PrefabObjectID, _prefabObjectID);

#if !COMPILE_WITHOUT_CSHARP
    // Handle C# objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsManagedType))
    {
        auto* const v = SERIALIZE_FIND_MEMBER(stream, "V");
        if (v != stream.MemberEnd() && v->value.IsObject() && v->value.MemberCount() != 0)
        {
            ManagedSerialization::Deserialize(v->value, GetOrCreateManagedInstance());
        }
    }
#endif

    // Handle custom scripting objects data serialization
    if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
    {
        auto* const v = SERIALIZE_FIND_MEMBER(stream, "D");
        if (v != stream.MemberEnd() && v->value.IsObject() && v->value.MemberCount() != 0)
        {
            _type.Module->DeserializeObject(v->value, this, modifier);
        }
    }
}
