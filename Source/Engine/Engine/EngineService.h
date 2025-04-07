// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Engine service object.
/// </summary>
class EngineService
{
public:
    typedef Array<EngineService*, FixedAllocation<128>> EngineServicesArray;

public:

    /// <summary>
    /// Gets the list with all registered services.
    /// </summary>
    static EngineServicesArray& GetServices();

    /// <summary>
    /// Sorts the registered services (sorting is skipped before first service initialization).
    /// </summary>
    static void Sort();

private:

    bool IsInitialized = false;

protected:

    EngineService(const Char* name, int32 order = 0);

public:

    virtual ~EngineService() = default;

    const Char* Name;
    int32 Order;

#define DECLARE_ENGINE_SERVICE_EVENT(result, name) virtual result name(); static void On##name();
    DECLARE_ENGINE_SERVICE_EVENT(bool, Init);
    DECLARE_ENGINE_SERVICE_EVENT(void, FixedUpdate);
    DECLARE_ENGINE_SERVICE_EVENT(void, Update);
    DECLARE_ENGINE_SERVICE_EVENT(void, LateUpdate);
    DECLARE_ENGINE_SERVICE_EVENT(void, LateFixedUpdate);
    DECLARE_ENGINE_SERVICE_EVENT(void, Draw);
    DECLARE_ENGINE_SERVICE_EVENT(void, BeforeExit);
    DECLARE_ENGINE_SERVICE_EVENT(void, Dispose);
#undef DECLARE_ENGINE_SERVICE_EVENT
};
