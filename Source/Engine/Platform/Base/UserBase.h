// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/ScriptingObject.h"

API_INJECT_CODE(cpp, "#include \"Engine/Platform/User.h\"");

/// <summary>
/// Native platform user object.
/// </summary>
API_CLASS(NoSpawn, NoConstructor, Sealed, Name="User")
class FLAXENGINE_API UserBase : public ScriptingObject
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(UserBase);
protected:

    const String _name;

public:

    UserBase(const String& name);
    UserBase(const SpawnParams& params, const String& name);

    /// <summary>
    /// Gets the username.
    /// </summary>
    API_PROPERTY() String GetName() const;
};
