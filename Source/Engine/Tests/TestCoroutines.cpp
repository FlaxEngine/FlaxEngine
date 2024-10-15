// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Scripting/Coroutines/CoroutineExecutor.h"
#include <ThirdParty/catch2/catch.hpp>

namespace 
{
    using ExecutorReference = ScriptingObjectReference<CoroutineExecutor>;
    using HandleReference   = ScriptingObjectReference<CoroutineHandle>;

    ExecutorReference NewCoroutineExecutor() { return ScriptingObject::NewObject<CoroutineExecutor>(); }
}

TEST_CASE("CoroutinesBuilder")
{
    const ExecutorReference executor = NewCoroutineExecutor();
    const HandleReference   handle = executor->ExecuteOnce(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenWaitFrames(1)
            ->ThenWaitSeconds(1.0f)
            ->ThenRunFunc([]() -> void {})
    );

    CHECK(handle != nullptr);
}

TEST_CASE("CoroutinesSwitching")
{
    const ExecutorReference executor = NewCoroutineExecutor();
    CHECK(executor->GetCoroutinesCount() == 0);

    const HandleReference handle1 = executor->ExecuteOnce(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenWaitFrames(1)
            ->ThenWaitSeconds(1.0f)
            ->ThenRunFunc([]() -> void {})
    );
    CHECK(handle1 != nullptr);
    CHECK(executor->GetCoroutinesCount() == 1);

    const HandleReference handle2 = executor->ExecuteOnce(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenWaitFrames(1)
            ->ThenWaitSeconds(1.0f)
            ->ThenRunFunc([]() -> void {})
    );
    CHECK(handle2 != nullptr);
    CHECK(executor->GetCoroutinesCount() == 2);
}

TEST_CASE("CoroutineTimeAccumulation")
{
    int result = 0;
    const ExecutorReference executor = NewCoroutineExecutor();

    const HandleReference handle = executor->ExecuteOnce(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenRunFunc([&result]{ ++result; })
            ->ThenWaitSeconds(1.0f)
            ->ThenRunFunc([&result]{ ++result; })
            ->ThenWaitFrames(3)
            ->ThenRunFunc([&result]{ ++result; })
    );

    // init (1st func exec)
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.3f); // 0.3s
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.3f); // 0.6s
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.3f); // 0.9s
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.3f); // 1.2s, 1st frame (end of 1.0s wait, 2nd func exec, start of 3 frame wait)
    CHECK(result == 2);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 2nd frame
    CHECK(result == 2);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 3rd frame
    CHECK(result == 2);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 4th frame (end of 3 frame wait, 3rd func exec)
    CHECK(result == 3);
}

TEST_CASE("CoroutineWaitForSuspensionPoint")
{
    //TODO(mtszkarbowiak) Test CoroutineWaitForSuspensionPoint
}

TEST_CASE("CoroutineWaitUntil")
{
    int result = 0;
    int signal = 0;

    const ExecutorReference executor = NewCoroutineExecutor();
    const HandleReference handle = executor->ExecuteOnce(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenRunFunc([&result]{ ++result; })
            ->ThenWaitUntilFunc([&signal](bool& canContinue){ canContinue = signal == 1; })
            ->ThenRunFunc([&result]{ ++result; })
    );

    // init (1st func exec, wait until miss)
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // before signal (wait until miss)
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // before signal (wait until miss again)
    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // before signal (wait until miss again)
    CHECK(result == 1);

    signal = 1;

    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // after signal (wait until hit, 2nd func exec)
    CHECK(result == 2);
}

TEST_CASE("CoroutineExecuteRepeating")
{
    int result = 0;

    constexpr int32 repeats = 4;
    const ExecutorReference executor = NewCoroutineExecutor();
    const HandleReference handle = executor->ExecuteRepeats(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenRunFunc([&result] { ++result; })
            ->ThenWaitFrames(1),
        repeats
    );

    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f);
    CHECK(result == 2);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f);
    CHECK(result == 3);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f);
    CHECK(result == 4);

    //TODO(mtszkarbowiak) Add check if the handle is still running. (Expected no)
}

TEST_CASE("CoroutineExecuteLoop")
{
    int result = 0;

    const ExecutorReference executor = NewCoroutineExecutor();
    const HandleReference handle = executor->ExecuteLooped(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenRunFunc([&result]{  ++result; })
            ->ThenWaitFrames(1)
    );

    // 1
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 1st call (wait, func exec)
    // 2
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 2nd call (wait, func exec)
    // 3
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 3rd call (wait, func exec)
    // 4

    CHECK(result == 4);

    //TODO(mtszkarbowiak) Add check if the handle is still running. (Expected yes)
}

TEST_CASE("CoroutineHandlePauseResume")
{
    int result = 0;
    const ExecutorReference executor = NewCoroutineExecutor();

    HandleReference handle = executor->ExecuteOnce(
        ScriptingObject::NewObject<CoroutineBuilder>()
            ->ThenRunFunc([&result]() -> void { ++result;  })
            ->ThenWaitFrames(1) // r = 1
            ->ThenRunFunc([&result]() -> void { ++result; })
            ->ThenWaitFrames(1) // r = 2
            ->ThenRunFunc([&result]() -> void { ++result; })
    );

    CHECK(result == 1);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 1st call (func exec)
    CHECK(result == 2);

    const bool paused0 = handle->Pause();
    CHECK(paused0);
    const bool paused1 = handle->Pause();
    CHECK(!paused1);

    CHECK(result == 2);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 2nd call (wait, func exec)
    CHECK(result == 2);

    const bool resumed0 = handle->Resume();
    CHECK(resumed0);
    const bool resumed1 = handle->Resume();
    CHECK(!resumed1);

    CHECK(executor->GetCoroutinesCount() == 1);

    CHECK(result == 2);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 2nd call (wait, func exec)
    CHECK(result == 3);

    CHECK(executor->GetCoroutinesCount() == 0);
}

TEST_CASE("CoroutineHandleCancel")
{
    
}
