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
        executor.Execute(
            new CoroutineBuilder()
                .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
                .ThenWaitSeconds(1.0f)
                .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
                .ThenWaitFrames(1)
                .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
            );
    }

    [Test]
    public void TestCoroutineSwitching()
    {
        CoroutineExecutor executor = new();

        var coroutineA = new CoroutineBuilder()
            .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspensionPointIndex.Update, () => { });

        executor.Execute(coroutineA);
           
        var coroutineB = new CoroutineBuilder()
            .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
            .ThenWaitSeconds(1.0f)
            .ThenRun(CoroutineSuspensionPointIndex.Update, () => { })
            .ThenWaitFrames(1)
            .ThenRun(CoroutineSuspensionPointIndex.Update, () => { });

        executor.Execute(coroutineB);
    }
}

#endif
