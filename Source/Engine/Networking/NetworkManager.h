// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Types.h"

API_CLASS(Namespace="FlaxEngine.Networking", Static) class FLAXENGINE_API NetworkManager
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(NetworkManager);
public:
    
    API_FUNCTION()
    static NetworkPeer* CreatePeer(const NetworkConfig& config);
    API_FUNCTION()
    static void ShutdownPeer(NetworkPeer* peer);
    
};
