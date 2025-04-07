// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/ISerializable.h"

/// <summary>
/// Base class for scripting objects that contain in-built serialization via ISerializable interface.
/// </summary>
API_CLASS() class FLAXENGINE_API SerializableScriptingObject : public ScriptingObject, public ISerializable
{
    DECLARE_SCRIPTING_TYPE(SerializableScriptingObject);

    // [ISerializable]
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
