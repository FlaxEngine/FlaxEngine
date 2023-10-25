// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "Online.h"
#include "IOnlinePlatform.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Scripting/ScriptingObject.h"

class OnlineService : public EngineService
{
public:
    OnlineService()
        : EngineService(TEXT("Online"), 500)
    {
    }

    void Dispose() override
    {
        // Cleanup current online platform
        Online::Initialize(nullptr);
    }
};

IOnlinePlatform* Online::Platform = nullptr;
Action Online::PlatformChanged;
OnlineService OnlineServiceInstance;

bool Online::Initialize(IOnlinePlatform* platform)
{
    if (Platform == platform)
        return false;
    const auto object = ScriptingObject::FromInterface(platform, IOnlinePlatform::TypeInitializer);
    LOG(Info, "Changing online platform to {0}", object ? object->ToString() : (platform ? TEXT("?") : TEXT("none")));

    if (Platform)
    {
        Platform->Deinitialize();
    }
    Platform = platform;
    if (Platform)
    {
        if (Platform->Initialize())
        {
            Platform = nullptr;
            LOG(Error, "Failed to initialize online platform.");
            return true;
        }
    }

    return false;
}
