// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

/// <summary>
/// Actor script component that represents missing script.
/// </summary>
API_CLASS(Attributes="HideInEditor") class FLAXENGINE_API MissingScript : public Script
{
    API_AUTO_SERIALIZATION();
    DECLARE_SCRIPTING_TYPE(MissingScript);

private:
    ScriptingObjectReference<Script> _referenceScript;

public:
    /// <summary>
    /// Namespace and type name of missing script.
    /// </summary>
    API_FIELD(Attributes="ReadOnly") String MissingTypeName;

    /// <summary>
    /// Missing script serialized data.
    /// </summary>
    API_FIELD(Hidden, Attributes="HideInEditor") String Data;

    /// <summary>
    /// Field for assigning new script to transfer data to.
    /// </summary>
    API_PROPERTY() ScriptingObjectReference<Script> GetReferenceScript() const
    {
        return _referenceScript;
    }

    /// <summary>
    /// Field for assigning new script to transfer data to.
    /// </summary>
    API_PROPERTY() void SetReferenceScript(const ScriptingObjectReference<Script>& value);
};

#endif
