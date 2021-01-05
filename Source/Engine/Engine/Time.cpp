// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Time.h"
#include "EngineService.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Config/TimeSettings.h"
#include "Engine/Serialization/Serialization.h"

namespace
{
    bool FixedDeltaTimeEnable;
    float FixedDeltaTimeValue;
    float MaxUpdateDeltaTime = 0.1f;
}

bool Time::_gamePaused = false;
float Time::_physicsMaxDeltaTime = 0.1f;
DateTime Time::StartupTime;
float Time::UpdateFPS = 30.0f;
float Time::PhysicsFPS = 60.0f;
float Time::DrawFPS = 60.0f;
float Time::TimeScale = 1.0f;
Time::TickData Time::Update;
Time::FixedStepTickData Time::Physics;
Time::TickData Time::Draw;
Time::TickData* Time::Current = nullptr;

class TimeService : public EngineService
{
public:

    TimeService()
        : EngineService(TEXT("Time"), -850)
    {
        FixedDeltaTimeEnable = false;
        FixedDeltaTimeValue = 0.0f;

#if USE_EDITOR
        // Disable gameplay in Editor on startup
        Time::_gamePaused = true;
#endif
    }
};

TimeService TimeServiceInstance;

void TimeSettings::Apply()
{
    Time::UpdateFPS = UpdateFPS;
    Time::PhysicsFPS = PhysicsFPS;
    Time::DrawFPS = DrawFPS;
    Time::TimeScale = TimeScale;
    ::MaxUpdateDeltaTime = MaxUpdateDeltaTime;
}

void TimeSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    DESERIALIZE(UpdateFPS);
    DESERIALIZE(PhysicsFPS);
    DESERIALIZE(DrawFPS);
    DESERIALIZE(TimeScale);
    DESERIALIZE(MaxUpdateDeltaTime);
}

void Time::TickData::OnBeforeRun(float targetFps, double currentTime)
{
    Time = UnscaledTime = TimeSpan::Zero();
    DeltaTime = UnscaledDeltaTime = targetFps > ZeroTolerance ? TimeSpan::FromSeconds(1.0f / targetFps) : TimeSpan::Zero();
    LastLength = static_cast<double>(DeltaTime.Ticks) / Constants::TicksPerSecond;
    LastBegin = currentTime - LastLength;
    LastEnd = currentTime;
}

void Time::TickData::OnReset(float targetFps, double currentTime)
{
    DeltaTime = UnscaledDeltaTime = targetFps > ZeroTolerance ? TimeSpan::FromSeconds(1.0f / targetFps) : TimeSpan::Zero();
    LastLength = static_cast<double>(DeltaTime.Ticks) / Constants::TicksPerSecond;
    LastBegin = currentTime - LastLength;
    LastEnd = currentTime;
}

bool Time::TickData::OnTickBegin(float targetFps, float maxDeltaTime)
{
    // Check if can perform a tick
    const double time = Platform::GetTimeSeconds();
    double deltaTime;
    if (FixedDeltaTimeEnable)
    {
        deltaTime = (double)FixedDeltaTimeValue;
    }
    else
    {
        deltaTime = Math::Clamp(time - LastBegin, 0.0, (double)maxDeltaTime);
        const double minDeltaTime = targetFps > ZeroTolerance ? 1.0 / (double)targetFps : 0.0;
        if (deltaTime < minDeltaTime)
            return false;
    }

    // Update data
    Advance(time, deltaTime);

    return true;
}

void Time::TickData::OnTickEnd()
{
    const double time = Platform::GetTimeSeconds();
    LastEnd = time;
    LastLength = time - LastBegin;
}

void Time::TickData::Advance(double time, double deltaTime)
{
    float timeScale = TimeScale;
    if (_gamePaused)
        timeScale = 0.0f;
    LastBegin = time;
    UnscaledDeltaTime = TimeSpan::FromSeconds(deltaTime);
    UnscaledTime += UnscaledDeltaTime;
    DeltaTime = TimeSpan::FromSeconds(deltaTime * (double)timeScale);
    Time += DeltaTime;
    TicksCount++;
}

bool Time::FixedStepTickData::OnTickBegin(float targetFps, float maxDeltaTime)
{
    // Check if can perform a tick
    double time = Platform::GetTimeSeconds();
    double deltaTime, minDeltaTime;
    if (FixedDeltaTimeEnable)
    {
        deltaTime = (double)FixedDeltaTimeValue;
        minDeltaTime = deltaTime;
    }
    else
    {
        deltaTime = Math::Clamp(time - LastBegin, 0.0, (double)maxDeltaTime);
        minDeltaTime = targetFps > ZeroTolerance ? 1.0 / (double)targetFps : 0.0;
        if (deltaTime < minDeltaTime)
            return false;
    }
    Samples.Add(deltaTime);

    // Check if last few ticks were not taking too long so it's running slowly
    const bool isRunningSlowly = Samples.Average() > 1.5 * minDeltaTime;
    if (!isRunningSlowly)
    {
        // Make steps fixed size
        const double diff = deltaTime - minDeltaTime;
        time -= diff;
        deltaTime = minDeltaTime;
    }

    // Update data
    Advance(time, deltaTime);

    return true;
}

Time::TickData* Time::GetHighestFrequency(float& fps)
{
    const auto updateFps = UpdateFPS;
    const auto drawFps = DrawFPS;
    const auto physicsFps = PhysicsFPS;

    if (physicsFps >= drawFps && physicsFps >= updateFps)
    {
        fps = physicsFps;
        return &Physics;
    }

    if (drawFps >= physicsFps && drawFps >= updateFps)
    {
        fps = drawFps;
        return &Draw;
    }

    fps = updateFps;
    return &Update;
}

void Time::SetGamePaused(bool value)
{
    if (_gamePaused == value)
        return;

    _gamePaused = value;

    const double time = Platform::GetTimeSeconds();
    Update.OnReset(UpdateFPS, time);
    Physics.OnReset(PhysicsFPS, time);
    Draw.OnReset(DrawFPS, time);
}

float Time::GetDeltaTime()
{
    auto* data = Current ? Current : &Update;
    return data->DeltaTime.GetTotalSeconds();
}

float Time::GetGameTime()
{
    auto* data = Current ? Current : &Update;
    return data->Time.GetTotalSeconds();
}

float Time::GetUnscaledDeltaTime()
{
    auto* data = Current ? Current : &Update;
    return data->UnscaledDeltaTime.GetTotalSeconds();
}

float Time::GetUnscaledGameTime()
{
    auto* data = Current ? Current : &Update;
    return data->UnscaledTime.GetTotalSeconds();
}

float Time::GetTimeSinceStartup()
{
    return (DateTime::Now() - StartupTime).GetTotalSeconds();
}

void Time::SetFixedDeltaTime(bool enable, float value)
{
    FixedDeltaTimeEnable = enable;
    FixedDeltaTimeValue = value;
}

void Time::OnBeforeRun()
{
    // Initialize tick data (based on a time settings)
    const double time = Platform::GetTimeSeconds();
    Update.OnBeforeRun(UpdateFPS, time);
    Physics.OnBeforeRun(PhysicsFPS, time);
    Draw.OnBeforeRun(DrawFPS, time);
}

bool Time::OnBeginUpdate()
{
    if (Update.OnTickBegin(UpdateFPS, MaxUpdateDeltaTime))
    {
        Current = &Update;
        return true;
    }
    return false;
}

bool Time::OnBeginPhysics()
{
    if (Physics.OnTickBegin(PhysicsFPS, _physicsMaxDeltaTime))
    {
        Current = &Physics;
        return true;
    }
    return false;
}

bool Time::OnBeginDraw()
{
    if (Draw.OnTickBegin(DrawFPS, 1.0f))
    {
        Current = &Draw;
        return true;
    }
    return false;
}

void Time::OnEndUpdate()
{
    Update.OnTickEnd();
    Current = nullptr;
}

void Time::OnEndPhysics()
{
    Physics.OnTickEnd();
    Current = nullptr;
}

void Time::OnEndDraw()
{
    Draw.OnTickEnd();
    Current = nullptr;
}
