// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "AnimEvent.h"
#include "Engine/Scripting/BinaryModule.h"
#include "Engine/Scripting/Internal/ManagedSerialization.h"
#include "Engine/Serialization/SerializationFwd.h"
#include "Engine/Serialization/Serialization.h"

AnimEvent::AnimEvent(const SpawnParams& params)
    : ScriptingObject(params)
{
}

void AnimEvent::Serialize(SerializeStream& stream, const void* otherObj)
{
    SERIALIZE_GET_OTHER_OBJ(AnimEvent);

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

void AnimEvent::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
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

AnimContinuousEvent::AnimContinuousEvent(const SpawnParams& params)
    : AnimEvent(params)
{
}
