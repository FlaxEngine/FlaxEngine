// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// Network connection structure - used to identify connected peers when we're listening.
/// </summary>
API_STRUCT(Namespace="FlaxEngine.Networking", NoDefault) struct FLAXENGINE_API NetworkConnection
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkConnection);

public:
    /// <summary>
    /// The identifier of the connection.
    /// </summary>
    /// <remarks>Used by network driver implementations.</remarks>
    API_FIELD() uint32 ConnectionId;
};

template<>
struct TIsPODType<NetworkConnection>
{
    enum { Value = true };
};

inline bool operator==(const NetworkConnection& a, const NetworkConnection& b)
{
    return a.ConnectionId == b.ConnectionId;
}
