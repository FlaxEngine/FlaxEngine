// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

API_ENUM(Namespace="FlaxEngine.Networking") enum class NetworkChannelType
{
    None = 0,
    
    Unreliable,
    UnreliableOrdered,
    
    Reliable,
    ReliableOrdered
};
