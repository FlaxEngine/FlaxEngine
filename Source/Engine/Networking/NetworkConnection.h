// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

API_STRUCT(Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkConnection
{
DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkConnection);
public:
    uint32 ConnectionId;
};
