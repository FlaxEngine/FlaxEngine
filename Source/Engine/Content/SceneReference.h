// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/ISerializable.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Represents the reference to the scene asset. Stores the unique ID of the scene to reference. Can be used to load the selected scene.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API SceneReference
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(SceneReference);

    /// <summary>
    /// The identifier of the scene asset (and the scene object).
    /// </summary>
    API_FIELD() Guid ID;

    FORCE_INLINE bool operator==(const SceneReference& other) const
    {
        return ID == other.ID;
    }

    FORCE_INLINE bool operator!=(const SceneReference& other) const
    {
        return ID != other.ID;
    }
};

template<>
struct TIsPODType<SceneReference>
{
    enum { Value = true };
};

inline uint32 GetHash(const SceneReference& key)
{
    return GetHash(key.ID);
}

// @formatter:off
namespace Serialization
{
    inline bool ShouldSerialize(const SceneReference& v, const void* otherObj)
    {
        return !otherObj || v != *(SceneReference*)otherObj;
    }
    void FLAXENGINE_API Serialize(ISerializable::SerializeStream& stream, const SceneReference& v, const void* otherObj);
    void FLAXENGINE_API Deserialize(ISerializable::DeserializeStream& stream, SceneReference& v, ISerializeModifier* modifier);
}
// @formatter:on
