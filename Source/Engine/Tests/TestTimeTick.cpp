// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Engine/Time.h"
#include "Engine/Platform/Platform.h"
#include <ThirdParty/catch2/catch.hpp>

TEST_CASE("TimeTick")
{
    // Keep the engine's tick settings to restore them after the test
    const float updateFps = Time::UpdateFPS, physicsFps = Time::PhysicsFPS, drawFps = Time::DrawFPS;
    const double nextUpdate = Time::Update.NextBegin, nextPhysics = Time::Physics.NextBegin, nextDraw = Time::Draw.NextBegin;
    const double now = Platform::GetTimeSeconds();

    SECTION("Test GetNextTick with uncapped drawing")
    {
        // The update and physics ticks are far away, but drawing is uncapped: a frame is always due (the main loop must not idle)
        Time::UpdateFPS = 60.0f;
        Time::PhysicsFPS = 60.0f;
        Time::DrawFPS = 0.0f;
        Time::Update.NextBegin = now + 1.0;
        Time::Physics.NextBegin = now + 1.0;
        CHECK(Time::GetNextTick() == 0.0);
    }

    SECTION("Test GetNextTick with capped drawing")
    {
        // The next tick is the earliest of the update, physics and draw ticks
        Time::UpdateFPS = 60.0f;
        Time::PhysicsFPS = 60.0f;
        Time::DrawFPS = 60.0f;
        Time::Update.NextBegin = now + 1.0;
        Time::Physics.NextBegin = now + 2.0;
        Time::Draw.NextBegin = now + 0.5;
        CHECK(Time::GetNextTick() == now + 0.5);
    }

    Time::UpdateFPS = updateFps;
    Time::PhysicsFPS = physicsFps;
    Time::DrawFPS = drawFps;
    Time::Update.NextBegin = nextUpdate;
    Time::Physics.NextBegin = nextPhysics;
    Time::Draw.NextBegin = nextDraw;
}
