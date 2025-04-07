// Copyright (c) Wojciech Figat. All rights reserved.

#include "EngineService.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/Sorting.h"
#include <ThirdParty/tracy/tracy/Tracy.hpp>

static bool CompareEngineServices(EngineService* const& a, EngineService* const& b)
{
    return a->Order < b->Order;
}

#define DEFINE_ENGINE_SERVICE_EVENT(name) \
    void EngineService::name() { } \
    void EngineService::On##name() \
    { \
        ZoneScoped; \
        auto& services = GetServices(); \
        for (int32 i = 0; i < services.Count(); i++) \
            services[i]->name(); \
    }
#define DEFINE_ENGINE_SERVICE_EVENT_INVERTED(name) \
    void EngineService::name() { } \
    void EngineService::On##name() \
    { \
        ZoneScoped; \
        auto& services = GetServices(); \
        for (int32 i = 0; i < services.Count(); i++) \
            services[i]->name(); \
    }

DEFINE_ENGINE_SERVICE_EVENT(FixedUpdate);
DEFINE_ENGINE_SERVICE_EVENT(Update);
DEFINE_ENGINE_SERVICE_EVENT(LateUpdate);
DEFINE_ENGINE_SERVICE_EVENT(LateFixedUpdate);
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
    ZoneScoped;
    Sort();

    // Init services from front to back
    auto& services = GetServices();
    for (int32 i = 0; i < services.Count(); i++)
    {
        const auto service = services[i];
        const StringView name(service->Name);
#if TRACY_ENABLE
        ZoneScoped;
        int32 nameBufferLength = 0;
        Char nameBuffer[100];
        for (int32 j = 0; j < name.Length(); j++)
            if (name[j] != ' ')
                nameBuffer[nameBufferLength++] = name[j];
        Platform::MemoryCopy(nameBuffer + nameBufferLength, TEXT("::Init"), 7 * sizeof(Char));
        nameBufferLength += 7;
        ZoneName(nameBuffer, nameBufferLength);
#endif
        LOG(Info, "Initialize {0}...", name);
        service->IsInitialized = true;
        if (service->Init())
        {
            Platform::Fatal(String::Format(TEXT("Failed to initialize {0}."), name));
        }
    }

    LOG(Info, "Engine services are ready!");
}

void EngineService::Dispose()
{
}

void EngineService::OnDispose()
{
    ZoneScoped;
    // Dispose services from back to front
    auto& services = GetServices();
    for (int32 i = services.Count() - 1; i >= 0; i--)
    {
        const auto service = services[i];
        if (service->IsInitialized)
        {
#if TRACY_ENABLE
            ZoneScoped;
            const StringView name(service->Name);
            int32 nameBufferLength = 0;
            Char nameBuffer[100];
            for (int32 j = 0; j < name.Length(); j++)
                if (name[j] != ' ')
                    nameBuffer[nameBufferLength++] = name[j];
            Platform::MemoryCopy(nameBuffer + nameBufferLength, TEXT("::Dispose"), 10 * sizeof(Char));
            nameBufferLength += 10;
            ZoneName(nameBuffer, nameBufferLength);
#endif
            service->IsInitialized = false;
            service->Dispose();
        }
    }
}
