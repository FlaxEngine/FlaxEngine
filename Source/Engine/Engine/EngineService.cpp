// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "EngineService.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Sorting.h"

static bool CompareEngineServices(EngineService* const& a, EngineService* const& b)
{
    return a->Order < b->Order;
}

#define DEFINE_ENGINE_SERVICE_EVENT(name) \
    void EngineService::name() { } \
    void EngineService::On##name() \
    { \
        auto& services = GetServices(); \
        for (int32 i = 0; i < services.Count(); i++) \
            services[i]->name(); \
    }
#define DEFINE_ENGINE_SERVICE_EVENT_INVERTED(name) \
    void EngineService::name() { } \
    void EngineService::On##name() \
    { \
        auto& services = GetServices(); \
        for (int32 i = 0; i < services.Count(); i++) \
            services[i]->name(); \
    }

DEFINE_ENGINE_SERVICE_EVENT(FixedUpdate);
DEFINE_ENGINE_SERVICE_EVENT(Update);
DEFINE_ENGINE_SERVICE_EVENT(LateUpdate);
DEFINE_ENGINE_SERVICE_EVENT(Draw);
DEFINE_ENGINE_SERVICE_EVENT_INVERTED(BeforeExit);

EngineService::EngineServicesArray& EngineService::GetServices()
{
    static EngineServicesArray Services;
    return Services;
}

void EngineService::Sort()
{
    auto& services = GetServices();
    Sorting::QuickSort(services.Get(), services.Count(), &CompareEngineServices);
}

EngineService::EngineService(const Char* name, int32 order)
{
    Name = name;
    Order = order;

    auto& services = GetServices();
    services.Add(this);
    if (services.First()->IsInitialized)
        Sort();
}

bool EngineService::Init()
{
    return false;
}

void EngineService::OnInit()
{
    Sort();

    // Init services from front to back
    auto& services = GetServices();
    for (int32 i = 0; i < services.Count(); i++)
    {
        const auto service = services[i];
        LOG(Info, "Initialize {0}...", service->Name);
        service->IsInitialized = true;
        if (service->Init())
        {
            Platform::Fatal(String::Format(TEXT("Failed to initialize {0}."), service->Name));
        }
    }

    LOG(Info, "Engine services are ready!");
}

void EngineService::Dispose()
{
}

void EngineService::OnDispose()
{
    // Dispose services from back to front
    auto& services = GetServices();
    for (int32 i = services.Count() - 1; i >= 0; i--)
    {
        const auto service = services[i];
        if (service->IsInitialized)
        {
            service->IsInitialized = false;
            service->Dispose();
        }
    }
}
