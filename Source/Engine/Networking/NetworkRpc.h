// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Scripting/ScriptingType.h"

class NetworkStream;

// Network RPC identifier name (pair of type and function name)
typedef Pair<ScriptingTypeHandle, StringAnsiView> NetworkRpcName;

// Network RPC descriptor
struct FLAXENGINE_API NetworkRpcInfo
{
    uint8 Server : 1;
    uint8 Client : 1;
    uint8 Channel : 4;
    void (*Execute)(ScriptingObject* obj, NetworkStream* stream, void* tag);
    void (*Invoke)(ScriptingObject* obj, void** args);
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
FORCE_INLINE void NetworkRpcInitArg(Array<void*, FixedAllocation<16>>& args, const T& first, Params&... params)
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
        if (Platform::IsDebuggerPresent()) \
            PLATFORM_DEBUG_BREAK; \
        Platform::Assert("Invalid RPC. Ensure to use proper type name and method name.", __FILE__, __LINE__); \
        return; \
    } \
    const NetworkRpcInfo& rpcInfo = *rpcInfoPtr; \
    const NetworkManagerMode networkMode = NetworkManager::Mode; \
    if ((rpcInfo.Server && networkMode == NetworkManagerMode::Client) || (rpcInfo.Client && networkMode != NetworkManagerMode::Client)) \
    { \
        Array<void*, FixedAllocation<16>> args; \
        NetworkRpcInitArg(args, __VA_ARGS__); \
        rpcInfo.Invoke(this, args.Get()); \
        if (rpcInfo.Server && networkMode == NetworkManagerMode::Client) \
            return; \
        if (rpcInfo.Client && networkMode == NetworkManagerMode::Server) \
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
