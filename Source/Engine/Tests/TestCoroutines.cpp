// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Engine/Scripting/Coroutines/CoroutineExecutor.h"
#include <ThirdParty/catch2/catch.hpp>

namespace 
{
    using ExecutorReference = ScriptingObjectReference<CoroutineExecutor>;
    using BuilderReference  = ScriptingObjectReference<CoroutineBuilder>;
    using HandleReference   = ScriptingObjectReference<CoroutineHandle>;

    ExecutorReference NewCoroutineExecutor() { return ScriptingObject::NewObject<CoroutineExecutor>(); }
    BuilderReference  NewCoroutineBuilder()  { return ScriptingObject::NewObject<CoroutineBuilder>();  }
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

    CHECK(result == 0);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 0.0s (1st func exec, start of 1.0s wait)
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
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // 3rd frame (end of 3 frame wait, 3rd func exec)
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

    CHECK(result == 0);
    executor->Continue(CoroutineSuspendPoint::Update, 0.0f); // init (1st func exec, wait until miss)
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
