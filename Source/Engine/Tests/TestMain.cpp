// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_MAC

#include "Engine/Core/Log.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Scripting/Scripting.h"
#include "Editor/Scripting/ScriptsBuilder.h"

#define CATCH_CONFIG_RUNNER
#include <ThirdParty/catch2/catch.hpp>

class TestsRunnerService : public EngineService
{
public:
    TestsRunnerService()
        : EngineService(TEXT("TestsRunnerService"), 10000)
    {
    }

    void Update() override;
};

TestsRunnerService TestsRunnerServiceInstance;

void TestsRunnerService::Update()
{
    // End if failed to perform a startup
    if (ScriptsBuilder::LastCompilationFailed())
    {
        Engine::RequestExit(-1);
        return;
    }

    // Wait for Editor to be ready for running tests (eg. scripting loaded)
    if (!ScriptsBuilder::IsReady() ||
        !Scripting::IsEveryAssemblyLoaded() ||
        !Scripting::HasGameModulesLoaded())
        return;

    // Runs tests
    Log::Logger::WriteFloor();
    LOG(Info, "Running Flax Tests...");
    const int result = Catch::Session().run();
    if (result == 0)
        LOG(Info, "Flax Tests result: {0}", result);
    else
        LOG(Error, "Flax Tests result: {0}", result);
    Log::Logger::WriteFloor();
    Engine::RequestExit(result);
}

#endif
