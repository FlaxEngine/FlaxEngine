// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Core/Collections/SamplesBuffer.h"

/// <summary>
/// Game ticking and timing system.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Time
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Time);
    friend class Engine;
    friend class TimeService;
    friend class PhysicsSettings;
public:

    /// <summary>
    /// Engine subsystem updating data.
    /// Used to invoke game logic updates, physics updates and rendering with possibly different frequencies.
    /// </summary>
    class TickData
    {
    public:

        virtual ~TickData() = default;

        /// <summary>
        /// The total amount of tick since start.
        /// </summary>
        uint64 TicksCount = 0;

        /// <summary>
        /// The last tick start time (gathered from PlatformTime::Seconds)
        /// </summary>
        double LastBegin;

        /// <summary>
        /// The last tick end time (gathered from PlatformTime::Seconds)
        /// </summary>
        double LastEnd;

        /// <summary>
        /// The last tick length in seconds. Note: LastEnd-LastBegin may be invalid inside a tick but LastLength is always valid.
        /// </summary>
        double LastLength;

        /// <summary>
        /// The delta time.
        /// </summary>
        TimeSpan DeltaTime;

        /// <summary>
        /// The total time.
        /// </summary>
        TimeSpan Time;

        /// <summary>
        /// The unscaled delta time.
        /// </summary>
        TimeSpan UnscaledDeltaTime;

        /// <summary>
        /// The unscaled total time.
        /// </summary>
        TimeSpan UnscaledTime;

    public:

        virtual void OnBeforeRun(float targetFps, double currentTime);
        virtual void OnReset(float targetFps, double currentTime);
        virtual bool OnTickBegin(float targetFps, float maxDeltaTime);
        virtual void OnTickEnd();

    protected:

        void Advance(double time, double deltaTime);
    };

    /// <summary>
    /// Ticking method that tries to use fixed steps policy as much as possible (if not running slowly).
    /// </summary>
    class FixedStepTickData : public TickData
    {
    public:

        /// <summary>
        /// The last few ticks delta times. Used to check if can use fixed steps or whenever is running slowly so should use normal stepping.
        /// </summary>
        SamplesBuffer<double, 4> Samples;

    public:

        // [TickData]
        bool OnTickBegin(float targetFps, float maxDeltaTime) override;
    };

private:

    static bool _gamePaused;
    static float _physicsMaxDeltaTime;

public:

    /// <summary>
    /// The time at which the game started (UTC local).
    /// </summary>
    API_FIELD(ReadOnly) static DateTime StartupTime;

    /// <summary>
    /// The target amount of the game logic updates per second (script updates frequency).
    /// </summary>
    API_FIELD() static float UpdateFPS;

    /// <summary>
    /// The target amount of the physics simulation updates per second (also fixed updates frequency).
    /// </summary>
    API_FIELD() static float PhysicsFPS;

    /// <summary>
    /// The target amount of the frames rendered per second (target game FPS).
    /// </summary>
    /// <remarks>
    /// To get the actual game FPS use <see cref="Engine.FramesPerSecond"/>
    /// </remarks>
    API_FIELD() static float DrawFPS;

    /// <summary>
    /// The game time scale factor. Default is 1.
    /// </summary>
    API_FIELD() static float TimeScale;

public:

    /// <summary>
    /// The game logic updating data.
    /// </summary>
    static TickData Update;

    /// <summary>
    /// The physics simulation updating data.
    /// </summary>
    static FixedStepTickData Physics;

    /// <summary>
    /// The rendering data.
    /// </summary>
    static TickData Draw;

    /// <summary>
    /// The current tick data (update, physics or draw).
    /// </summary>
    static TickData* Current;

public:

    /// <summary>
    /// Gets the current tick data (safety so returns Update tick data if no active).
    /// </summary>
    /// <returns>The tick data.</returns>
    FORCE_INLINE static TickData* GetCurrentSafe()
    {
        return Current ? Current : &Update;
    }

    /// <summary>
    /// Gets the tick data that uses the highest frequency for the ticking.
    /// </summary>
    /// <param name="fps">The FPS rate of the highest frequency ticking group.</param>
    /// <returns>The tick data.</returns>
    static TickData* GetHighestFrequency(float& fps);

    /// <summary>
    /// Gets the value indicating whenever game logic is paused (physics, script updates, etc.).
    /// </summary>
    /// <returns>True if game is being paused, otherwise false.</returns>
    API_PROPERTY() FORCE_INLINE static bool GetGamePaused()
    {
        return _gamePaused;
    }

    /// <summary>
    /// Sets the value indicating whenever game logic is paused (physics, script updates, etc.).
    /// </summary>
    /// <param name="value">>True if pause game logic, otherwise false.</param>
    API_PROPERTY() static void SetGamePaused(bool value);

    /// <summary>
    /// Gets time in seconds it took to complete the last frame, <see cref="TimeScale"/> dependent.
    /// </summary>
    API_PROPERTY() static float GetDeltaTime();

    /// <summary>
    /// Gets time at the beginning of this frame. This is the time in seconds since the start of the game.
    /// </summary>
    API_PROPERTY() static float GetGameTime();

    /// <summary>
    /// Gets timeScale-independent time in seconds it took to complete the last frame.
    /// </summary>
    API_PROPERTY() static float GetUnscaledDeltaTime();

    /// <summary>
    /// Gets timeScale-independent time at the beginning of this frame. This is the time in seconds since the start of the game.
    /// </summary>
    API_PROPERTY() static float GetUnscaledGameTime();

    /// <summary>
    /// Gets the time since startup in seconds (unscaled).
    /// </summary>
    API_PROPERTY() static float GetTimeSinceStartup();

    /// <summary>
    /// Sets the fixed FPS for game logic updates (draw and update).
    /// </summary>
    /// <param name="enable">>True if enable this feature, otherwise false.</param>
    /// <param name="value">>The fixed draw/update rate for the time.</param>
    API_FUNCTION() static void SetFixedDeltaTime(bool enable, float value);

private:

    // Methods used by the Engine class

    static void OnBeforeRun();

    static bool OnBeginUpdate();
    static bool OnBeginPhysics();
    static bool OnBeginDraw();

    static void OnEndUpdate();
    static void OnEndPhysics();
    static void OnEndDraw();
};
