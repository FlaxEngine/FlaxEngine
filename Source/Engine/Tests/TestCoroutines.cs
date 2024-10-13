// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if FLAX_TESTS

using System;
using NUnit.Framework;

namespace FlaxEngine.Tests;

[TestFixture]
public class TestCoroutines
{
    [Test]
    public void TestCoroutineBuilder()
    {
        CoroutineExecutor executor = new();
        executor.ExecuteOnce(
            new CoroutineBuilder()
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
                .ThenWaitSeconds(1.0f)
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
                .ThenWaitFrames(1)
                .ThenRun(CoroutineSuspendPoint.Update, () => { })
            );
    }

    [Test]
    public void TestCoroutineSwitching()
    {
        CoroutineExecutor executor = new();

        var coroutineA = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspendPoint.Update, () => { });

        executor.ExecuteOnce(coroutineA);
           
        var coroutineB = new CoroutineBuilder()
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspendPoint.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspendPoint.Update, () => { });

        executor.ExecuteOnce(coroutineB);
    }
}

#endif
