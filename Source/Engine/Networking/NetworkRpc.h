// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Networking/NetworkManager.h"

class NetworkStream;

// Additional context parameters for Network RPC execution (eg. to identify who sends the data).
API_STRUCT(NoDefault, Namespace="FlaxEngine.Networking") struct FLAXENGINE_API NetworkRpcParams
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(NetworkRpcParams);
    NetworkRpcParams() = default;
    NetworkRpcParams(const NetworkStream* stream);

    /// <summary>
    /// The ClientId of the network client that is a data sender. Can be used to detect who send the incoming RPC or replication data. Ignored when sending data.
    /// </summary>
    API_FIELD() uint32 SenderId = 0;

    /// <summary>
    /// The list of ClientId of the network clients that should receive RPC. Can be used to send RPC to a specific client(s). Ignored when receiving data.
    /// </summary>
    API_FIELD() Span<uint32> TargetIds;
};

// Network RPC identifier name (pair of type and function name)
typedef Pair<ScriptingTypeHandle, StringAnsiView> NetworkRpcName;

// Network RPC descriptor
struct FLAXENGINE_API NetworkRpcInfo
{
    uint8 Server : 1;
    uint8 Client : 1;
    uint8 Channel : 4;
    void (*Execute)(ScriptingObject* obj, NetworkStream* stream, void* tag);
    bool (*Invoke)(ScriptingObject* obj, void** args);
    void* Tag;

    /// <summary>
    /// Global table for registered RPCs. Key: pair of type, RPC name. Value: RPC descriptor.
    /// </summary>
    static Dictionary<NetworkRpcName, NetworkRpcInfo> RPCsTable;
};

// Gets the pointer to the RPC argument into the args buffer
template<typename T>
FORCE_INLINE void NetworkRpcInitArg(Array<void*, FixedAllocation<16>>& args, const T& v)
{
    args.Add((void*)&v);
}

// Gets the pointers to the RPC arguments into the args buffer
template<typename T, typename... Params>
FORCE_INLINE void NetworkRpcInitArg(Array<void*, FixedAllocation<16>>& args, const T& first, Params&&... params)
{
    NetworkRpcInitArg(args, first);
    NetworkRpcInitArg(args, Forward<Params>(params)...);
}

// Network RPC implementation (placed in the beginning of the method body)
#define NETWORK_RPC_IMPL(type, name, ...) \
    { \
    const NetworkRpcInfo* rpcInfoPtr = NetworkRpcInfo::RPCsTable.TryGet(NetworkRpcName(type::TypeInitializer, StringAnsiView(#name))); \
    if (rpcInfoPtr == nullptr) \
    { \
        LOG(Error, "Invalid RPC {0}::{1}. Ensure to use proper type name and method name (and 'Network' tag on a code module).", TEXT(#type), TEXT(#name)); \
        if (Platform::IsDebuggerPresent()) \
            PLATFORM_DEBUG_BREAK; \
        Platform::Assert("Invalid RPC.", __FILE__, __LINE__); \
        return; \
    } \
    const NetworkRpcInfo& rpcInfo = *rpcInfoPtr; \
    const NetworkManagerMode networkMode = NetworkManager::Mode; \
    if ((rpcInfo.Server && networkMode == NetworkManagerMode::Client) || (rpcInfo.Client && networkMode != NetworkManagerMode::Client)) \
    { \
        Array<void*, FixedAllocation<16>> args; \
        NetworkRpcInitArg(args, __VA_ARGS__); \
        if (rpcInfo.Invoke(this, args.Get())) \
            return; \
    } \
    }

// Network RPC override implementation (placed in the beginning of the overriden method body - after call to the base class method)
#define NETWORK_RPC_OVERRIDE_IMPL(type, name, ...) \
    { \
    const NetworkRpcInfo& rpcInfo = NetworkRpcInfo::RPCsTable[NetworkRpcName(type::TypeInitializer, StringAnsiView(#name))]; \
    const NetworkManagerMode networkMode = NetworkManager::Mode; \
    if ((rpcInfo.Server && networkMode == NetworkManagerMode::Client) || (rpcInfo.Client && networkMode != NetworkManagerMode::Client)) \
    { \
        if (rpcInfo.Server && networkMode == NetworkManagerMode::Client) \
            return; \
        if (rpcInfo.Client && networkMode == NetworkManagerMode::Server) \
            return; \
    } \
    }
