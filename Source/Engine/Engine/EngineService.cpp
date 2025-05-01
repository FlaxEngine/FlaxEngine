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

#if TRACY_ENABLE

StringView FillEventNameBuffer(Char* buffer, StringView name, StringView postfix)
{
    int32 size = 0;
    for (int32 j = 0; j < name.Length(); j++)
        if (name[j] != ' ')
            buffer[size++] = name[j];
    Platform::MemoryCopy(buffer + size, postfix.Get(), (postfix.Length() + 1) * sizeof(Char));
    size += postfix.Length();
    return StringView(buffer, size);
}

#endif

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
        Char nameBuffer[100];
        StringView zoneName = FillEventNameBuffer(nameBuffer, name, StringView(TEXT("::Init"), 6));
        ZoneName(zoneName.Get(), zoneName.Length());
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
            Char nameBuffer[100];
            const StringView name(service->Name);
            StringView zoneName = FillEventNameBuffer(nameBuffer, name, StringView(TEXT("::Dispose"), 9));
            ZoneName(zoneName.Get(), zoneName.Length());
#endif
            service->IsInitialized = false;
            service->Dispose();
        }
    }
}
